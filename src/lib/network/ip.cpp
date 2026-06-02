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
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/processor.h"
#include "mint/system/async_io.h"
#include "mint/system/errno.h"
#include "socket.h"
#include "ip.h"
#include <array>
#include <bit>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <system_error>
#include <tuple>
#include <utility>
#include <variant>

#ifdef MINT_OS_WINDOWS
#include <WinSock2.h>
#include <MSWSock.h>
#include <ws2def.h>
#include <ws2ipdef.h>
#include <WS2tcpip.h>
#include <inaddr.h>
#else
#ifdef MINT_OS_LINUX
#include <linux/ipv6.h>
#include <linux/in6.h>
#endif
#ifdef MINT_ASYNC_BACKEND_EPOLL
#include <sys/epoll.h>
#endif
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef MINT_ASYNC_BACKEND_IO_URING
#include <liburing.h>
#endif

std::tuple<std::string, u_short> mint_network::get_ip_socket_info(const sockaddr& endpoint) {
	switch (endpoint.sa_family) {
	case AF_INET:
		{
			auto buffer = std::array<char, INET_ADDRSTRLEN>();
			const auto* addr = reinterpret_cast<const sockaddr_in*>(&endpoint);
			if (const char* address = inet_ntop(endpoint.sa_family, &addr->sin_addr, buffer.data(), buffer.size())) {
				return {address, htons(addr->sin_port)};
			}
			throw std::system_error(last_socket_error_code());
		}
		break;
	case AF_INET6:
		{
			auto buffer = std::array<char, INET6_ADDRSTRLEN>();
			const auto* addr = reinterpret_cast<const sockaddr_in6*>(&endpoint);
			if (const char* address = inet_ntop(endpoint.sa_family, &addr->sin6_addr, buffer.data(), buffer.size())) {
				return {address, htons(addr->sin6_port)};
			}
			throw std::system_error(last_socket_error_code());
		}
		break;
	default:
		throw std::system_error(std::make_error_code(std::errc::address_family_not_supported));
	}
}

namespace {

mint::Reference mint_ip_endpoint_create(mint::Cursor& cursor, const mint::Reference& ip_version,
    const mint::Reference& address, const mint::Reference& port) {
	const std::string address_str = to_string(address);
	switch (to_integer<int>(cursor, ip_version)) {
	case mint_network::ip_version_4:
		{
			auto d_ptr = std::make_unique<sockaddr_in>(sockaddr_in {
			    .sin_family = AF_INET,
			});
			d_ptr->sin_port = htons(to_integer<std::uint16_t>(cursor, port));
			if (::inet_pton(AF_INET, address_str.c_str(), &d_ptr->sin_addr.s_addr) == 1) {
				return mint::create_c_object<sockaddr>(cursor.ast(), reinterpret_cast<sockaddr*>(d_ptr.release()));
			}
		}
		break;
	case mint_network::ip_version_6:
		{
			auto d_ptr = std::make_unique<sockaddr_in6>(sockaddr_in6 {
			    .sin6_family = AF_INET6,
			});
			d_ptr->sin6_port = htons(to_integer<std::uint16_t>(cursor, port));
			if (::inet_pton(AF_INET6, address_str.c_str(), &d_ptr->sin6_addr.s6_addr) == 1) {
				return mint::create_c_object<sockaddr>(cursor.ast(), reinterpret_cast<sockaddr*>(d_ptr.release()));
			}
		}
		break;
	default:
		break;
	}
	return {};
}

mint::Reference mint_ip_endpoint_delete(mint::Cursor& /*cursor*/, const mint::Reference& self) {
	switch (self.data<mint::LibObject<sockaddr>>().ptr->sa_family) {
	case AF_INET:
		delete reinterpret_cast<sockaddr_in*>(self.data<mint::LibObject<sockaddr>>().ptr);
		break;
	case AF_INET6:
		delete reinterpret_cast<sockaddr_in6*>(self.data<mint::LibObject<sockaddr>>().ptr);
		break;
	default:
		delete self.data<mint::LibObject<sockaddr>>().ptr;
		break;
	}
	return {};
}

mint::Reference mint_ip_endpoint_get_version(mint::Cursor& /*cursor*/, const mint::Reference& self) {
	switch (self.data<mint::LibObject<sockaddr>>().ptr->sa_family) {
	case AF_INET:
		return mint::create_number(mint_network::ip_version_4);
	case AF_INET6:
		return mint::create_number(mint_network::ip_version_6);
	default:
		return {};
	}
}

mint::Reference mint_ip_endpoint_get_address(mint::Cursor& cursor, const mint::Reference& self) {
	switch (self.data<mint::LibObject<sockaddr>>().ptr->sa_family) {
	case AF_INET:
		{
			auto buffer = std::array<char, INET_ADDRSTRLEN>();
			return mint::create_string(cursor.ast(),
			    inet_ntop(AF_INET, &self.data<mint::LibObject<sockaddr_in>>().ptr->sin_addr, buffer.data(),
			        buffer.size()));
		}
	case AF_INET6:
		{
			auto buffer = std::array<char, INET6_ADDRSTRLEN>();
			return mint::create_string(cursor.ast(),
			    inet_ntop(AF_INET6, &self.data<mint::LibObject<sockaddr_in6>>().ptr->sin6_addr, buffer.data(),
			        buffer.size()));
		}
	default:
		return {};
	}
}

mint::Reference mint_ip_endpoint_get_port(mint::Cursor& /*cursor*/, const mint::Reference& self) {
	switch (self.data<mint::LibObject<sockaddr>>().ptr->sa_family) {
	case AF_INET:
		return mint::create_number(ntohs(self.data<mint::LibObject<sockaddr_in>>().ptr->sin_port));
	case AF_INET6:
		return mint::create_number(ntohs(self.data<mint::LibObject<sockaddr_in6>>().ptr->sin6_port));
	default:
		return {};
	}
}

mint::Reference mint_ip_socket_accept(mint::Cursor& cursor, const mint::Reference& socket) {

	sockaddr remote_address {};
	socklen_t remote_address_length = sizeof(remote_address);
	const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));

	mint::unlock_processor();
	const SOCKET client_fd = ::accept(socket_fd, &remote_address, &remote_address_length);
	mint::lock_processor();

	if (client_fd != INVALID_SOCKET) {
		try {
			const auto [address, port] = mint_network::get_ip_socket_info(remote_address);
			mint_network::SocketManager::instance().accept_socket(client_fd);
			return mint::create_iterator_from(cursor, mint::create_number(0),
			    mint_network::create_socket(cursor.ast(), client_fd), mint::create_string(cursor.ast(), address),
			    mint::create_number(port));
		}
		catch (const std::system_error& error) {
			return mint::create_iterator_from(cursor, mint::create_number(error.code().value()));
		}
	}

	switch (const int error = mint_network::errno_from_socket_last_error()) {
	case EINPROGRESS:
	case EWOULDBLOCK:
		mint_network::SocketManager::instance().set_socket_blocked(socket_fd, true);
		break;
	default:
		return mint::create_iterator_from(cursor, mint::create_number(error));
	}

	return {};
}

mint::Reference mint_ip_socket_accept_async(mint::Cursor& cursor, const mint::Reference& self,
    const mint::Reference& socket) {

#ifdef MINT_ASYNC_BACKEND_IOCP
	static constexpr auto address_length = sizeof(sockaddr_storage) + 16;
#endif

	class AsyncAcceptOperation : public mint::MintAsyncOperation {
		std::reference_wrapper<mint::Cursor> _cursor;
#ifdef MINT_ASYNC_BACKEND_IOCP
		SOCKET _client_fd = INVALID_SOCKET;
		std::array<char, 2 * address_length> _accept_buffer {};
#elifdef MINT_OS_LINUX
		sockaddr_storage _remote_address {};
		socklen_t _remote_address_length = static_cast<socklen_t>(sizeof(_remote_address));
#endif
	public:
		AsyncAcceptOperation(mint::Cursor& cursor, mint::Reference self, SOCKET socket_fd) :
		    mint::MintAsyncOperation(std::move(self), std::bit_cast<mint::handle_t>(socket_fd)),
		    _cursor(cursor) {}

		std::error_code start() override {
			const auto socket_fd = reinterpret_cast<SOCKET>(get_handle());

			try {
#ifdef MINT_ASYNC_BACKEND_IOCP
				const auto type = mint_network::get_socket_option<int>(socket_fd, SO_TYPE);
				const auto info = mint_network::get_socket_option<WSAPROTOCOL_INFOW>(socket_fd, SO_PROTOCOL_INFOW);

				_client_fd = WSASocketW(info.iAddressFamily, type, info.iProtocol, nullptr, 0, WSA_FLAG_OVERLAPPED);

				auto bytes_received = DWORD();
				auto* address_buffer = _accept_buffer.data();
				auto address_buffer_length = static_cast<DWORD>(address_length);
				if (!AcceptEx(socket_fd, _client_fd, address_buffer, 0, address_buffer_length, address_buffer_length,
				        &bytes_received, this)) {
					switch (WSAGetLastError()) {
					case WSA_IO_PENDING:
						break;
					default:
						return mint_network::last_socket_error_code();
					}
				}
#elifdef MINT_ASYNC_BACKEND_KQUEUE
				filter = EVFILT_READ;
				result = accept(socket_fd, reinterpret_cast<sockaddr*>(&_remote_address), &_remote_address_length);
				if (result < 0) {
					return mint::last_error_code();
				}
#elifdef MINT_OS_LINUX
				return std::visit(mint::Overloaded {
#ifdef MINT_ASYNC_BACKEND_IO_URING
				                      [&](mint::IoUringOperation& self) -> std::error_code {
					                      io_uring_prep_accept(self.sqe, socket_fd,
					                          reinterpret_cast<sockaddr*>(&_remote_address), &_remote_address_length,
					                          0);
					                      return {};
				                      },
#endif
#ifdef MINT_ASYNC_BACKEND_EPOLL
				                      [&](mint::EPollOperation& self) -> std::error_code {
					                      const auto _ = mint_network::SocketBlockingModeGuard<false>(socket_fd);
					                      self.events = EPOLLIN;
					                      self.result = accept(socket_fd, reinterpret_cast<sockaddr*>(&_remote_address),
					                          &_remote_address_length);
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
			}
			catch (const std::system_error& error) {
				return error.code();
			}
			return {};
		}

		void complete(std::error_code error, [[maybe_unused]] std::size_t bytes_transferred) override {
			if (error) {
				done(mint::create_iterator_from(_cursor, mint::create_number(error.value())));
			}
			else {
#ifdef MINT_ASYNC_BACKEND_IOCP
				auto socket_fd = reinterpret_cast<SOCKET>(get_handle());
				auto* address_buffer = _accept_buffer.data();
				auto address_buffer_length = static_cast<DWORD>(address_length);
				socklen_t local_address_length = sizeof(sockaddr);
				LPSOCKADDR local_address = nullptr;
				socklen_t remote_address_length = sizeof(sockaddr);
				LPSOCKADDR remote_address = nullptr;

				GetAcceptExSockaddrs(address_buffer, 0, address_buffer_length, address_buffer_length, &local_address,
				    &local_address_length, &remote_address, &remote_address_length);

				try {
					mint_network::set_socket_option(_client_fd, SO_UPDATE_ACCEPT_CONTEXT, socket_fd);
					const auto [address, port] = mint_network::get_ip_socket_info(*remote_address);
					mint_network::SocketManager::instance().accept_socket(_client_fd);
					done(mint::create_iterator_from(_cursor, mint::create_number(0),
					    mint_network::create_socket(_cursor.get().ast(), _client_fd),
					    mint::create_string(_cursor.get().ast(), address), mint::create_number(port)));
				}
				catch (const std::system_error& error) {
					done(mint::create_iterator_from(_cursor, mint::create_number(error.code().value())));
				}
#elifdef MINT_OS_LINUX
				const auto client_fd = static_cast<SOCKET>(bytes_transferred);
				const auto [address, port] = mint_network::get_ip_socket_info(
				    *reinterpret_cast<sockaddr*>(&_remote_address));
				mint_network::SocketManager::instance().accept_socket(client_fd);
				done(mint::create_iterator_from(_cursor, mint::create_number(0),
				    mint_network::create_socket(_cursor.get().ast(), client_fd),
				    mint::create_string(_cursor.get().ast(), address), mint::create_number(port)));
#else
#error "This operation is not implemented for this platform"
#endif
			}
		}
	};

	return mint::create_async_operation(cursor.ast(),
	    new AsyncAcceptOperation(cursor, std::move(self), std::bit_cast<SOCKET>(mint::to_handle(socket))));
}

mint::Reference mint_ip_socket_setup_options(mint::Cursor& /*cursor*/, const mint::Reference& ip_socket_option) {

#define BIND_MCAST_VALUE(_enum, _option) \
	_enum.data<mint::Object>().metadata.find_global(#_option)->value.data<mint::Number>().value = _option
#define BIND_MCAST_DISABLE(_enum, _option) \
	_enum.data<mint::Object>().metadata.find_global(#_option)->value.move_data(mint::create_none())

#ifdef MCAST_JOIN_GROUP
	BIND_MCAST_VALUE(ip_socket_option, MCAST_JOIN_GROUP);
#else
	BIND_MCAST_DISABLE(ip_socket_option, MCAST_JOIN_GROUP);
#endif
#ifdef MCAST_LEAVE_GROUP
	BIND_MCAST_VALUE(ip_socket_option, MCAST_LEAVE_GROUP);
#else
	BIND_MCAST_DISABLE(ip_socket_option, MCAST_LEAVE_GROUP);
#endif
#ifdef MCAST_BLOCK_SOURCE
	BIND_MCAST_VALUE(ip_socket_option, MCAST_BLOCK_SOURCE);
#else
	BIND_MCAST_DISABLE(ip_socket_option, MCAST_BLOCK_SOURCE);
#endif
#ifdef MCAST_UNBLOCK_SOURCE
	BIND_MCAST_VALUE(ip_socket_option, MCAST_UNBLOCK_SOURCE);
#else
	BIND_MCAST_DISABLE(ip_socket_option, MCAST_UNBLOCK_SOURCE);
#endif
#ifdef MCAST_JOIN_SOURCE_GROUP
	BIND_MCAST_VALUE(ip_socket_option, MCAST_JOIN_SOURCE_GROUP);
#else
	BIND_MCAST_DISABLE(ip_socket_option, MCAST_JOIN_SOURCE_GROUP);
#endif
#ifdef MCAST_LEAVE_SOURCE_GROUP
	BIND_MCAST_VALUE(ip_socket_option, MCAST_LEAVE_SOURCE_GROUP);
#else
	BIND_MCAST_DISABLE(ip_socket_option, MCAST_LEAVE_SOURCE_GROUP);
#endif

	return {};
}

mint::Reference mint_ipv4_socket_setup_options(mint::Cursor& /*cursor*/, const mint::Reference& ip_v4_socket_option) {

#define BIND_IP_VALUE(_enum, _option) \
	_enum.data<mint::Object>().metadata.find_global(#_option)->value.data<mint::Number>().value = IP_##_option
#define BIND_IP_DISABLE(_enum, _option) \
	_enum.data<mint::Object>().metadata.find_global(#_option)->value.move_data(mint::create_none())

#ifdef IP_HEADERSINCL
	BIND_IP_VALUE(ip_v4_socket_option, HDRINCL);
#else
	BIND_IP_DISABLE(ip_v4_socket_option, HDRINCL);
#endif
#ifdef IP_OPTIONS
	BIND_IP_VALUE(ip_v4_socket_option, OPTIONS);
#else
	BIND_IP_DISABLE(ip_v4_socket_option, OPTIONS);
#endif
#ifdef IP_RECVDSTADDR
	BIND_IP_VALUE(ip_v4_socket_option, RECVDSTADDR);
#else
	BIND_IP_DISABLE(ip_v4_socket_option, RECVDSTADDR);
#endif
#ifdef IP_RECVIF
	BIND_IP_VALUE(ip_v4_socket_option, RECVIF);
#else
	BIND_IP_DISABLE(ip_v4_socket_option, RECVIF);
#endif
#ifdef IP_TOS
	BIND_IP_VALUE(ip_v4_socket_option, TOS);
#else
	BIND_IP_DISABLE(ip_v4_socket_option, TOS);
#endif
#ifdef IP_TTL
	BIND_IP_VALUE(ip_v4_socket_option, TTL);
#else
	BIND_IP_DISABLE(ip_v4_socket_option, TTL);
#endif
#ifdef IP_MULTICAST_IF
	BIND_IP_VALUE(ip_v4_socket_option, MULTICAST_IF);
#else
	BIND_IP_DISABLE(ip_v4_socket_option, MULTICAST_IF);
#endif
#ifdef IP_MULTICAST_TTL
	BIND_IP_VALUE(ip_v4_socket_option, MULTICAST_TTL);
#else
	BIND_IP_DISABLE(ip_v4_socket_option, MULTICAST_TTL);
#endif
#ifdef IP_MULTICAST_LOOP
	BIND_IP_VALUE(ip_v4_socket_option, MULTICAST_LOOP);
#else
	BIND_IP_DISABLE(ip_v4_socket_option, MULTICAST_LOOP);
#endif
#ifdef IP_ADD_MEMBERSHIP
	BIND_IP_VALUE(ip_v4_socket_option, ADD_MEMBERSHIP);
#else
	BIND_IP_DISABLE(ip_v4_socket_option, ADD_MEMBERSHIP);
#endif
#ifdef IP_DROP_MEMBERSHIP
	BIND_IP_VALUE(ip_v4_socket_option, DROP_MEMBERSHIP);
#else
	BIND_IP_DISABLE(ip_v4_socket_option, DROP_MEMBERSHIP);
#endif
#ifdef IP_BLOCK_SOURCE
	BIND_IP_VALUE(ip_v4_socket_option, BLOCK_SOURCE);
#else
	BIND_IP_DISABLE(ip_v4_socket_option, BLOCK_SOURCE);
#endif
#ifdef IP_UNBLOCK_SOURCE
	BIND_IP_VALUE(ip_v4_socket_option, UNBLOCK_SOURCE);
#else
	BIND_IP_DISABLE(ip_v4_socket_option, UNBLOCK_SOURCE);
#endif
#ifdef IP_ADD_SOURCE_MEMBERSHIP
	BIND_IP_VALUE(ip_v4_socket_option, ADD_SOURCE_MEMBERSHIP);
#else
	BIND_IP_DISABLE(ip_v4_socket_option, ADD_SOURCE_MEMBERSHIP);
#endif
#ifdef IP_DROP_SOURCE_MEMBERSHIP
	BIND_IP_VALUE(ip_v4_socket_option, DROP_SOURCE_MEMBERSHIP);
#else
	BIND_IP_DISABLE(ip_v4_socket_option, DROP_SOURCE_MEMBERSHIP);
#endif

	return {};
}

mint::Reference mint_ipv4_socket_get_option_number(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::Reference result = mint::create_iterator(cursor.ast());

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = to_integer<int>(cursor, option);
		const auto option_value = mint_network::get_socket_option<int>(socket_fd, IPPROTO_IP, option_id);
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(option_value));
	}
	catch (const std::system_error& error) {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(error.code().value()));
	}

	return result;
}

mint::Reference mint_ipv4_socket_set_option_number(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = to_integer<int>(cursor, option);
		const auto option_value = to_integer<int>(cursor, value);
		mint_network::set_socket_option(socket_fd, IPPROTO_IP, option_id, option_value);
	}
	catch (const std::system_error& error) {
		return mint::create_number(error.code().value());
	}

	return {};
}

mint::Reference mint_ipv4_socket_get_option_boolean(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::Reference result = mint::create_iterator(cursor.ast());

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = to_integer<int>(cursor, option);
		const auto option_value = mint_network::get_socket_option<mint_network::sockopt_bool>(socket_fd, IPPROTO_IP,
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

mint::Reference mint_ipv4_socket_set_option_boolean(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = to_integer<int>(cursor, option);
		const auto option_value = to_boolean(value) ? mint_network::sockopt_true : mint_network::sockopt_false;
		mint_network::set_socket_option(socket_fd, IPPROTO_IP, option_id, option_value);
	}
	catch (const std::system_error& error) {
		return mint::create_number(error.code().value());
	}

	return {};
}

mint::Reference mint_ipv4_socket_get_option_byte(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::Reference result = mint::create_iterator(cursor.ast());

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = to_integer<int>(cursor, option);
		const auto option_value = mint_network::get_socket_option<u_char>(socket_fd, IPPROTO_IP, option_id);
		iterator_yield(cursor, result.data<mint::Iterator>(),
		    mint::create_number(option_value != mint_network::sockopt_false));
	}
	catch (const std::system_error& error) {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(error.code().value()));
	}

	return result;
}

mint::Reference mint_ipv4_socket_set_option_byte(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = to_integer<int>(cursor, option);
		const auto option_value = to_integer<u_char>(cursor, value);
		mint_network::set_socket_option(socket_fd, IPPROTO_IP, option_id, option_value);
	}
	catch (const std::system_error& error) {
		return mint::create_number(error.code().value());
	}

	return {};
}

mint::Reference mint_ipv4_socket_get_option_flag(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::Reference result = mint::create_iterator(cursor.ast());

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = to_integer<int>(cursor, option);
		const auto option_value = mint_network::get_socket_option<u_char>(socket_fd, IPPROTO_IP, option_id);
		iterator_yield(cursor, result.data<mint::Iterator>(),
		    mint::create_boolean(option_value != mint_network::sockopt_false));
	}
	catch (const std::system_error& error) {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(error.code().value()));
	}

	return result;
}

mint::Reference mint_ipv4_socket_set_option_flag(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = to_integer<int>(cursor, option);
		const auto option_value = to_boolean(value) ? u_char {1} : u_char {0};
		mint_network::set_socket_option(socket_fd, IPPROTO_IP, option_id, option_value);
	}
	catch (const std::system_error& error) {
		return mint::create_number(error.code().value());
	}

	return {};
}

mint::Reference mint_ipv4_socket_get_option_addr(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::Reference result = mint::create_iterator(cursor.ast());

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = to_integer<int>(cursor, option);
		const auto option_value = mint_network::get_socket_option<in_addr>(socket_fd, IPPROTO_IP, option_id);
		auto buffer = std::array<char, INET_ADDRSTRLEN>();
		if (const char* address = inet_ntop(AF_INET, &option_value, buffer.data(), buffer.size())) {
			iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_string(cursor.ast(), address));
		}
		else {
			throw std::system_error(mint_network::last_socket_error_code());
		}
	}
	catch (const std::system_error& error) {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(error.code().value()));
	}

	return result;
}

mint::Reference mint_ipv4_socket_set_option_addr(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = to_integer<int>(cursor, option);
		const auto address_str = to_string(value);
		switch (auto option_value = in_addr(); ::inet_pton(AF_INET, address_str.c_str(), &option_value)) {
		case 0:
			throw std::system_error(std::error_code(EINVAL, std::system_category()));
		case 1:
			mint_network::set_socket_option(socket_fd, IPPROTO_IP, option_id, option_value);
			break;
		default:
			throw std::system_error(mint_network::last_socket_error_code());
		}
	}
	catch (const std::system_error& error) {
		return mint::create_number(error.code().value());
	}

	return {};
}

mint::Reference mint_ipv4_socket_get_option_mreq(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::Reference result = create_iterator(cursor.ast());

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = to_integer<int>(cursor, option);
		auto option_value = mint_network::get_socket_option<std::unique_ptr<ip_mreq>>(socket_fd, IPPROTO_IP, option_id);
		iterator_yield(cursor, result.data<mint::Iterator>(),
		    mint::create_c_object(cursor.ast(), option_value.release()));
	}
	catch (const std::system_error& error) {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(error.code().value()));
	}

	return result;
}

mint::Reference mint_ipv4_socket_set_option_mreq(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = to_integer<int>(cursor, option);
		const auto* option_value = value.data<mint::LibObject<ip_mreq>>().ptr;
		mint_network::set_socket_option(socket_fd, IPPROTO_IP, option_id, *option_value);
	}
	catch (const std::system_error& error) {
		return mint::create_number(error.code().value());
	}

	return {};
}

mint::Reference mint_ipv4_socket_get_option_mreq_source(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::Reference result = mint::create_iterator(cursor.ast());

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = to_integer<int>(cursor, option);
		auto option_value = mint_network::get_socket_option<std::unique_ptr<ip_mreq_source>>(socket_fd, IPPROTO_IP,
		    option_id);
		iterator_yield(cursor, result.data<mint::Iterator>(),
		    mint::create_c_object(cursor.ast(), option_value.release()));
	}
	catch (const std::system_error& error) {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(error.code().value()));
	}

	return result;
}

mint::Reference mint_ipv4_socket_set_option_mreq_source(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = to_integer<int>(cursor, option);
		const auto* option_value = value.data<mint::LibObject<ip_mreq_source>>().ptr;
		mint_network::set_socket_option(socket_fd, IPPROTO_IP, option_id, *option_value);
	}
	catch (const std::system_error& error) {
		return mint::create_number(error.code().value());
	}

	return {};
}

mint::Reference mint_ipv4_socket_mreq_create(mint::Cursor& cursor, const mint::Reference& imr_multiaddr,
    const mint::Reference& imr_interface) {

	auto group = std::make_unique<ip_mreq>();
	if (!inet_pton(AF_INET, to_string(imr_multiaddr).c_str(), &group->imr_multiaddr)) {
		return {};
	}
	if (!inet_pton(AF_INET, to_string(imr_interface).c_str(), &group->imr_interface)) {
		return {};
	}
	return create_c_object(cursor.ast(), group.release());
}

mint::Reference mint_ipv4_socket_mreq_delete(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	delete d_ptr.data<mint::LibObject<ip_mreq>>().ptr;
	return {};
}

mint::Reference mint_ipv4_socket_mreq_get_multiaddr(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	std::array<char, INET_ADDRSTRLEN> buffer {};
	if (const char* address = inet_ntop(AF_INET, &d_ptr.data<mint::LibObject<ip_mreq>>().ptr->imr_multiaddr,
	        buffer.data(), buffer.size())) {
		return create_string(cursor.ast(), address);
	}
	return {};
}

mint::Reference mint_ipv4_socket_mreq_set_multiaddr(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr,
    const mint::Reference& address) {
	return mint::create_boolean(
	    inet_pton(AF_INET, to_string(address).c_str(), &d_ptr.data<mint::LibObject<ip_mreq>>().ptr->imr_multiaddr));
}

mint::Reference mint_ipv4_socket_mreq_get_interface(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	std::array<char, INET_ADDRSTRLEN> buffer {};
	if (const char* address = inet_ntop(AF_INET, &d_ptr.data<mint::LibObject<ip_mreq>>().ptr->imr_interface,
	        buffer.data(), buffer.size())) {
		return create_string(cursor.ast(), address);
	}
	return {};
}

mint::Reference mint_ipv4_socket_mreq_set_interface(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr,
    const mint::Reference& address) {
	return mint::create_boolean(
	    inet_pton(AF_INET, to_string(address).c_str(), &d_ptr.data<mint::LibObject<ip_mreq>>().ptr->imr_interface));
}

mint::Reference mint_ipv4_socket_mreq_source_create(mint::Cursor& cursor, const mint::Reference& imr_multiaddr,
    const mint::Reference& imr_sourceaddr, const mint::Reference& imr_interface) {
	auto group = std::make_unique<ip_mreq_source>();
	if (!inet_pton(AF_INET, to_string(imr_multiaddr).c_str(), &group->imr_multiaddr)) {
		return {};
	}
	if (!inet_pton(AF_INET, to_string(imr_sourceaddr).c_str(), &group->imr_sourceaddr)) {
		return {};
	}
	if (!inet_pton(AF_INET, to_string(imr_interface).c_str(), &group->imr_interface)) {
		return {};
	}
	return create_c_object(cursor.ast(), group.release());
}

mint::Reference mint_ipv4_socket_mreq_source_delete(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	delete d_ptr.data<mint::LibObject<ip_mreq_source>>().ptr;
	return {};
}

mint::Reference mint_ipv4_socket_mreq_source_get_multiaddr(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	std::array<char, INET_ADDRSTRLEN> buffer {};
	if (const char* address = inet_ntop(AF_INET, &d_ptr.data<mint::LibObject<ip_mreq_source>>().ptr->imr_multiaddr,
	        buffer.data(), buffer.size())) {
		return create_string(cursor.ast(), address);
	}
	return {};
}

mint::Reference mint_ipv4_socket_mreq_source_set_multiaddr(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr,
    const mint::Reference& address) {
	return mint::create_boolean(inet_pton(AF_INET, to_string(address).c_str(),
	    &d_ptr.data<mint::LibObject<ip_mreq_source>>().ptr->imr_multiaddr));
}

mint::Reference mint_ipv4_socket_mreq_source_get_sourceaddr(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	std::array<char, INET_ADDRSTRLEN> buffer {};
	if (const char* address = inet_ntop(AF_INET, &d_ptr.data<mint::LibObject<ip_mreq_source>>().ptr->imr_sourceaddr,
	        buffer.data(), buffer.size())) {
		return create_string(cursor.ast(), address);
	}
	return {};
}

mint::Reference mint_ipv4_socket_mreq_source_set_sourceaddr(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr,
    const mint::Reference& address) {
	return mint::create_boolean(inet_pton(AF_INET, to_string(address).c_str(),
	    &d_ptr.data<mint::LibObject<ip_mreq_source>>().ptr->imr_sourceaddr));
}

mint::Reference mint_ipv4_socket_mreq_source_get_interface(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	std::array<char, INET_ADDRSTRLEN> buffer {};
	if (const char* address = inet_ntop(AF_INET, &d_ptr.data<mint::LibObject<ip_mreq_source>>().ptr->imr_interface,
	        buffer.data(), buffer.size())) {
		return create_string(cursor.ast(), address);
	}
	return {};
}

mint::Reference mint_ipv4_socket_mreq_source_set_interface(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr,
    const mint::Reference& address) {
	return mint::create_boolean(inet_pton(AF_INET, to_string(address).c_str(),
	    &d_ptr.data<mint::LibObject<ip_mreq_source>>().ptr->imr_interface));
}

mint::Reference mint_ipv6_socket_setup_options(mint::Cursor& /*cursor*/, const mint::Reference& ip_v6_socket_option) {

#define BIND_IPV6_VALUE(_enum, _option) \
	_enum.data<mint::Object>().metadata.find_global(#_option)->value.data<mint::Number>().value = IPV6_##_option
#define BIND_IPV6_DISABLE(_enum, _option) \
	_enum.data<mint::Object>().metadata.find_global(#_option)->value.move_data(mint::create_none())

#ifdef IPV6_CHECKSUM
	BIND_IPV6_VALUE(ip_v6_socket_option, CHECKSUM);
#else
	BIND_IPV6_DISABLE(ip_v6_socket_option, CHECKSUM);
#endif
#ifdef IPV6_DONTFRAG
	BIND_IPV6_VALUE(ip_v6_socket_option, DONTFRAG);
#else
	BIND_IPV6_DISABLE(ip_v6_socket_option, DONTFRAG);
#endif
#ifdef IPV6_NEXTHOP
	BIND_IPV6_VALUE(ip_v6_socket_option, NEXTHOP);
#else
	BIND_IPV6_DISABLE(ip_v6_socket_option, NEXTHOP);
#endif
#ifdef IPV6_PATHMTU
	BIND_IPV6_VALUE(ip_v6_socket_option, PATHMTU);
#else
	BIND_IPV6_DISABLE(ip_v6_socket_option, PATHMTU);
#endif
#ifdef IPV6_RECVDSTOPTS
	BIND_IPV6_VALUE(ip_v6_socket_option, RECVDSTOPTS);
#else
	BIND_IPV6_DISABLE(ip_v6_socket_option, RECVDSTOPTS);
#endif
#ifdef IPV6_RECVHOPLIMIT
	BIND_IPV6_VALUE(ip_v6_socket_option, RECVHOPLIMIT);
#else
	BIND_IPV6_DISABLE(ip_v6_socket_option, RECVHOPLIMIT);
#endif
#ifdef IPV6_RECVHOPOPTS
	BIND_IPV6_VALUE(ip_v6_socket_option, RECVHOPOPTS);
#else
	BIND_IPV6_DISABLE(ip_v6_socket_option, RECVHOPOPTS);
#endif
#ifdef IPV6_RECVPATHMTU
	BIND_IPV6_VALUE(ip_v6_socket_option, RECVPATHMTU);
#else
	BIND_IPV6_DISABLE(ip_v6_socket_option, RECVPATHMTU);
#endif
#ifdef IPV6_RECVPKTINFO
	BIND_IPV6_VALUE(ip_v6_socket_option, RECVPKTINFO);
#else
	BIND_IPV6_DISABLE(ip_v6_socket_option, RECVPKTINFO);
#endif
#ifdef IPV6_RECVRTHDR
	BIND_IPV6_VALUE(ip_v6_socket_option, RECVRTHDR);
#else
	BIND_IPV6_DISABLE(ip_v6_socket_option, RECVRTHDR);
#endif
#ifdef IPV6_RECVTCLASS
	BIND_IPV6_VALUE(ip_v6_socket_option, RECVTCLASS);
#else
	BIND_IPV6_DISABLE(ip_v6_socket_option, RECVTCLASS);
#endif
#ifdef IPV6_UNICAT_HOPS
	BIND_IPV6_VALUE(ip_v6_socket_option, UNICAT_HOPS);
#else
	BIND_IPV6_DISABLE(ip_v6_socket_option, UNICAT_HOPS);
#endif
#ifdef IPV6_USE_MIN_MTU
	BIND_IPV6_VALUE(ip_v6_socket_option, USE_MIN_MTU);
#else
	BIND_IPV6_DISABLE(ip_v6_socket_option, USE_MIN_MTU);
#endif
#ifdef IPV6_V6ONLY
	BIND_IPV6_VALUE(ip_v6_socket_option, V6ONLY);
#else
	BIND_IPV6_DISABLE(ip_v6_socket_option, V6ONLY);
#endif
#ifdef IPV6_XXX
	BIND_IPV6_VALUE(ip_v6_socket_option, XXX);
#else
	BIND_IPV6_DISABLE(ip_v6_socket_option, XXX);
#endif
#ifdef IPV6_MULTICAST_IF
	BIND_IPV6_VALUE(ip_v6_socket_option, MULTICAST_IF);
#else
	BIND_IPV6_DISABLE(ip_v6_socket_option, MULTICAST_IF);
#endif
#ifdef IPV6_MULTICAST_HOPS
	BIND_IPV6_VALUE(ip_v6_socket_option, MULTICAST_HOPS);
#else
	BIND_IPV6_DISABLE(ip_v6_socket_option, MULTICAST_HOPS);
#endif
#ifdef IPV6_MULTICAST_LOOP
	BIND_IPV6_VALUE(ip_v6_socket_option, MULTICAST_LOOP);
#else
	BIND_IPV6_DISABLE(ip_v6_socket_option, MULTICAST_LOOP);
#endif
#ifdef IPV6_JOIN_GROUP
	BIND_IPV6_VALUE(ip_v6_socket_option, JOIN_GROUP);
#else
	BIND_IPV6_DISABLE(ip_v6_socket_option, JOIN_GROUP);
#endif
#ifdef IPV6_LEAVE_GROUP
	BIND_IPV6_VALUE(ip_v6_socket_option, LEAVE_GROUP);
#else
	BIND_IPV6_DISABLE(ip_v6_socket_option, LEAVE_GROUP);
#endif

	return {};
}

mint::Reference mint_ipv6_socket_get_option_number(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::Reference result = mint::create_iterator(cursor.ast());

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = to_integer<int>(cursor, option);
		const auto option_value = mint_network::get_socket_option<int>(socket_fd, IPPROTO_IPV6, option_id);
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(option_value));
	}
	catch (const std::system_error& error) {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(error.code().value()));
	}

	return result;
}

mint::Reference mint_ipv6_socket_set_option_number(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = to_integer<int>(cursor, option);
		const auto option_value = to_integer<int>(cursor, value);
		mint_network::set_socket_option(socket_fd, IPPROTO_IPV6, option_id, option_value);
	}
	catch (const std::system_error& error) {
		return mint::create_number(error.code().value());
	}

	return {};
}

mint::Reference mint_ipv6_socket_get_option_boolean(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::Reference result = mint::create_iterator(cursor.ast());

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = to_integer<int>(cursor, option);
		const auto option_value = mint_network::get_socket_option<mint_network::sockopt_bool>(socket_fd, IPPROTO_IPV6,
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

mint::Reference mint_ipv6_socket_set_option_boolean(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = to_integer<int>(cursor, option);
		const auto option_value = to_boolean(value) ? mint_network::sockopt_true : mint_network::sockopt_false;
		mint_network::set_socket_option(socket_fd, IPPROTO_IPV6, option_id, option_value);
	}
	catch (const std::system_error& error) {
		return mint::create_number(error.code().value());
	}

	return {};
}

mint::Reference mint_ipv6_socket_get_option_addr(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::Reference result = mint::create_iterator(cursor.ast());

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = to_integer<int>(cursor, option);
		auto option_value = mint_network::get_socket_option<std::unique_ptr<sockaddr_in6>>(socket_fd, IPPROTO_IPV6,
		    option_id);
		iterator_yield(cursor, result.data<mint::Iterator>(),
		    mint::create_c_object(cursor.ast(), option_value.release()));
	}
	catch (const std::system_error& error) {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(error.code().value()));
	}

	return result;
}

mint::Reference mint_ipv6_socket_set_option_addr(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = to_integer<int>(cursor, option);
		const auto* option_value = value.data<mint::LibObject<sockaddr_in6>>().ptr;
		mint_network::set_socket_option(socket_fd, IPPROTO_IPV6, option_id, *option_value);
	}
	catch (const std::system_error& error) {
		return mint::create_number(error.code().value());
	}

	return {};
}

mint::Reference mint_ipv6_socket_get_option_mtuinfo(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::Reference result = mint::create_iterator(cursor.ast());

#if defined(MINT_OS_LINUX) && defined(__UAPI_DEF_IP6_MTUINFO)
	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = to_integer<int>(cursor, option);
		auto option_value = mint_network::get_socket_option<std::unique_ptr<ip6_mtuinfo>>(socket_fd, IPPROTO_IPV6,
		    option_id);
		iterator_yield(cursor, result.data<mint::Iterator>(),
		    mint::create_c_object(cursor.ast(), option_value.release()));
	}
	catch (const std::system_error& error) {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(error.code().value()));
	}
#else
	((void)option);
	((void)socket);
	iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
	iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(ENOTSUP));
#endif

	return result;
}

mint::Reference mint_ipv6_socket_set_option_mtuinfo(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

#if defined(MINT_OS_LINUX) && defined(__UAPI_DEF_IP6_MTUINFO)
	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = to_integer<int>(cursor, option);
		const auto* option_value = value.data<mint::LibObject<ip6_mtuinfo>>().ptr;
		mint_network::set_socket_option(socket_fd, IPPROTO_IPV6, option_id, *option_value);
	}
	catch (const std::system_error& error) {
		return mint::create_number(error.code().value());
	}

	return {};
#else
	((void)cursor);
	((void)value);
	((void)option);
	((void)socket);
	return mint::create_number(ENOTSUP);
#endif
}

mint::Reference mint_ipv6_socket_get_option_mreq(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::Reference result = mint::create_iterator(cursor.ast());

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = to_integer<int>(cursor, option);
		auto option_value = mint_network::get_socket_option<std::unique_ptr<ipv6_mreq>>(socket_fd, IPPROTO_IPV6,
		    option_id);
		iterator_yield(cursor, result.data<mint::Iterator>(),
		    mint::create_c_object(cursor.ast(), option_value.release()));
	}
	catch (const std::system_error& error) {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(error.code().value()));
	}

	return result;
}

mint::Reference mint_ipv6_socket_set_option_mreq(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = to_integer<int>(cursor, option);
		const auto* option_value = value.data<mint::LibObject<ipv6_mreq>>().ptr;
		mint_network::set_socket_option(socket_fd, IPPROTO_IPV6, option_id, *option_value);
	}
	catch (const std::system_error& error) {
		return mint::create_number(error.code().value());
	}

	return {};
}

mint::Reference mint_ipv6_socket_mreq_create(mint::Cursor& cursor, const mint::Reference& ipv6mr_multiaddr,
    mint::Reference& ipv6mr_interface) {

	auto group = std::make_unique<ipv6_mreq>();
	if (!inet_pton(AF_INET6, to_string(ipv6mr_multiaddr).c_str(), &group->ipv6mr_multiaddr)) {
		return {};
	}
#ifdef MINT_OS_WINDOWS
	group->ipv6mr_interface = to_integer<ULONG>(cursor, ipv6mr_interface);
#else
	group->ipv6mr_ifindex = to_integer<int>(cursor, ipv6mr_interface);
#endif
	return create_c_object(cursor.ast(), group.release());
}

mint::Reference mint_ipv6_socket_mreq_delete(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	delete d_ptr.data<mint::LibObject<ipv6_mreq>>().ptr;
	return {};
}

mint::Reference mint_ipv6_socket_mreq_get_multiaddr(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	std::array<char, INET_ADDRSTRLEN> buffer {};
	if (const char* address = inet_ntop(AF_INET6, &d_ptr.data<mint::LibObject<ipv6_mreq>>().ptr->ipv6mr_multiaddr,
	        buffer.data(), buffer.size())) {
		return create_string(cursor.ast(), address);
	}
	return {};
}

mint::Reference mint_ipv6_socket_mreq_set_multiaddr(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr,
    const mint::Reference& address) {
	return mint::create_boolean(inet_pton(AF_INET6, to_string(address).c_str(),
	    &d_ptr.data<mint::LibObject<ipv6_mreq>>().ptr->ipv6mr_multiaddr));
}

mint::Reference mint_ipv6_socket_mreq_get_interface(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
#ifdef MINT_OS_WINDOWS
	return mint::create_number(d_ptr.data<mint::LibObject<ipv6_mreq>>().ptr->ipv6mr_interface);
#else
	return mint::create_number(d_ptr.data<mint::LibObject<ipv6_mreq>>().ptr->ipv6mr_ifindex);
#endif
}

mint::Reference mint_ipv6_socket_mreq_set_interface(mint::Cursor& cursor, const mint::Reference& d_ptr,
    mint::Reference& index) {
#ifdef MINT_OS_WINDOWS
	d_ptr.data<mint::LibObject<ipv6_mreq>>().ptr->ipv6mr_interface = to_integer<ULONG>(cursor, index);
#else
	d_ptr.data<mint::LibObject<ipv6_mreq>>().ptr->ipv6mr_ifindex = to_integer<int>(cursor, index);
#endif
	return {};
}

}

MINT_EXPORT_FUNCTION(mint_ip_endpoint_create, 3)
MINT_EXPORT_FUNCTION(mint_ip_endpoint_delete, 1)
MINT_EXPORT_FUNCTION(mint_ip_endpoint_get_version, 1)
MINT_EXPORT_FUNCTION(mint_ip_endpoint_get_address, 1)
MINT_EXPORT_FUNCTION(mint_ip_endpoint_get_port, 1)

MINT_EXPORT_FUNCTION(mint_ip_socket_accept, 1);
MINT_EXPORT_FUNCTION(mint_ip_socket_accept_async, 2);

MINT_EXPORT_FUNCTION(mint_ip_socket_setup_options, 1);
MINT_EXPORT_FUNCTION(mint_ipv4_socket_setup_options, 1);
MINT_EXPORT_FUNCTION(mint_ipv4_socket_get_option_number, 2);
MINT_EXPORT_FUNCTION(mint_ipv4_socket_set_option_number, 3);
MINT_EXPORT_FUNCTION(mint_ipv4_socket_get_option_boolean, 2);
MINT_EXPORT_FUNCTION(mint_ipv4_socket_set_option_boolean, 3);
MINT_EXPORT_FUNCTION(mint_ipv4_socket_get_option_byte, 2);
MINT_EXPORT_FUNCTION(mint_ipv4_socket_set_option_byte, 3);
MINT_EXPORT_FUNCTION(mint_ipv4_socket_get_option_flag, 2);
MINT_EXPORT_FUNCTION(mint_ipv4_socket_set_option_flag, 3);
MINT_EXPORT_FUNCTION(mint_ipv4_socket_get_option_addr, 2);
MINT_EXPORT_FUNCTION(mint_ipv4_socket_set_option_addr, 3);
MINT_EXPORT_FUNCTION(mint_ipv4_socket_get_option_mreq, 2);
MINT_EXPORT_FUNCTION(mint_ipv4_socket_set_option_mreq, 3);
MINT_EXPORT_FUNCTION(mint_ipv4_socket_get_option_mreq_source, 2);
MINT_EXPORT_FUNCTION(mint_ipv4_socket_set_option_mreq_source, 3);

MINT_EXPORT_FUNCTION(mint_ipv4_socket_mreq_create, 2);
MINT_EXPORT_FUNCTION(mint_ipv4_socket_mreq_delete, 1);
MINT_EXPORT_FUNCTION(mint_ipv4_socket_mreq_get_multiaddr, 1);
MINT_EXPORT_FUNCTION(mint_ipv4_socket_mreq_set_multiaddr, 2);
MINT_EXPORT_FUNCTION(mint_ipv4_socket_mreq_get_interface, 1);
MINT_EXPORT_FUNCTION(mint_ipv4_socket_mreq_set_interface, 2);

MINT_EXPORT_FUNCTION(mint_ipv4_socket_mreq_source_create, 3);
MINT_EXPORT_FUNCTION(mint_ipv4_socket_mreq_source_delete, 1);
MINT_EXPORT_FUNCTION(mint_ipv4_socket_mreq_source_get_multiaddr, 1);
MINT_EXPORT_FUNCTION(mint_ipv4_socket_mreq_source_set_multiaddr, 2);
MINT_EXPORT_FUNCTION(mint_ipv4_socket_mreq_source_get_sourceaddr, 1);
MINT_EXPORT_FUNCTION(mint_ipv4_socket_mreq_source_set_sourceaddr, 2);
MINT_EXPORT_FUNCTION(mint_ipv4_socket_mreq_source_get_interface, 1);
MINT_EXPORT_FUNCTION(mint_ipv4_socket_mreq_source_set_interface, 2);

MINT_EXPORT_FUNCTION(mint_ipv6_socket_setup_options, 1);
MINT_EXPORT_FUNCTION(mint_ipv6_socket_get_option_number, 2);
MINT_EXPORT_FUNCTION(mint_ipv6_socket_set_option_number, 3);
MINT_EXPORT_FUNCTION(mint_ipv6_socket_get_option_boolean, 2);
MINT_EXPORT_FUNCTION(mint_ipv6_socket_set_option_boolean, 3);
MINT_EXPORT_FUNCTION(mint_ipv6_socket_get_option_addr, 2);
MINT_EXPORT_FUNCTION(mint_ipv6_socket_set_option_addr, 3);
MINT_EXPORT_FUNCTION(mint_ipv6_socket_get_option_mtuinfo, 2);
MINT_EXPORT_FUNCTION(mint_ipv6_socket_set_option_mtuinfo, 3);
MINT_EXPORT_FUNCTION(mint_ipv6_socket_get_option_mreq, 2);
MINT_EXPORT_FUNCTION(mint_ipv6_socket_set_option_mreq, 3);

MINT_EXPORT_FUNCTION(mint_ipv6_socket_mreq_create, 2);
MINT_EXPORT_FUNCTION(mint_ipv6_socket_mreq_delete, 1);
MINT_EXPORT_FUNCTION(mint_ipv6_socket_mreq_get_multiaddr, 1);
MINT_EXPORT_FUNCTION(mint_ipv6_socket_mreq_set_multiaddr, 2);
MINT_EXPORT_FUNCTION(mint_ipv6_socket_mreq_get_interface, 1);
MINT_EXPORT_FUNCTION(mint_ipv6_socket_mreq_set_interface, 2);
