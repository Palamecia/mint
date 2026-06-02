/**
 * Copyright (c) 2026 Gauvain CHERY.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "mint/system/async_io.h"
#include "mint/config.h"

#include <array>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <optional>
#include <span>
#include <system_error>
#include <unordered_map>
#include <mutex>
#include <variant>

#if defined(MINT_OS_MAC)
#include <sys/event.h>
#include <sys/types.h>
#elif defined(MINT_OS_LINUX)
#include <sys/epoll.h>
#include <unistd.h>
// io_uring headers (optional - with fallback to epoll)
#if HAS_IO_URING
#include <liburing.h>
#include <linux/time_types.h>
#else
#endif
#else
#include <sys/epoll.h>
#include <unistd.h>
#endif

mint::AsyncOperation::AsyncOperation(handle_t handle) :
    async_operation_t({}),
    _handle(handle) {}

#ifdef MINT_ASYNC_BACKEND_KQUEUE

mint::AsyncRuntime::AsyncRuntime() :
    _context(kqueue()) {
	if (_context == -1) {
		// Initialization failed
		_context = -1;
	}
}

mint::AsyncRuntime::~AsyncRuntime() {
	if (_context >= 0) {
		close(_context);
	}
}

bool mint::AsyncRuntime::submit(AsyncOperation& operation) {

	if (_context < 0) [[unlikely]] {
		return false;
	}

	if (const auto error = operation.start()) {
		if (error != std::make_error_code(std::errc::resource_unavailable_try_again) && error.value() != EAGAIN
		    && error.value() != EWOULDBLOCK) {
			operation.complete(error, 0);
			return false;
		}
		operation.pending = true;
	}

	if (!operation.pending) {
		operation.complete({}, static_cast<std::size_t>(operation.result));
		return false;
	}

	const auto event = kevent {
	    .ident = static_cast<uintptr_t>(operation.get_handle()),
	    .filter = static_cast<int16_t>(operation.filter),
	    .flags = static_cast<uint16_t>(operation.flags | EV_ADD | EV_ENABLE | EV_ONESHOT),
	    .fflags = 0,
	    .data = 0,
	    .udata = &operation,
	};
	if (kevent(_context, &event, 1, nullptr, 0, nullptr) < 0) {
		operation.complete(std::error_code(errno, std::system_category()), 0);
		return false;
	}

	const auto _ = std::scoped_lock(_mutex);
	_operations.emplace(&operation);
	return true;
}

bool mint::AsyncRuntime::cancel(AsyncOperation& operation) {

	if (_context < 0) [[unlikely]] {
		return false;
	}

	const auto _ = std::scoped_lock(_mutex);

	const std::uintptr_t operation_id = reinterpret_cast<std::uintptr_t>(&operation);
	auto it = _operations.find(operation_id);
	if (it != _operations.end()) {
		const auto event = kevent {
		    .ident = static_cast<uintptr_t>(operation.get_handle()),
		    .filter = static_cast<int16_t>(operation.filter),
		    .flags = EV_DELETE,
		    .fflags = 0,
		    .data = 0,
		    .udata = nullptr,
		};
		kevent(_context, &event, 1, nullptr, 0, nullptr);
		_operations.erase(it);
		return true;
	}

	return false;
}

mint::AsyncOperation* mint::AsyncRuntime::poll(std::optional<std::chrono::milliseconds> timeout) {

	if (_context < 0) [[unlikely]] {
		return false;
	}

	struct kevent events[16];
	struct timespec timeout_ts = timeout
	                                 .transform([](std::chrono::milliseconds ms) {
		                                 const auto timeout_ms = ms.count();
		                                 return timespec {
			                                 .tv_sec = timeout_ms / 1000;
			                                 .tv_nsec = (timeout_ms % 1000) * 1000000;
		                                 };
	                                 })
	                                 .value_or({});

	int nu_events = kevent(_context, nullptr, 0, events, 16, timeout ? &timeout_ts : nullptr);
	if (nu_events < 0) {
		return false; // Error
	}

	const auto _ = std::scoped_lock(_mutex);

	// Process completed events
	for (int i = 0; i < nu_events; ++i) {
		const auto operation_id = reinterpret_cast<std::uintptr_t>(events[i].udata);
		auto it = _operations.find(operation_id);
		if (it != _operations.end()) {
			AsyncOperation* operation = it->second;
			_operations.erase(it);
			operation->pending = false;
			if (const auto error = operation->start()) {
				if (error.value() == EAGAIN || error.value() == EWOULDBLOCK) {
					operation->pending = true;

					const auto retry = struct kevent {
						.ident = static_cast<uintptr_t>(operation->get_handle()),
						.filter = static_cast<int16_t>(operation->filter),
						.flags = static_cast<uint16_t>(operation->flags | EV_ADD | EV_ENABLE | EV_ONESHOT), .fflags = 0,
						.data = 0, .udata = operation,
					};

					kevent(_context, &retry, 1, nullptr, 0, nullptr);
					_operations.emplace(operation);
				}
				else {
					operation->complete(error, 0);
				}
			}
			else {
				operation->complete({}, static_cast<std::size_t>(operation->result));
			}
		}
	}

	return true;
}

#elifdef MINT_OS_LINUX

#ifdef MINT_ASYNC_BACKEND_IO_URING
#include <liburing/io_uring.h>
#endif

mint::AsyncRuntime::AsyncRuntime() {
#ifdef MINT_ASYNC_BACKEND_IO_URING
	if (auto context = io_uring(); io_uring_queue_init(256, &context, 0) >= 0) {
		_context = context;
		return;
	}
#endif
#ifdef MINT_ASYNC_BACKEND_EPOLL
	if (auto context = epoll_create1(EPOLL_CLOEXEC); context >= 0) {
		_context = EPollContext {
		    .fd = context,
		};
		return;
	}
#endif
}

mint::AsyncRuntime::~AsyncRuntime() {
	std::visit(Overloaded {
#ifdef MINT_ASYNC_BACKEND_IO_URING
	               [](io_uring& context) {
		               io_uring_queue_exit(&context);
	               },
#endif
#ifdef MINT_ASYNC_BACKEND_EPOLL
	               [](EPollContext& context) {
		               close(context.fd);
	               },
#endif
	               [](std::monostate) {},
	           },
	    _context);
}

std::error_code mint::AsyncRuntime::register_handle(handle_t /*handle*/) {
	return {};
}

bool mint::AsyncRuntime::submit(AsyncOperation& operation) {
	return std::visit<bool>(Overloaded {
#ifdef MINT_ASYNC_BACKEND_IO_URING
	                            [this, &operation](io_uring& context) -> bool {
		                            auto& op = operation.emplace<IoUringOperation>();
		                            auto lock = std::unique_lock(_mutex);

		                            op.sqe = io_uring_get_sqe(&context);
		                            if (op.sqe == nullptr) [[unlikely]] {
			                            return false;
		                            }

		                            lock.unlock();

		                            if (const auto error = operation.start()) {
			                            operation.complete(error, 0);
			                            return false;
		                            }

		                            lock.lock();

		                            const auto operation_id = reinterpret_cast<std::uint64_t>(&operation);
		                            io_uring_sqe_set_data64(op.sqe, operation_id);
		                            _operations.emplace(&operation);
		                            io_uring_submit(&context);
		                            return true;
	                            },
#endif
#ifdef MINT_ASYNC_BACKEND_EPOLL
	                            [this, &operation](EPollContext& context) -> bool {
		                            auto& op = operation.emplace<EPollOperation>();
		                            if (const auto error = operation.start()) {
			                            if (error.value() != EAGAIN && error.value() != EWOULDBLOCK) {
				                            operation.complete(error, 0);
				                            return false;
			                            }
			                            op.pending = true;
		                            }

		                            if (op.pending) {

			                            if (op.events == 0) [[unlikely]] {
				                            operation.complete(std::make_error_code(std::errc::operation_not_supported),
				                                0);
				                            return false;
			                            }

			                            auto event = epoll_event {
			                                .events = op.events | EPOLLERR | EPOLLHUP,
			                                .data =
			                                    {
			                                        .ptr = &operation,
			                                    },
			                            };
			                            if (epoll_ctl(context.fd, EPOLL_CTL_ADD, operation.get_handle(), &event) < 0) {
				                            operation.complete(std::error_code(errno, std::system_category()), 0);
				                            return false;
			                            }
		                            }
		                            else {
			                            context.ready_operations.push_back(&operation);
		                            }

		                            const auto _ = std::scoped_lock(_mutex);
		                            _operations.emplace(&operation);
		                            return true;
	                            },
#endif
	                            [](std::monostate) -> bool {
		                            return false;
	                            },
	                        },
	    _context);
}

bool mint::AsyncRuntime::cancel(AsyncOperation& operation) {

	return std::visit<bool>(Overloaded {
#ifdef MINT_ASYNC_BACKEND_IO_URING
	                            [this, &operation](io_uring&) -> bool {
		                            const auto _ = std::scoped_lock(_mutex);

		                            if (auto it = _operations.find(&operation); it != _operations.end()) {
			                            _operations.erase(it);
			                            return true;
		                            }

		                            return false;
	                            },
#endif
#ifdef MINT_ASYNC_BACKEND_EPOLL
	                            [this, &operation](EPollContext& context) -> bool {
		                            const auto _ = std::scoped_lock(_mutex);

		                            if (auto it = _operations.find(&operation); it != _operations.end()) {
			                            epoll_ctl(context.fd, EPOLL_CTL_DEL, operation.get_handle(), nullptr);
			                            _operations.erase(it);
			                            return true;
		                            }

		                            return false;
	                            },
#endif
	                            [](std::monostate) -> bool {
		                            return false;
	                            },
	                        },
	    _context);
}

std::error_code mint::AsyncRuntime::post_deferred_completion(AsyncOperation& operation) {
	return std::visit<std::error_code>(Overloaded {
#ifdef MINT_ASYNC_BACKEND_IO_URING
	                                       [this, &operation](io_uring& context) {
		                                       const auto _ = std::scoped_lock(_mutex);

		                                       auto* sqe = io_uring_get_sqe(&context);
		                                       if (sqe == nullptr) {
			                                       return std::make_error_code(
			                                           std::errc::resource_unavailable_try_again);
		                                       }

		                                       const auto operation_id = reinterpret_cast<std::uint64_t>(&operation);
		                                       io_uring_prep_msg_ring(sqe, context.ring_fd, 0, operation_id, 0);
		                                       if (io_uring_submit(&context) < 0) {
			                                       return std::make_error_code(std::errc::io_error);
		                                       }
		                                       return std::error_code {};
	                                       },
#endif
#ifdef MINT_ASYNC_BACKEND_EPOLL
	                                       [&operation](EPollContext& context) {
		                                       context.ready_operations.push_back(&operation);
		                                       return std::error_code {};
	                                       },
#endif
	                                       [](std::monostate) {
		                                       return std::make_error_code(std::errc::invalid_argument);
	                                       },
	                                   },
	    _context);
}

mint::AsyncOperation* mint::AsyncRuntime::poll(std::optional<std::chrono::milliseconds> timeout) {
	return std::visit<AsyncOperation*>(
	    Overloaded {
#ifdef MINT_ASYNC_BACKEND_IO_URING
	        [this, timeout](io_uring& context) -> AsyncOperation* {
		        const auto _ = std::scoped_lock(_mutex);
		        struct io_uring_cqe* cqe = nullptr;

		        const auto ret = timeout
		                             .and_then([&](auto milliseconds) -> std::optional<int> {
			                             const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
			                                 milliseconds);
			                             auto timespec = __kernel_timespec {
			                                 .tv_sec = seconds.count(),
			                                 .tv_nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(
			                                     milliseconds - seconds)
			                                     .count(),
			                             };
			                             return io_uring_wait_cqe_timeout(&context, &cqe, &timespec);
		                             })
		                             .or_else([&]() -> std::optional<int> {
			                             return io_uring_peek_cqe(&context, &cqe);
		                             })
		                             .value();

		        if (ret < 0) {
			        return nullptr;
		        }

		        auto error = std::error_code();
		        auto bytes_transferred = std::size_t();
		        auto* operation = reinterpret_cast<AsyncOperation*>(io_uring_cqe_get_data64(cqe));

		        if (cqe->res < 0) {
			        error = std::error_code(-cqe->res, std::system_category());
		        }
		        else {
			        bytes_transferred = static_cast<std::size_t>(cqe->res);
		        }

		        io_uring_cqe_seen(&context, cqe);

		        if (const auto it = _operations.find(operation); it != _operations.end()) {
			        _operations.erase(it);
			        operation->complete(error, bytes_transferred);
			        return operation;
		        }

		        return nullptr;
	        },
#endif
#ifdef MINT_ASYNC_BACKEND_EPOLL
	        [this, timeout](EPollContext& context) -> AsyncOperation* {
		        if (context.event_index == context.event_count) {
			        const auto epoll_result = epoll_wait(context.fd, context.events.data(), context.events.size(),
			            context.ready_operations.empty() ? timeout
			                                                   .transform([](std::chrono::milliseconds ms) {
				                                                   return static_cast<int>(ms.count());
			                                                   })
			                                                   .value_or(-1)
			                                             : 0);
			        if (epoll_result < 0) {
				        return nullptr;
			        }
			        context.event_count = static_cast<std::size_t>(epoll_result);
			        context.event_index = 0;
		        }
		        const auto _ = std::scoped_lock(_mutex);
		        if (context.event_count != 0) {
			        auto& event = context.events.at(context.event_index++);
			        auto* operation = static_cast<AsyncOperation*>(event.data.ptr);
			        auto* op = std::get_if<EPollOperation>(operation);
			        if (const auto it = _operations.find(operation); it != _operations.end()) {
				        epoll_ctl(context.fd, EPOLL_CTL_DEL, operation->get_handle(), nullptr);
				        if (const auto error = operation->start()) {
					        if (error.value() == EAGAIN || error.value() == EWOULDBLOCK) {
						        op->pending = true;
						        auto retry_event = epoll_event {
						            .events = op->events | EPOLLERR | EPOLLHUP,
						            .data =
						                {
						                    .ptr = operation,
						                },
						        };
						        epoll_ctl(context.fd, EPOLL_CTL_ADD, operation->get_handle(), &retry_event);
						        return nullptr;
					        }
					        _operations.erase(it);
					        operation->complete(error, 0);
					        return operation;
				        }
				        _operations.erase(it);
				        operation->complete({}, static_cast<std::size_t>(op->result));
				        return operation;
			        }
		        }
		        if (!context.ready_operations.empty()) {
			        auto* operation = context.ready_operations.front();
			        context.ready_operations.pop_front();
			        auto* op = std::get_if<EPollOperation>(operation);
			        if (const auto it = _operations.find(operation); it != _operations.end()) {
				        _operations.erase(it);
				        operation->complete({}, static_cast<std::size_t>(op->result));
				        return operation;
			        }
		        }
		        return nullptr;
	        },
#endif
	        [](std::monostate) -> AsyncOperation* {
		        return nullptr;
	        },
	    },
	    _context);
}

#else
#error "AsyncRuntime is not implemented for this platform"
#endif
