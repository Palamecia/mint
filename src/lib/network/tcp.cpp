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

#include "ip.h"
#include "mint/ast/cursor.h"
#include "mint/config.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/cast_tools.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/processor.h"
#include "mint/scheduler/scheduler.h"
#include "mint/system/async_io.h"
#include "mint/system/errno.h"
#include "socket.h"
#include <array>
#include <bit>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

#ifdef MINT_OS_WINDOWS
#include <cstdio>
#include <Windows.h>
#include <WinSock2.h>
#include <minwindef.h>
#else
#ifdef MINT_OS_LINUX
#include <linux/sockios.h>
#endif
#include <asm-generic/socket.h>
#include <memory>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif

#ifdef MINT_ASYNC_BACKEND_IO_URING
#include <liburing.h>
#endif

namespace {

mint::Reference mint_tcp_socket_open(mint::Cursor& cursor, const mint::Reference& ip_version) {

	mint::Reference result = mint::create_iterator(cursor.ast());
	auto socket_fd = INVALID_SOCKET;

	switch (to_integer<int>(cursor, ip_version)) {
	case mint_network::ip_version_4:
		socket_fd = mint_network::SocketManager::instance().open_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		break;
	case mint_network::ip_version_6:
		socket_fd = mint_network::SocketManager::instance().open_socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
		break;
	default:
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(EAFNOSUPPORT));
		return result;
	}

	if (socket_fd == INVALID_SOCKET) {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(),
		    mint::create_number(mint_network::errno_from_socket_last_error()));
	}
	else {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint_network::create_socket(cursor.ast(), socket_fd));
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
	}

	return result;
}

mint::Reference mint_tcp_socket_send(mint::FunctionHelper& helper, const mint::Reference& socket,
    mint::Reference& buffer) {

	const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
	std::vector<std::uint8_t>* buf = buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;
	auto io_status = helper.reference(mint_network::symbols::network)
	                     .member(mint_network::symbols::socket)
	                     .member(mint_network::symbols::io_status);

#ifdef MINT_OS_WINDOWS
	const auto flags = 0;
#else
	const auto flags = MSG_NOSIGNAL;
#endif

	mint::unlock_processor();
	const auto bytes_transferred = send(socket_fd, reinterpret_cast<const char*>(buf->data()),
	    static_cast<int>(buf->size()), flags);
	mint::lock_processor();

	switch (bytes_transferred) {
	case -1:
		switch (const int error = mint_network::errno_from_socket_last_error()) {
		case EINPROGRESS:
		case EWOULDBLOCK:
			mint_network::SocketManager::instance().set_socket_blocked(socket_fd, true);
			return mint::create_iterator_from(helper.cursor(),
			    io_status.member(mint_network::symbols::io_would_block).share());
		case EPIPE:
			return mint::create_iterator_from(helper.cursor(),
			    io_status.member(mint_network::symbols::io_closed).share());
		default:
			return mint::create_iterator_from(helper.cursor(),
			    io_status.member(mint_network::symbols::io_error).share(), mint::create_number(error));
		}
		break;
	case 0:
		return mint::create_iterator_from(helper.cursor(), io_status.member(mint_network::symbols::io_closed).share());
		break;
	default:
		return mint::create_iterator_from(helper.cursor(), io_status.member(mint_network::symbols::io_success).share(),
		    mint::create_signed_number(bytes_transferred));
	}

	return {};
}

mint::Reference mint_tcp_socket_send_async(mint::FunctionHelper& helper, const mint::Reference& self,
    const mint::Reference& socket, mint::Reference& buffer) {
	class AsyncSendOperation : public mint::MintAsyncOperation {
		std::reference_wrapper<mint::Cursor> _cursor;
		mint::Reference _io_status;
		std::span<std::uint8_t> _buffer;
		std::size_t _offset = 0;
#ifdef MINT_ASYNC_BACKEND_IOCP
		WSABUF _buffer_desc {};
#endif
	public:
		AsyncSendOperation(mint::FunctionHelper& helper, mint::Reference self, SOCKET socket_fd,
		    std::vector<std::uint8_t>* buffer) :
		    mint::MintAsyncOperation(std::move(self), std::bit_cast<mint::handle_t>(socket_fd)),
		    _cursor(helper.cursor()),
		    _io_status(helper.reference(mint_network::symbols::network)
		            .member(mint_network::symbols::socket)
		            .member(mint_network::symbols::io_status)
		            .get()),
		    _buffer(*buffer) {}

		std::error_code start() override {

			const auto socket_fd = reinterpret_cast<SOCKET>(get_handle());

#ifdef MINT_ASYNC_BACKEND_IOCP
			const DWORD flags = 0;
			_buffer_desc = WSABUF {
			    .len = static_cast<ULONG>(_buffer.size() - _offset),
			    .buf = reinterpret_cast<char*>(_buffer.subspan(_offset).data()),
			};

			switch (WSASend(socket_fd, &_buffer_desc, 1, nullptr, flags, this, nullptr)) {
			case SOCKET_ERROR:
				switch (WSAGetLastError()) {
				case WSA_IO_PENDING:
					break;
				default:
					return mint_network::last_socket_error_code();
				}
				break;
			default:
				break;
			}
#elifdef MINT_ASYNC_BACKEND_KQUEUE
			filter = EVFILT_WRITE;
			result = send(socket_fd, reinterpret_cast<const char*>(_buffer.subspan(_offset).data()),
			    static_cast<int>(_buffer.size() - _offset), MSG_NOSIGNAL);
			if (result < 0) {
				return mint::last_error_code();
			}
#elifdef MINT_OS_LINUX
			return std::visit(mint::Overloaded {
#ifdef MINT_ASYNC_BACKEND_IO_URING
			                      [&](mint::IoUringOperation& self) -> std::error_code {
				                      const auto flags = MSG_NOSIGNAL;
				                      io_uring_prep_send(self.sqe, socket_fd, _buffer.data(), _buffer.size(), flags);
				                      return {};
			                      },
#endif
#ifdef MINT_ASYNC_BACKEND_EPOLL
			                      [&](mint::EPollOperation& self) -> std::error_code {
				                      self.events = EPOLLOUT;
				                      self.result = send(socket_fd,
				                          reinterpret_cast<const char*>(_buffer.subspan(_offset).data()),
				                          static_cast<int>(_buffer.size() - _offset), MSG_NOSIGNAL | MSG_DONTWAIT);
				                      if (self.result < 0) {
					                      return mint::last_error_code();
				                      }

				                      return {};
			                      },
#endif
			                      [](std::monostate) -> std::error_code {
				                      return std::make_error_code(std::errc::not_supported);
			                      },
			                  },
			    *this);
#else
#error "This operation is not implemented for this platform"
#endif

			return {};
		}

		void complete(std::error_code error, std::size_t bytes_transferred) override {
			if (error) {
				done(mint::create_iterator_from(_cursor,
				    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(), mint_network::symbols::io_error),
				    mint::create_number(error.value())));
			}
			else {
				_offset += bytes_transferred;
				done(mint::create_iterator_from(_cursor,
				    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
				        mint_network::symbols::io_success),
				    mint::create_unsigned_number(bytes_transferred)));
			}
		}
	};

	return mint::create_async_operation(helper.cursor().ast(),
	    new AsyncSendOperation(helper, std::move(self), std::bit_cast<SOCKET>(mint::to_handle(socket)),
	        buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr));
}

mint::Reference mint_tcp_socket_recv(mint::FunctionHelper& helper, const mint::Reference& socket,
    mint::Reference& buffer) {

	const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
	std::vector<std::uint8_t>* buf = buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;
	auto io_status = helper.reference(mint_network::symbols::network)
	                     .member(mint_network::symbols::socket)
	                     .member(mint_network::symbols::io_status);

	socklen_t length = 0;
#ifdef MINT_OS_UNIX
	if (ioctl(socket_fd, SIOCINQ, &length) == -1) {
		return mint::create_iterator_from(helper.cursor(), io_status.member(mint_network::symbols::io_error).share(),
		    mint::create_number(errno));
	}
#else
	length = BUFSIZ; // TODO: get better value
#endif

	auto local_buffer = std::make_unique<std::uint8_t[]>(length);
	mint::unlock_processor();
	const auto bytes_transferred = recv(socket_fd, reinterpret_cast<char*>(local_buffer.get()),
	    static_cast<int>(length), 0);
	mint::lock_processor();

	switch (bytes_transferred) {
	case -1:
		switch (const int error = mint_network::errno_from_socket_last_error()) {
		case EINPROGRESS:
		case EWOULDBLOCK:
			mint_network::SocketManager::instance().set_socket_blocked(socket_fd, true);
			return mint::create_iterator_from(helper.cursor(),
			    io_status.member(mint_network::symbols::io_would_block).share());
		case EPIPE:
			return mint::create_iterator_from(helper.cursor(),
			    io_status.member(mint_network::symbols::io_closed).share());
		default:
			return mint::create_iterator_from(helper.cursor(),
			    io_status.member(mint_network::symbols::io_error).share(), mint::create_number(error));
		}
		break;
	case 0:
		return mint::create_iterator_from(helper.cursor(), io_status.member(mint_network::symbols::io_closed).share());
	default:
		buf->append_range(std::span(local_buffer.get(), bytes_transferred));
		return mint::create_iterator_from(helper.cursor(), io_status.member(mint_network::symbols::io_success).share());
	}

	return {};
}

mint::Reference mint_tcp_socket_recv_async(mint::FunctionHelper& helper, const mint::Reference& self,
    const mint::Reference& socket, mint::Reference& buffer) {
	class AsyncRecvOperation : public mint::MintAsyncOperation {
		std::reference_wrapper<mint::Cursor> _cursor;
		mint::Reference _io_status;
		std::vector<std::uint8_t>* _buf;
		std::array<std::uint8_t, BUFSIZ> _local_buffer {};
#ifdef MINT_ASYNC_BACKEND_IOCP
		WSABUF _buffer_desc {};
#endif
	public:
		AsyncRecvOperation(mint::FunctionHelper& helper, mint::Reference self, SOCKET socket_fd,
		    std::vector<std::uint8_t>* buf) :
		    mint::MintAsyncOperation(std::move(self), std::bit_cast<mint::handle_t>(socket_fd)),
		    _cursor(helper.cursor()),
		    _io_status(helper.reference(mint_network::symbols::network)
		            .member(mint_network::symbols::socket)
		            .member(mint_network::symbols::io_status)
		            .get()),
		    _buf(buf) {}

		std::error_code start() override {

			const auto socket_fd = reinterpret_cast<SOCKET>(get_handle());

#ifdef MINT_ASYNC_BACKEND_IOCP
			DWORD flags = 0;
			_buffer_desc = WSABUF {
			    .len = static_cast<ULONG>(_local_buffer.size()),
			    .buf = reinterpret_cast<char*>(_local_buffer.data()),
			};

			switch (WSARecv(socket_fd, &_buffer_desc, 1, nullptr, &flags, this, nullptr)) {
			case SOCKET_ERROR:
				switch (WSAGetLastError()) {
				case WSA_IO_PENDING:
					break;
				default:
					return mint_network::last_socket_error_code();
				}
				break;
			default:
				break;
			}
#elifdef MINT_ASYNC_BACKEND_KQUEUE
			filter = EVFILT_READ;
			result = recv(socket_fd, reinterpret_cast<char*>(_local_buffer.data()),
			    static_cast<int>(_local_buffer.size()), 0);
			if (epoll.result < 0) {
				return mint::last_error_code();
			}
#elifdef MINT_OS_LINUX
			return std::visit(mint::Overloaded {
#ifdef MINT_ASYNC_BACKEND_IO_URING
			                      [&](mint::IoUringOperation& self) -> std::error_code {
				                      const auto flags = 0;
				                      io_uring_prep_recv(self.sqe, socket_fd, _local_buffer.data(),
				                          _local_buffer.size(), flags);
				                      return {};
			                      },
#endif
#ifdef MINT_ASYNC_BACKEND_EPOLL
			                      [&](mint::EPollOperation& self) -> std::error_code {
				                      self.events = EPOLLIN;
				                      self.result = recv(socket_fd, reinterpret_cast<char*>(_local_buffer.data()),
				                          static_cast<int>(_local_buffer.size()), MSG_DONTWAIT);
				                      if (self.result < 0) {
					                      return mint::last_error_code();
				                      }

				                      return {};
			                      },
#endif
			                      [](std::monostate) -> std::error_code {
				                      return std::make_error_code(std::errc::not_supported);
			                      },
			                  },
			    *this);
#else
#error "This operation is not implemented for this platform"
#endif

			return {};
		}

		void complete(std::error_code error, std::size_t bytes_transferred) override {
			if (error) {
				done(mint::create_iterator_from(_cursor,
				    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(), mint_network::symbols::io_error),
				    mint::create_number(error.value())));
			}
			else if (bytes_transferred == 0) {
				done(mint::create_iterator_from(_cursor,
				    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
				        mint_network::symbols::io_closed)));
			}
			else {
				_buf->append_range(std::span(_local_buffer.data(), bytes_transferred));
				done(mint::create_iterator_from(_cursor,
				    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
				        mint_network::symbols::io_success)));
			}
		}
	};

	return mint::create_async_operation(helper.cursor().ast(),
	    new AsyncRecvOperation(helper, std::move(self), std::bit_cast<SOCKET>(mint::to_handle(socket)),
	        buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr));
}

mint::Reference mint_tcp_socket_recv_some(mint::FunctionHelper& helper, const mint::Reference& socket,
    mint::Reference& buffer, mint::Reference& count) {

	const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
	std::vector<std::uint8_t>* buf = buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;
	auto io_status = helper.reference(mint_network::symbols::network)
	                     .member(mint_network::symbols::socket)
	                     .member(mint_network::symbols::io_status);

	const auto length = mint::to_integer<socklen_t>(helper.cursor(), count);

	auto local_buffer = std::make_unique<std::uint8_t[]>(length);
	mint::unlock_processor();
	const auto bytes_transferred = recv(socket_fd, reinterpret_cast<char*>(local_buffer.get()),
	    static_cast<int>(length), 0);
	mint::lock_processor();

	switch (bytes_transferred) {
	case -1:
		switch (const int error = mint_network::errno_from_socket_last_error()) {
		case EINPROGRESS:
		case EWOULDBLOCK:
			mint_network::SocketManager::instance().set_socket_blocked(socket_fd, true);
			return mint::create_iterator_from(helper.cursor(),
			    io_status.member(mint_network::symbols::io_would_block).share());
		case EPIPE:
			return mint::create_iterator_from(helper.cursor(),
			    io_status.member(mint_network::symbols::io_closed).share());
		default:
			return mint::create_iterator_from(helper.cursor(),
			    io_status.member(mint_network::symbols::io_error).share(), mint::create_number(error));
			break;
		}
		break;
	case 0:
		return mint::create_iterator_from(helper.cursor(), io_status.member(mint_network::symbols::io_closed).share());
	default:
		buf->append_range(std::span(local_buffer.get(), bytes_transferred));
		return mint::create_iterator_from(helper.cursor(), io_status.member(mint_network::symbols::io_success).share());
	}

	return {};
}

mint::Reference mint_tcp_socket_recv_some_async(mint::FunctionHelper& helper, const mint::Reference& self,
    const mint::Reference& socket, mint::Reference& buffer, mint::Reference& count) {
	class AsyncRecvSomeOperation : public mint::MintAsyncOperation {
		std::reference_wrapper<mint::Cursor> _cursor;
		mint::Reference _io_status;
		std::vector<std::uint8_t>* _buf;
		std::unique_ptr<std::uint8_t[]> _local_buffer;
		socklen_t _local_buffer_length;
#ifdef MINT_ASYNC_BACKEND_IOCP
		WSABUF _buffer_desc {};
#endif
	public:
		AsyncRecvSomeOperation(mint::FunctionHelper& helper, mint::Reference self, SOCKET socket_fd,
		    std::vector<std::uint8_t>* buf, socklen_t buffer_length) :
		    mint::MintAsyncOperation(std::move(self), std::bit_cast<mint::handle_t>(socket_fd)),
		    _cursor(helper.cursor()),
		    _io_status(helper.reference(mint_network::symbols::network)
		            .member(mint_network::symbols::socket)
		            .member(mint_network::symbols::io_status)
		            .get()),
		    _buf(buf),
		    _local_buffer(std::make_unique<std::uint8_t[]>(buffer_length)),
		    _local_buffer_length(buffer_length) {}

		std::error_code start() override {

			const auto socket_fd = reinterpret_cast<SOCKET>(get_handle());

#ifdef MINT_ASYNC_BACKEND_IOCP
			DWORD flags = 0;
			_buffer_desc = WSABUF {
			    .len = static_cast<ULONG>(_local_buffer_length),
			    .buf = reinterpret_cast<char*>(_local_buffer.get()),
			};

			switch (WSARecv(socket_fd, &_buffer_desc, 1, nullptr, &flags, this, nullptr)) {
			case SOCKET_ERROR:
				switch (WSAGetLastError()) {
				case WSA_IO_PENDING:
					break;
				default:
					return mint_network::last_socket_error_code();
				}
				break;
			default:
				break;
			}
#elifdef MINT_ASYNC_BACKEND_KQUEUE
			filter = EVFILT_READ;
			result = recv(socket_fd, reinterpret_cast<char*>(_local_buffer.get()),
			    static_cast<int>(_local_buffer_length), 0);
			if (result < 0) {
				return mint::last_error_code();
			}
#elifdef MINT_OS_LINUX
			return std::visit(mint::Overloaded {
#ifdef MINT_ASYNC_BACKEND_IO_URING
			                      [&](mint::IoUringOperation& self) -> std::error_code {
				                      const auto flags = 0;
				                      io_uring_prep_recv(self.sqe, socket_fd, _local_buffer.get(), _local_buffer_length,
				                          flags);
				                      return {};
			                      },
#endif
#ifdef MINT_ASYNC_BACKEND_EPOLL
			                      [&](mint::EPollOperation& self) -> std::error_code {
				                      self.events = EPOLLIN;
				                      self.result = recv(socket_fd, reinterpret_cast<char*>(_local_buffer.get()),
				                          static_cast<int>(_local_buffer_length), MSG_DONTWAIT);
				                      if (self.result < 0) {
					                      return mint::last_error_code();
				                      }

				                      return {};
			                      },
#endif
			                      [](std::monostate) -> std::error_code {
				                      return std::make_error_code(std::errc::not_supported);
			                      },
			                  },
			    *this);
#else
#error "This operation is not implemented for this platform"
#endif

			return {};
		}

		void complete(std::error_code error, std::size_t bytes_transferred) override {
			if (error) {
				done(mint::create_iterator_from(_cursor,
				    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(), mint_network::symbols::io_error),
				    mint::create_number(error.value())));
			}
			else if (bytes_transferred == 0) {
				done(mint::create_iterator_from(_cursor,
				    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
				        mint_network::symbols::io_closed)));
			}
			else {
				_buf->append_range(std::span(_local_buffer.get(), bytes_transferred));
				done(mint::create_iterator_from(_cursor,
				    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
				        mint_network::symbols::io_success)));
			}
		}
	};

	return mint::create_async_operation(helper.cursor().ast(),
	    new AsyncRecvSomeOperation(helper, std::move(self), std::bit_cast<SOCKET>(mint::to_handle(socket)),
	        buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr,
	        mint::to_integer<socklen_t>(helper.cursor(), count)));
}

mint::Reference mint_tcp_socket_setup_options(mint::Cursor& /*cursor*/, const mint::Reference& tcp_socket_option) {

#define BIND_TCP_VALUE(_enum, _option) \
	_enum.data<mint::Object>().metadata.find_global(#_option)->value.data<mint::Number>().value = TCP_##_option
#define BIND_TCP_DISABLE(_enum, _option) \
	_enum.data<mint::Object>().metadata.find_global(#_option)->value.move_data(mint::create_none())

#ifdef TCP_MAXSEG
	BIND_TCP_VALUE(tcp_socket_option, MAXSEG);
#else
	BIND_TCP_DISABLE(tcp_socket_option, MAXSEG);
#endif
#ifdef TCP_NODELAY
	BIND_TCP_VALUE(tcp_socket_option, NODELAY);
#else
	BIND_TCP_DISABLE(tcp_socket_option, NODELAY);
#endif

	return {};
}

mint::Reference mint_tcp_socket_get_option_number(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::Reference result = mint::create_iterator(cursor.ast());

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = to_integer<int>(cursor, option);
		const auto option_value = mint_network::get_socket_option<int>(socket_fd, IPPROTO_TCP, option_id);
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(option_value));
	}
	catch (const std::system_error& error) {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(error.code().value()));
	}

	return result;
}

mint::Reference mint_tcp_socket_set_option_number(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = to_integer<int>(cursor, option);
		const auto option_value = to_integer<int>(cursor, value);
		mint_network::set_socket_option(socket_fd, IPPROTO_TCP, option_id, option_value);
	}
	catch (const std::system_error& error) {
		return mint::create_number(error.code().value());
	}

	return {};
}

mint::Reference mint_tcp_socket_get_option_boolean(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::Reference result = mint::create_iterator(cursor.ast());

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = to_integer<int>(cursor, option);
		const auto option_value = mint_network::get_socket_option<mint_network::sockopt_bool>(socket_fd, IPPROTO_TCP,
		    option_id);
		iterator_yield(cursor, result.data<mint::Iterator>(),
		    mint::create_boolean(option_value != mint_network::sockopt_false));
	}
	catch (const std::system_error& error) {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(error.code().value()));
	}

	return result;
}

mint::Reference mint_tcp_socket_set_option_boolean(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = to_integer<int>(cursor, option);
		const auto option_value = to_boolean(value) ? mint_network::sockopt_true : mint_network::sockopt_false;
		mint_network::set_socket_option(socket_fd, IPPROTO_TCP, option_id, option_value);
	}
	catch (const std::system_error& error) {
		return mint::create_number(error.code().value());
	}

	return {};
}

}

MINT_EXPORT_FUNCTION(mint_tcp_socket_open, 1);
MINT_EXPORT_FUNCTION(mint_tcp_socket_send, 2);
MINT_EXPORT_FUNCTION(mint_tcp_socket_send_async, 3);
MINT_EXPORT_FUNCTION(mint_tcp_socket_recv, 2);
MINT_EXPORT_FUNCTION(mint_tcp_socket_recv_async, 3);
MINT_EXPORT_FUNCTION(mint_tcp_socket_recv_some, 3);
MINT_EXPORT_FUNCTION(mint_tcp_socket_recv_some_async, 4);

MINT_EXPORT_FUNCTION(mint_tcp_socket_setup_options, 1);
MINT_EXPORT_FUNCTION(mint_tcp_socket_get_option_number, 2);
MINT_EXPORT_FUNCTION(mint_tcp_socket_set_option_number, 3);
MINT_EXPORT_FUNCTION(mint_tcp_socket_get_option_boolean, 2);
MINT_EXPORT_FUNCTION(mint_tcp_socket_set_option_boolean, 3);
