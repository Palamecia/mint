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

#include "mint/config.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/cast_tools.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/processor.h"
#include "mint/system/async_io.h"
#include "mint/system/errno.h"
#include "socket.h"
#include "ip.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iterator>
#include <memory>
#include <span>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

#ifdef MINT_OS_WINDOWS
#include <cstdio>
#include <Windows.h>
#include <in6addr.h>
#include <inaddr.h>
#include <winsock.h>
#include <WinSock2.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>
#else
#ifdef MINT_OS_LINUX
#include <linux/sockios.h>
#endif
#include <arpa/inet.h>
#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef MINT_ASYNC_BACKEND_IO_URING
#include <bits/types/struct_iovec.h>
#include <liburing.h>
#endif

namespace {

inline constexpr std::size_t max_udp_payload = 65507;

mint::Reference mint_udp_socket_open(mint::Cursor& cursor, const mint::Reference& ip_version) {

	mint::Reference result = mint::create_iterator(cursor.ast());
	auto socket_fd = INVALID_SOCKET;

	switch (mint::to_integer<int>(cursor, ip_version)) {
	case mint_network::ip_version_4:
		socket_fd = mint_network::SocketManager::instance().open_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		break;
	case mint_network::ip_version_6:
		socket_fd = mint_network::SocketManager::instance().open_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
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

mint::Reference mint_udp_socket_sendto(mint::FunctionHelper& helper, const mint::Reference& socket,
    mint::Reference& endpoint, const mint::Reference& buffer) {

	auto io_status = helper.reference(mint_network::symbols::network)
	                     .member(mint_network::symbols::socket)
	                     .member(mint_network::symbols::io_status);

	mint::Reference result = mint::create_iterator(helper.cursor().ast());

	try {
		std::vector<std::uint8_t>* buf = buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto [address, address_length] = mint_network::to_sockaddr(endpoint);

#ifdef MINT_OS_WINDOWS
		const auto flags = 0;
#else
		const auto flags = MSG_CONFIRM;
#endif

		mint::unlock_processor();
		const auto count = sendto(socket_fd, reinterpret_cast<const char*>(buf->data()), static_cast<int>(buf->size()),
		    flags, address, address_length);
		mint::lock_processor();

		switch (count) {
		case -1:
			switch (const int error = mint_network::errno_from_socket_last_error()) {
			case EINPROGRESS:
			case EWOULDBLOCK:
				iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
				    io_status.member(mint_network::symbols::io_would_block).share());
				mint_network::SocketManager::instance().set_socket_blocked(socket_fd, true);
				break;

			case EPIPE:
				iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
				    io_status.member(mint_network::symbols::io_closed).share());
				break;

			default:
				iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
				    io_status.member(mint_network::symbols::io_error).share());
				iterator_yield(helper.cursor(), result.data<mint::Iterator>(), mint::create_number(error));
				break;
			}
			break;
		case 0:
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
			    io_status.member(mint_network::symbols::io_closed).share());
			break;
		default:
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
			    io_status.member(mint_network::symbols::io_success).share());
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(), mint::create_signed_number(count));
			break;
		}

		return result;
	}
	catch (const std::system_error& error) {
		iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
		    io_status.member(mint_network::symbols::io_error).share());
		iterator_yield(helper.cursor(), result.data<mint::Iterator>(), mint::create_number(error.code().value()));
		return result;
	}
}

mint::Reference mint_udp_socket_sendto_async(mint::FunctionHelper& helper, const mint::Reference& self,
    const mint::Reference& socket, mint::Reference& endpoint, const mint::Reference& buffer) {

	class AsyncSendToOperation : public mint::MintAsyncOperation {
		std::reference_wrapper<mint::Cursor> _cursor;
		mint::Reference _io_status;
		sockaddr* _remote_address;
		socklen_t _remote_address_length;
		std::vector<std::uint8_t>* _buf;

#ifdef MINT_ASYNC_BACKEND_IOCP
		WSABUF _buffer_desc {};
#elifdef MINT_ASYNC_BACKEND_IO_URING
		iovec _iov {};
		msghdr _msg {};
#endif

	public:
		AsyncSendToOperation(mint::FunctionHelper& helper, mint::Reference self, SOCKET socket_fd, sockaddr* address,
		    socklen_t address_length, std::vector<std::uint8_t>* buf) :
		    mint::MintAsyncOperation(std::move(self), std::bit_cast<mint::handle_t>(socket_fd)),
		    _cursor(helper.cursor()),
		    _io_status(helper.reference(mint_network::symbols::network)
		            .member(mint_network::symbols::socket)
		            .member(mint_network::symbols::io_status)
		            .get()),
		    _remote_address(address),
		    _remote_address_length(address_length),
		    _buf(buf) {}

		std::error_code start() override {

			const auto socket_fd = reinterpret_cast<SOCKET>(get_handle());

#ifdef MINT_ASYNC_BACKEND_IOCP
			const DWORD flags = 0;
			_buffer_desc = WSABUF {
			    .len = static_cast<ULONG>(_buf->size()),
			    .buf = reinterpret_cast<char*>(_buf->data()),
			};

			switch (WSASendTo(socket_fd, &_buffer_desc, 1, nullptr, flags, _remote_address, _remote_address_length,
			    this, nullptr)) {
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
			result = sendmsg(socket_fd, &_msg, 0);
			if (result < 0) {
				return mint::last_error_code();
			}
#elifdef MINT_OS_LINUX
			return std::visit(mint::Overloaded {
#ifdef MINT_ASYNC_BACKEND_IO_URING
			                      [&](mint::IoUringOperation& self) -> std::error_code {
				                      _iov = iovec {
				                          .iov_base = _buf->data(),
				                          .iov_len = _buf->size(),
				                      };
				                      _msg = msghdr {
				                          .msg_name = _remote_address,
				                          .msg_namelen = _remote_address_length,
				                          .msg_iov = &_iov,
				                          .msg_iovlen = 1,
				                      };
				                      io_uring_prep_sendmsg(self.sqe, socket_fd, &_msg, 0);
				                      return {};
			                      },
#endif
#ifdef MINT_ASYNC_BACKEND_EPOLL
			                      [&](mint::EPollOperation& self) -> std::error_code {
				                      self.events = EPOLLOUT;
				                      self.result = ::sendto(socket_fd, reinterpret_cast<const char*>(_buf->data()),
				                          static_cast<int>(_buf->size()), MSG_DONTWAIT, _remote_address,
				                          _remote_address_length);
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
				done(mint::create_iterator_from(_cursor,
				    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
				        mint_network::symbols::io_success),
				    mint::create_unsigned_number(bytes_transferred)));
			}
		}
	};

	try {

		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto [address, address_length] = mint_network::to_sockaddr(endpoint);

		return mint::create_iterator_from(helper.cursor(), mint::create_number(0),
		    mint::create_async_operation(helper.cursor().ast(),
		        new AsyncSendToOperation(helper, std::move(self), socket_fd, address, address_length,
		            buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr)));
	}
	catch (const std::system_error& error) {
		return mint::create_iterator_from(helper.cursor(), mint::create_number(error.code().value()));
	}
}

mint::Reference mint_udp_socket_recvfrom(mint::FunctionHelper& helper, const mint::Reference& socket,
    mint::Reference& buffer) {

	mint::Reference result = mint::create_iterator(helper.cursor().ast());

	const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
	std::vector<std::uint8_t>* buf = buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;
	auto io_status = helper.reference(mint_network::symbols::network)
	                     .member(mint_network::symbols::socket)
	                     .member(mint_network::symbols::io_status);

	sockaddr source_address {};
	socklen_t source_address_length = sizeof(source_address);

	socklen_t length = 0;
#ifdef MINT_OS_UNIX
	if (ioctl(socket_fd, SIOCINQ, &length) == -1) {
		iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
		    io_status.member(mint_network::symbols::io_error).share());
		iterator_yield(helper.cursor(), result.data<mint::Iterator>(), mint::create_number(errno));
		return result;
	}
#else
	length = BUFSIZ; /// @todo get better value
#endif

	const auto flags = 0; // MSG_WAITALL;
	auto local_buffer = std::make_unique<std::uint8_t[]>(length);
	mint::unlock_processor();
	auto count = recvfrom(socket_fd, reinterpret_cast<char*>(local_buffer.get()), static_cast<int>(length), flags,
	    &source_address, &source_address_length);
	mint::lock_processor();

	switch (count) {
	case -1:
		switch (const int error = mint_network::errno_from_socket_last_error()) {
		case EINPROGRESS:
		case EWOULDBLOCK:
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
			    io_status.member(mint_network::symbols::io_would_block).share());
			mint_network::SocketManager::instance().set_socket_blocked(socket_fd, true);
			break;

		case EPIPE:
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
			    io_status.member(mint_network::symbols::io_closed).share());
			break;

		default:
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
			    io_status.member(mint_network::symbols::io_error).share());
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(), mint::create_none());
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(), mint::create_none());
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(), mint::create_number(error));
			break;
		}
		break;
	case 0:
		iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
		    io_status.member(mint_network::symbols::io_closed).share());
		break;
	default:
		try {
			const auto [address, port] = mint_network::get_ip_socket_info(source_address);
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
			    io_status.member(mint_network::symbols::io_success).share());
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
			    mint::create_string(helper.cursor().ast(), address));
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(), mint::create_number(port));
			std::copy_n(local_buffer.get(), count, std::back_inserter(*buf));
		}
		catch (const std::system_error& error) {
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
			    io_status.member(mint_network::symbols::io_error).share());
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(), mint::create_none());
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(), mint::create_none());
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(), mint::create_number(error.code().value()));
		}
		break;
	}

	return result;
}

mint::Reference mint_udp_socket_recvfrom_async(mint::FunctionHelper& helper, const mint::Reference& self,
    const mint::Reference& socket, mint::Reference& buffer) {

	class AsyncRecvFromOperation : public mint::MintAsyncOperation {
		std::reference_wrapper<mint::Cursor> _cursor;
		mint::Reference _io_status;
		sockaddr_storage _source_address;
		socklen_t _source_address_length = sizeof(sockaddr_storage);
		std::vector<std::uint8_t>* _buf;
		std::array<std::uint8_t, max_udp_payload> _local_buffer {};
#ifdef MINT_ASYNC_BACKEND_IOCP
		WSABUF _buffer_desc {};
#elifdef MINT_ASYNC_BACKEND_IO_URING
		iovec _iov {};
		msghdr _msg {};
#endif

	public:
		AsyncRecvFromOperation(mint::FunctionHelper& helper, mint::Reference self, SOCKET socket_fd,
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
			DWORD bytes_transferred = 0;
			DWORD flags = 0;
			_buffer_desc = WSABUF {
			    .len = static_cast<ULONG>(_local_buffer.size()),
			    .buf = reinterpret_cast<char*>(_local_buffer.data()),
			};

			switch (WSARecvFrom(socket_fd, &_buffer_desc, 1, &bytes_transferred, &flags,
			    reinterpret_cast<sockaddr*>(&_source_address), &_source_address_length, this, nullptr)) {
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
			result = recvmsg(socket_fd, &_msg, 0);
			if (result < 0) {
				return mint::last_error_code();
			}
#elifdef MINT_OS_LINUX
			return std::visit(mint::Overloaded {
#ifdef MINT_ASYNC_BACKEND_IO_URING
			                      [&](mint::IoUringOperation& self) -> std::error_code {
				                      _iov = iovec {
				                          .iov_base = _local_buffer.data(),
				                          .iov_len = _local_buffer.size(),
				                      };
				                      _msg = msghdr {
				                          .msg_name = reinterpret_cast<sockaddr*>(&_source_address),
				                          .msg_namelen = _source_address_length,
				                          .msg_iov = &_iov,
				                          .msg_iovlen = 1,
				                      };
				                      io_uring_prep_recvmsg(self.sqe, socket_fd, &_msg, 0);
				                      return {};
			                      },
#endif
#ifdef MINT_ASYNC_BACKEND_EPOLL
			                      [&](mint::EPollOperation& self) -> std::error_code {
				                      self.events = EPOLLIN;
				                      self.result = ::recvfrom(socket_fd, reinterpret_cast<char*>(_local_buffer.data()),
				                          static_cast<int>(_local_buffer.size()), MSG_DONTWAIT,
				                          reinterpret_cast<sockaddr*>(&_source_address), &_source_address_length);
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
				    mint::create_none(), mint::create_none(), mint::create_number(error.value())));
			}
			else {
				try {
					const auto [address, port] = mint_network::get_ip_socket_info(
					    *reinterpret_cast<sockaddr*>(&_source_address));
					_buf->append_range(std::span(_local_buffer.data(), bytes_transferred));
					done(mint::create_iterator_from(_cursor,
					    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
					        mint_network::symbols::io_success),
					    mint::create_string(_cursor.get().ast(), address), mint::create_number(port)));
				}
				catch (const std::system_error& error) {
					done(mint::create_iterator_from(_cursor,
					    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
					        mint_network::symbols::io_error),
					    mint::create_none(), mint::create_none(), mint::create_number(error.code().value())));
				}
			}
		}
	};

	return mint::create_async_operation(helper.cursor().ast(),
	    new AsyncRecvFromOperation(helper, std::move(self), std::bit_cast<SOCKET>(mint::to_handle(socket)),
	        buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr));
}

mint::Reference mint_udp_socket_send(mint::FunctionHelper& helper, const mint::Reference& socket,
    mint::Reference& buffer) {

	mint::Reference result = mint::create_iterator(helper.cursor().ast());

	const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
	std::vector<std::uint8_t>* buf = buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;
	auto io_status = helper.reference(mint_network::symbols::network)
	                     .member(mint_network::symbols::socket)
	                     .member(mint_network::symbols::io_status);

#ifdef MINT_OS_WINDOWS
	const auto flags = 0;
#else
	const auto flags = MSG_CONFIRM;
#endif

	mint::unlock_processor();
	const auto count = send(socket_fd, reinterpret_cast<const char*>(buf->data()), static_cast<int>(buf->size()), flags);
	mint::lock_processor();

	switch (count) {
	case -1:
		switch (const int error = mint_network::errno_from_socket_last_error()) {
		case EINPROGRESS:
		case EWOULDBLOCK:
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
			    io_status.member(mint_network::symbols::io_would_block).share());
			mint_network::SocketManager::instance().set_socket_blocked(socket_fd, true);
			break;

		case EPIPE:
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
			    io_status.member(mint_network::symbols::io_closed).share());
			break;

		default:
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
			    io_status.member(mint_network::symbols::io_error).share());
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(), mint::create_number(error));
			break;
		}
		break;
	case 0:
		iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
		    io_status.member(mint_network::symbols::io_closed).share());
		break;
	default:
		iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
		    io_status.member(mint_network::symbols::io_success).share());
		iterator_yield(helper.cursor(), result.data<mint::Iterator>(), mint::create_signed_number(count));
		break;
	}

	return result;
}

mint::Reference mint_udp_socket_send_async(mint::FunctionHelper& helper, const mint::Reference& self,
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
			DWORD bytes_transferred = 0;
			const DWORD flags = 0;
			_buffer_desc = WSABUF {
			    .len = static_cast<ULONG>(_buffer.size() - _offset),
			    .buf = reinterpret_cast<char*>(_buffer.subspan(_offset).data()),
			};

			switch (WSASend(socket_fd, &_buffer_desc, 1, &bytes_transferred, flags, this, nullptr)) {
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
			result = send(socket_fd, reinterpret_cast<const char*>(_buffer.data()), static_cast<int>(_buffer.size()),
			    MSG_CONFIRM);
			if (result < 0) {
				return mint::last_error_code();
			}
#elifdef MINT_OS_LINUX
			return std::visit(mint::Overloaded {
#ifdef MINT_ASYNC_BACKEND_IO_URING
			                      [&](mint::IoUringOperation& self) -> std::error_code {
				                      const auto flags = MSG_CONFIRM;
				                      io_uring_prep_send(self.sqe, socket_fd, _buffer.data(), _buffer.size(), flags);
				                      return {};
			                      },
#endif
#ifdef MINT_ASYNC_BACKEND_EPOLL
			                      [&](mint::EPollOperation& self) -> std::error_code {
				                      self.events = EPOLLOUT;
				                      self.result = send(socket_fd, reinterpret_cast<const char*>(_buffer.data()),
				                          static_cast<int>(_buffer.size()), MSG_NOSIGNAL | MSG_DONTWAIT);
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

mint::Reference mint_udp_socket_recv(mint::FunctionHelper& helper, const mint::Reference& socket,
    mint::Reference& buffer) {

	mint::Reference result = create_iterator(helper.cursor().ast());

	const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
	std::vector<std::uint8_t>* buf = buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;
	auto io_status = helper.reference(mint_network::symbols::network)
	                     .member(mint_network::symbols::socket)
	                     .member(mint_network::symbols::io_status);

	socklen_t length = 0;
#ifdef MINT_OS_UNIX
	if (ioctl(socket_fd, SIOCINQ, &length) == -1) {
		iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
		    io_status.member(mint_network::symbols::io_error).share());
		iterator_yield(helper.cursor(), result.data<mint::Iterator>(), mint::create_number(errno));
		return result;
	}
#else
	length = BUFSIZ; /// @todo get better value
#endif

	const auto flags = MSG_WAITALL;
	auto local_buffer = std::make_unique<std::uint8_t[]>(length);
	mint::unlock_processor();
	auto count = recv(socket_fd, reinterpret_cast<char*>(local_buffer.get()), static_cast<int>(length), flags);
	mint::lock_processor();

	switch (count) {
	case -1:
		switch (const int error = mint_network::errno_from_socket_last_error()) {
		case EINPROGRESS:
		case EWOULDBLOCK:
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
			    io_status.member(mint_network::symbols::io_would_block).share());
			mint_network::SocketManager::instance().set_socket_blocked(socket_fd, true);
			break;

		case EPIPE:
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
			    io_status.member(mint_network::symbols::io_closed).share());
			break;

		default:
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
			    io_status.member(mint_network::symbols::io_error).share());
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(), mint::create_number(error));
			break;
		}
		break;
	case 0:
		iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
		    io_status.member(mint_network::symbols::io_closed).share());
		break;
	default:
		iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
		    io_status.member(mint_network::symbols::io_success).share());
		std::copy_n(local_buffer.get(), count, std::back_inserter(*buf));
		break;
	}

	return result;
}

mint::Reference mint_udp_socket_recv_async(mint::FunctionHelper& helper, const mint::Reference& self,
    const mint::Reference& socket, mint::Reference& buffer) {
	class AsyncRecvOperation : public mint::MintAsyncOperation {
		std::reference_wrapper<mint::Cursor> _cursor;
		mint::Reference _io_status;
		std::vector<std::uint8_t>* _buf;
		std::array<std::uint8_t, max_udp_payload> _local_buffer {};
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
			DWORD flags = MSG_WAITALL;
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
			if (result < 0) {
				return mint::last_error_code();
			}
#elifdef MINT_OS_LINUX
			return std::visit(mint::Overloaded {
#ifdef MINT_ASYNC_BACKEND_IO_URING
			                      [&](mint::IoUringOperation& self) -> std::error_code {
				                      const auto flags = MSG_WAITALL;
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

}

MINT_EXPORT_FUNCTION(mint_udp_socket_open, 1);
MINT_EXPORT_FUNCTION(mint_udp_socket_sendto, 3);
MINT_EXPORT_FUNCTION(mint_udp_socket_sendto_async, 4);
MINT_EXPORT_FUNCTION(mint_udp_socket_recvfrom, 2);
MINT_EXPORT_FUNCTION(mint_udp_socket_recvfrom_async, 3);
MINT_EXPORT_FUNCTION(mint_udp_socket_send, 2);
MINT_EXPORT_FUNCTION(mint_udp_socket_send_async, 3);
MINT_EXPORT_FUNCTION(mint_udp_socket_recv, 2);
MINT_EXPORT_FUNCTION(mint_udp_socket_recv_async, 3);
