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

#include "mint/ast/abstract_syntax_tree.h"
#include "mint/ast/symbol.h"
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
#include <bit>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <system_error>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <variant>
#include "socket.h"

#ifdef MINT_OS_WINDOWS
#include <WinSock2.h>
#include <MSWSock.h>
#include <afunix.h>
#include <handleapi.h>
#include <ws2def.h>
#include <ws2ipdef.h>
#include <minwindef.h>
#include <winerror.h>
#else
#ifdef MINT_ASYNC_BACKEND_EPOLL
#include <sys/epoll.h>
#endif
#include <asm-generic/ioctls.h>
#include <asm-generic/socket.h>
#include <bits/types/struct_timeval.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#endif

#ifdef MINT_ASYNC_BACKEND_IO_URING
#include <liburing.h>
#endif

namespace {

#ifdef MINT_OS_WINDOWS
constexpr char* sockopt_cast(auto* value) {
	return reinterpret_cast<char*>(value);
}
#else
constexpr auto* sockopt_cast(auto* value) {
	return value;
}
#endif

}

#ifdef MINT_OS_WINDOWS
mint_network::SocketManager::SocketManager() :
    _wsa_data {} {
	WSAStartup(MAKEWORD(2, 0), &_wsa_data);
}
#else
mint_network::SocketManager::SocketManager() {}
#endif

mint_network::SocketManager::~SocketManager() {
#ifdef MINT_OS_WINDOWS
	WSACleanup();
#endif
}

mint_network::SocketManager& mint_network::SocketManager::instance() {
	static SocketManager g_instance;
	return g_instance;
}

SOCKET mint_network::SocketManager::open_socket(int domain, int type, int protocol) {
#ifdef MINT_OS_WINDOWS
	const auto socket = WSASocketW(domain, type, protocol, nullptr, 0, WSA_FLAG_OVERLAPPED);
#else
	const auto socket = ::socket(domain, type, protocol);
#endif

	if (socket != INVALID_SOCKET) {
		_sockets.emplace(socket, SocketInfo {
		                             .native = true,
		                             .blocked = false,
		                             .blocking = true,
		                             .listening = false,
		                         });
	}

	return socket;
}

void mint_network::SocketManager::accept_socket(SOCKET socket) {
	_sockets.emplace(socket, SocketInfo {
	                             .native = true,
	                             .blocked = false,
	                             .blocking = true,
	                             .listening = false,
	                         });
}

void mint_network::SocketManager::close_socket(SOCKET socket) {
	_sockets.erase(socket);
#ifdef MINT_OS_WINDOWS
	if (closesocket(socket) != 0) {
		throw std::system_error(last_socket_error_code());
	}
#else
	if (close(socket) != 0) {
		throw std::system_error(last_socket_error_code());
	}
#endif
}

SOCKET mint_network::SocketManager::open_socket_from_handle(mint::handle_t handle) {
	auto socket = std::bit_cast<SOCKET>(handle);
	if (socket != INVALID_SOCKET) {
		_sockets.emplace(socket, SocketInfo {
		                             .native = false,
		                             .blocked = false,
		                             .blocking = true,
		                             .listening = false,
		                         });
	}
	return socket;
}

void mint_network::SocketManager::accept_socket_from_handle(mint::handle_t handle) {
	auto socket = std::bit_cast<SOCKET>(handle);
	_sockets.emplace(socket, SocketInfo {
	                             .native = false,
	                             .blocked = false,
	                             .blocking = true,
	                             .listening = false,
	                         });
}

void mint_network::SocketManager::close_socket_from_handle(mint::handle_t handle) {
	_sockets.erase(std::bit_cast<SOCKET>(handle));
#ifdef MINT_OS_WINDOWS
	if (!CloseHandle(handle)) {
		throw std::system_error(last_socket_error_code());
	}
#else
	if (close(handle) != 0) {
		throw std::system_error(last_socket_error_code());
	}
#endif
}

bool mint_network::SocketManager::is_native_socket(SOCKET socket) const {
	if (const auto it = _sockets.find(socket); it != _sockets.end()) {
		return it->second.native;
	}
	return false;
}

bool mint_network::SocketManager::is_socket_listening(SOCKET socket) const {
	if (const auto it = _sockets.find(socket); it != _sockets.end()) {
		return it->second.listening;
	}
	return false;
}

void mint_network::SocketManager::set_socket_listening(SOCKET socket, bool listening) {
	if (const auto it = _sockets.find(socket); it != _sockets.end()) {
		it->second.listening = listening;
	}
}

bool mint_network::SocketManager::is_socket_blocking(SOCKET socket) const {
	if (const auto it = _sockets.find(socket); it != _sockets.end()) {
		return it->second.blocking;
	}
	return true;
}

void mint_network::SocketManager::set_socket_blocking(SOCKET socket, bool blocking) {
	if (const auto it = _sockets.find(socket); it != _sockets.end()) {
		it->second.blocking = blocking;
	}
}

bool mint_network::SocketManager::is_socket_blocked(SOCKET socket) const {
	if (const auto it = _sockets.find(socket); it != _sockets.end()) {
		return it->second.blocked;
	}
	return false;
}

void mint_network::SocketManager::set_socket_blocked(SOCKET socket, bool blocked) {
	if (const auto it = _sockets.find(socket); it != _sockets.end()) {
		it->second.blocked = blocked;
	}
}

mint::Reference mint_network::create_socket(mint::AbstractSyntaxTree& ast, SOCKET socket) {
	return mint::create_handle(ast, std::bit_cast<mint::handle_t>(socket));
}

std::tuple<sockaddr*, socklen_t> mint_network::to_sockaddr(const mint::Reference& reference) {
	switch (auto* address = reference.data<mint::LibObject<sockaddr>>().ptr; address->sa_family) {
	case AF_INET:
		return {address, static_cast<socklen_t>(sizeof(sockaddr_in))};
	case AF_INET6:
		return {address, static_cast<socklen_t>(sizeof(sockaddr_in6))};
#ifdef AF_UNIX
	case AF_UNIX:
		return {address, static_cast<socklen_t>(sizeof(sockaddr_un))};
#endif
	default:
		throw std::system_error(std::make_error_code(std::errc::address_family_not_supported));
	}
}

void mint_network::get_socket_option(SOCKET socket, int level, int option, void* value, socklen_t len) {
	if (getsockopt(socket, level, option, sockopt_cast(value), &len) != 0) {
		throw std::system_error(last_socket_error_code());
	}
}

void mint_network::set_socket_option(SOCKET socket, int level, int option, const void* value, socklen_t len) {
	if (setsockopt(socket, level, option, reinterpret_cast<const char*>(value), len) != 0) {
		throw std::system_error(last_socket_error_code());
	}
}

void mint_network::set_socket_non_blocking(SOCKET socket, bool enabled) {
#ifdef MINT_OS_WINDOWS
	auto value = static_cast<u_long>(enabled);
	if (ioctlsocket(socket, FIONBIO, &value) == SOCKET_ERROR) {
		throw std::system_error(mint_network::last_socket_error_code());
	}
#else
	auto value = static_cast<int>(enabled);
	if (ioctl(socket, FIONBIO, &value) == -1) {
		throw std::system_error(mint_network::last_socket_error_code());
	}
#endif
}

std::error_code mint_network::last_socket_error_code() {
	return {errno_from_socket_last_error(), std::generic_category()};
}

int mint_network::errno_from_socket_last_error() {
#ifdef MINT_OS_WINDOWS
	static const std::unordered_map<int, int> g_errno_for = {
	    {WSAEINTR, ECANCELED},
	    {WSAEBADF, EBADF},
	    {WSAEACCES, EACCES},
	    {WSAEFAULT, EFAULT},
	    {WSAEINVAL, EINVAL},
	    {WSAEMFILE, EMFILE},
	    {WSAEWOULDBLOCK, EWOULDBLOCK},
	    {WSAEINPROGRESS, EINPROGRESS},
	    {WSAEALREADY, EALREADY},
	    {WSAENOTSOCK, ENOTSOCK},
	    {WSAEDESTADDRREQ, EDESTADDRREQ},
	    {WSAEMSGSIZE, EMSGSIZE},
	    {WSAEPROTOTYPE, EPROTOTYPE},
	    {WSAENOPROTOOPT, ENOPROTOOPT},
	    {WSAEPROTONOSUPPORT, EPROTONOSUPPORT},
#ifdef ESOCKTNOSUPPORT
	    {WSAESOCKTNOSUPPORT, ESOCKTNOSUPPORT},
#endif
	    {WSAEOPNOTSUPP, EOPNOTSUPP},
#ifdef EPFNOSUPPORT
	    {WSAEPFNOSUPPORT, EPFNOSUPPORT},
#endif
	    {WSAEAFNOSUPPORT, EAFNOSUPPORT},
	    {WSAEADDRINUSE, EADDRINUSE},
	    {WSAEADDRNOTAVAIL, EADDRNOTAVAIL},
	    {WSAENETDOWN, ENETDOWN},
	    {WSAENETUNREACH, ENETUNREACH},
	    {WSAENETRESET, ENETRESET},
	    {WSAECONNABORTED, ECONNABORTED},
	    {WSAECONNRESET, ECONNRESET},
	    {WSAENOBUFS, ENOBUFS},
	    {WSAEISCONN, EISCONN},
	    {WSAENOTCONN, ENOTCONN},
#ifdef ESHUTDOWN
	    {WSAESHUTDOWN, ESHUTDOWN},
#endif
#ifdef ETOOMANYREFS
	    {WSAETOOMANYREFS, ETOOMANYREFS},
#endif
	    {WSAETIMEDOUT, ETIMEDOUT},
	    {WSAECONNREFUSED, ECONNREFUSED},
	    {WSAELOOP, ELOOP},
	    {WSAENAMETOOLONG, ENAMETOOLONG},
#ifdef EHOSTDOWN
	    {WSAEHOSTDOWN, EHOSTDOWN},
#endif
	    {WSAEHOSTUNREACH, EHOSTUNREACH},
	    {WSAENOTEMPTY, ENOTEMPTY},
#ifdef EPROCLIM
	    {WSAEPROCLIM, EPROCLIM},
#endif
#ifdef EUSERS
	    {WSAEUSERS, EUSERS},
#endif
#ifdef EDQUOT
	    {WSAEDQUOT, EDQUOT},
#endif
#ifdef ESTALE
	    {WSAESTALE, ESTALE},
#endif
#ifdef EREMOTE
	    {WSAEREMOTE, EREMOTE},
#endif
	    /** @todo
		{ WSASYSNOTREADY, },
		{ WSAVERNOTSUPPORTED, },
		{ WSANOTINITIALISED, },
		{ WSAEDISCON, },
		{ WSAENOMORE, },
		{ WSAECANCELLED, },
		{ WSAEINVALIDPROCTABLE, },
		{ WSAEINVALIDPROVIDER, },
		{ WSAEPROVIDERFAILEDINIT, },
		{ WSASYSCALLFAILURE, },
		{ WSASERVICE_NOT_FOUND, },
		{ WSATYPE_NOT_FOUND, },
		{ WSA_E_NO_MORE, },
		{ WSA_E_CANCELLED, },
		{ WSAEREFUSED, },
		{ WSAHOST_NOT_FOUND, },
		{ WSATRY_AGAIN, },
		{ WSANO_RECOVERY, },
		{ WSANO_DATA, },
		{ WSA_QOS_RECEIVERS, },
		{ WSA_QOS_SENDERS, },
		{ WSA_QOS_NO_SENDERS, },
		{ WSA_QOS_NO_RECEIVERS, },
		{ WSA_QOS_REQUEST_CONFIRMED, },
		{ WSA_QOS_ADMISSION_FAILURE, },
		{ WSA_QOS_POLICY_FAILURE, },
		{ WSA_QOS_BAD_STYLE, },
		{ WSA_QOS_BAD_OBJECT, },
		{ WSA_QOS_TRAFFIC_CTRL_ERROR, },
		{ WSA_QOS_GENERIC_ERROR, },
		{ WSA_QOS_ESERVICETYPE, },
		{ WSA_QOS_EFLOWSPEC, },
		{ WSA_QOS_EPROVSPECBUF, },
		{ WSA_QOS_EFILTERSTYLE, },
		{ WSA_QOS_EFILTERTYPE, },
		{ WSA_QOS_EFILTERCOUNT, },
		{ WSA_QOS_EOBJLENGTH, },
		{ WSA_QOS_EFLOWCOUNT, },
		{ WSA_QOS_EUNKOWNPSOBJ, },
		{ WSA_QOS_EPOLICYOBJ, },
		{ WSA_QOS_EFLOWDESC, },
		{ WSA_QOS_EPSFLOWSPEC, },
		{ WSA_QOS_EPSFILTERSPEC, },
		{ WSA_QOS_ESDMODEOBJ, },
		{ WSA_QOS_ESHAPERATEOBJ, },
		{ WSA_QOS_RESERVED_PETYPE, },
		{ WSA_SECURE_HOST_NOT_FOUND, },
		{ WSA_IPSEC_NAME_POLICY_ERROR, }
		*/
	};

	const auto it = g_errno_for.find(WSAGetLastError());
	return (it != g_errno_for.end()) ? it->second : EINVAL;
#else
	return errno;
#endif
}

namespace {

std::tuple<sockaddr_storage, socklen_t> make_local_endpoint_for_connect(const sockaddr& remote) {
	switch (remote.sa_family) {
	case AF_INET:
		{
			auto storage = sockaddr_storage {
			    .ss_family = AF_INET,
			};
			auto* addr = reinterpret_cast<sockaddr_in*>(&storage);
			addr->sin_addr.s_addr = htonl(INADDR_ANY);
			addr->sin_port = 0;
			return {storage, sizeof(sockaddr_in)};
		}

	case AF_INET6:
		{
			auto storage = sockaddr_storage {
			    .ss_family = AF_INET6,
			};

			auto* addr = reinterpret_cast<sockaddr_in6*>(&storage);
			addr->sin6_family = AF_INET6;
			addr->sin6_addr = in6addr_any;
			addr->sin6_port = 0;
			return {storage, sizeof(sockaddr_in6)};
		}

	default:
		throw std::system_error(std::make_error_code(std::errc::address_family_not_supported));
	}
}

mint::Reference mint_socket_is_non_blocking(mint::Cursor& /*cursor*/, const mint::Reference& socket) {

	const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));

#ifndef MINT_OS_WINDOWS
	if (const auto flags = fcntl(socket_fd, F_GETFL, 0); flags != -1) {
		return mint::create_boolean(flags & O_NONBLOCK);
	}
#endif

	return mint::create_boolean(!mint_network::SocketManager::instance().is_socket_blocking(socket_fd));
}

mint::Reference mint_socket_set_non_blocking(mint::Cursor& /*cursor*/, const mint::Reference& socket,
    mint::Reference& enabled) {

	const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
	const auto value = to_boolean(enabled);

	try {
		mint_network::set_socket_non_blocking(socket_fd, value);
	}
	catch (const std::system_error& error) {
		return mint::create_number(error.code().value());
	}

	mint_network::SocketManager::instance().set_socket_blocking(socket_fd, !value);
	return {};
}

mint::Reference mint_socket_setup_options(mint::Cursor& /*cursor*/, const mint::Reference& socket_option) {

#define BIND_SO_VALUE(_enum, _option) \
	_enum.data<mint::Object>().metadata.find_global(#_option)->value.data<mint::Number>().value = SO_##_option
#define BIND_SO_DISABLE(_enum, _option) \
	_enum.data<mint::Object>().metadata.find_global(#_option)->value.move_data(mint::create_none())

#ifdef SO_BROADCAST
	BIND_SO_VALUE(socket_option, BROADCAST);
#else
	BIND_SO_DISABLE(socket_option, BROADCAST);
#endif
#ifdef SO_DEBUG
	BIND_SO_VALUE(socket_option, DEBUG);
#else
	BIND_SO_DISABLE(socket_option, DEBUG);
#endif
#ifdef SO_DONTROUTE
	BIND_SO_VALUE(socket_option, DONTROUTE);
#else
	BIND_SO_DISABLE(socket_option, DONTROUTE);
#endif
#ifdef SO_ERROR
	BIND_SO_VALUE(socket_option, ERROR);
#else
	BIND_SO_DISABLE(socket_option, ERROR);
#endif
#ifdef SO_KEEPALIVE
	BIND_SO_VALUE(socket_option, KEEPALIVE);
#else
	BIND_SO_DISABLE(socket_option, KEEPALIVE);
#endif
#ifdef SO_LINGER
	BIND_SO_VALUE(socket_option, LINGER);
#else
	BIND_SO_DISABLE(socket_option, LINGER);
#endif
#ifdef SO_OOBINLINE
	BIND_SO_VALUE(socket_option, OOBINLINE);
#else
	BIND_SO_DISABLE(socket_option, OOBINLINE);
#endif
#ifdef SO_RCVBUF
	BIND_SO_VALUE(socket_option, RCVBUF);
#else
	BIND_SO_DISABLE(socket_option, RCVBUF);
#endif
#ifdef SO_SNDBUF
	BIND_SO_VALUE(socket_option, SNDBUF);
#else
	BIND_SO_DISABLE(socket_option, SNDBUF);
#endif
#ifdef SO_RCVLOWAT
	BIND_SO_VALUE(socket_option, RCVLOWAT);
#else
	BIND_SO_DISABLE(socket_option, RCVLOWAT);
#endif
#ifdef SO_SNDLOWAT
	BIND_SO_VALUE(socket_option, SNDLOWAT);
#else
	BIND_SO_DISABLE(socket_option, SNDLOWAT);
#endif
#ifdef SO_RCVTIMEO
	BIND_SO_VALUE(socket_option, RCVTIMEO);
#else
	BIND_SO_DISABLE(socket_option, RCVTIMEO);
#endif
#ifdef SO_SNDTIMEO
	BIND_SO_VALUE(socket_option, SNDTIMEO);
#else
	BIND_SO_DISABLE(socket_option, SNDTIMEO);
#endif
#ifdef SO_REUSEADDR
	BIND_SO_VALUE(socket_option, REUSEADDR);
#else
	BIND_SO_DISABLE(socket_option, REUSEADDR);
#endif
#ifdef SO_REUSEPORT
	BIND_SO_VALUE(socket_option, REUSEPORT);
#else
	BIND_SO_DISABLE(socket_option, REUSEPORT);
#endif
#ifdef SO_TYPE
	BIND_SO_VALUE(socket_option, TYPE);
#else
	BIND_SO_DISABLE(socket_option, TYPE);
#endif
#ifdef SO_USELOOPBACK
	BIND_SO_VALUE(socket_option, USELOOPBACK);
#else
	BIND_SO_DISABLE(socket_option, USELOOPBACK);
#endif
	return {};
}

mint::Reference mint_socket_get_option_number(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::Reference result = mint::create_iterator(cursor.ast());

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = mint::to_integer<int>(cursor, option);
		const auto option_value = mint_network::get_socket_option<int>(socket_fd, option_id);
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(option_value));
	}
	catch (const std::system_error& error) {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(error.code().value()));
	}

	return result;
}

mint::Reference mint_socket_set_option_number(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = mint::to_integer<int>(cursor, option);
		const auto option_value = mint::to_integer<int>(cursor, value);
		mint_network::set_socket_option(socket_fd, option_id, option_value);
	}
	catch (const std::system_error& error) {
		return mint::create_number(error.code().value());
	}

	return {};
}

mint::Reference mint_socket_get_option_boolean(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::Reference result = mint::create_iterator(cursor.ast());

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = mint::to_integer<int>(cursor, option);
		const auto option_value = mint_network::get_socket_option<mint_network::sockopt_bool>(socket_fd, option_id);
		iterator_yield(cursor, result.data<mint::Iterator>(),
		    mint::create_boolean(option_value != mint_network::sockopt_false));
	}
	catch (const std::system_error& error) {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(error.code().value()));
	}

	return result;
}

mint::Reference mint_socket_set_option_boolean(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = to_integer<int>(cursor, option);
		const auto option_value = to_boolean(value) ? mint_network::sockopt_true : mint_network::sockopt_false;
		mint_network::set_socket_option(socket_fd, option_id, option_value);
	}
	catch (const std::system_error& error) {
		return mint::create_number(error.code().value());
	}

	return {};
}

mint::Reference mint_socket_get_option_linger(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::Reference result = mint::create_iterator(cursor.ast());

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = mint::to_integer<int>(cursor, option);
		auto option_value = mint_network::get_socket_option<std::unique_ptr<linger>>(socket_fd, option_id);
		iterator_yield(cursor, result.data<mint::Iterator>(),
		    mint::create_c_object(cursor.ast(), option_value.release()));
	}
	catch (const std::system_error& error) {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(error.code().value()));
	}

	return result;
}

mint::Reference mint_socket_set_option_linger(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = to_integer<int>(cursor, option);
		const auto* option_value = value.data<mint::LibObject<linger>>().ptr;
		mint_network::set_socket_option(socket_fd, option_id, *option_value);
	}
	catch (const std::system_error& error) {
		return mint::create_number(error.code().value());
	}

	return {};
}

mint::Reference mint_socket_get_option_timeval(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::Reference result = mint::create_iterator(cursor.ast());

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = mint::to_integer<int>(cursor, option);
		auto option_value = mint_network::get_socket_option<std::unique_ptr<timeval>>(socket_fd, option_id);
		iterator_yield(cursor, result.data<mint::Iterator>(),
		    mint::create_c_object(cursor.ast(), option_value.release()));
	}
	catch (const std::system_error& error) {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(error.code().value()));
	}

	return result;
}

mint::Reference mint_socket_set_option_timeval(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	try {
		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto option_id = mint::to_integer<int>(cursor, option);
		const auto* option_value = value.data<mint::LibObject<timeval>>().ptr;
		mint_network::set_socket_option(socket_fd, option_id, *option_value);
	}
	catch (const std::system_error& error) {
		return mint::create_number(error.code().value());
	}

	return {};
}

mint::Reference mint_socket_bind(mint::Cursor& /*cursor*/, const mint::Reference& socket,
    const mint::Reference& endpoint) {
	try {

		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto [address, address_length] = mint_network::to_sockaddr(endpoint);

		if (::bind(socket_fd, address, address_length) != 0) {
			return mint::create_number(mint_network::errno_from_socket_last_error());
		}

		return {};
	}
	catch (const std::system_error& error) {
		return mint::create_number(error.code().value());
	}
}

mint::Reference mint_socket_connect(mint::FunctionHelper& helper, const mint::Reference& socket,
    mint::Reference& endpoint) {

	auto io_status = helper.reference(mint_network::symbols::network)
	                     .member(mint_network::symbols::socket)
	                     .member(mint_network::symbols::io_status);

	try {

		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto [address, address_length] = mint_network::to_sockaddr(endpoint);

		mint_network::SocketManager::instance().set_socket_listening(socket_fd, false);

		mint::unlock_processor();
		const auto connect_result = ::connect(socket_fd, address, address_length);
		mint::lock_processor();

		if (connect_result == 0) {
			return mint::create_iterator_from(helper.cursor(),
			    io_status.member(mint_network::symbols::io_success).share());
		}
		switch (const int error = mint_network::errno_from_socket_last_error()) {
		case EINPROGRESS:
		case EWOULDBLOCK:
			mint_network::SocketManager::instance().set_socket_blocked(socket_fd, true);
			return mint::create_iterator_from(helper.cursor(),
			    io_status.member(mint_network::symbols::io_would_block).share());
		default:
			return mint::create_iterator_from(helper.cursor(),
			    io_status.member(mint_network::symbols::io_error).share(), mint::create_number(error));
		}
	}
	catch (const std::system_error& error) {
		mint::create_iterator_from(helper.cursor(), io_status.member(mint_network::symbols::io_error).share(),
		    mint::create_number(error.code().value()));
	}
	return {};
}

mint::Reference mint_socket_connect_async(mint::FunctionHelper& helper, const mint::Reference& self,
    const mint::Reference& socket, mint::Reference& endpoint) {

	class AsyncConnectOperation : public mint::MintAsyncOperation {
		std::reference_wrapper<mint::Cursor> _cursor;
		mint::Reference _io_status;
		const sockaddr* _remote_address;
		socklen_t _remote_address_length;
	public:
		AsyncConnectOperation(mint::FunctionHelper& helper, mint::Reference self, SOCKET socket_fd,
		    const sockaddr* address, socklen_t address_length) :
		    mint::MintAsyncOperation(std::move(self), std::bit_cast<mint::handle_t>(socket_fd)),
		    _cursor(helper.cursor()),
		    _io_status(helper.reference(mint_network::symbols::network)
		            .member(mint_network::symbols::socket)
		            .member(mint_network::symbols::io_status)
		            .get()),
		    _remote_address(address),
		    _remote_address_length(address_length) {}

		std::error_code start() override {

			const auto socket_fd = reinterpret_cast<SOCKET>(get_handle());

#ifdef MINT_ASYNC_BACKEND_IOCP
			try {

				LPFN_CONNECTEX ConnectEx = nullptr;
				GUID guid = WSAID_CONNECTEX;
				DWORD bytes = 0;

				if (WSAIoctl(socket_fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &ConnectEx,
				        sizeof(ConnectEx), &bytes, nullptr, nullptr)
				    == SOCKET_ERROR) {
					return mint_network::last_socket_error_code();
				}

				auto [local_address, local_address_length] = make_local_endpoint_for_connect(*_remote_address);
				if (::bind(socket_fd, reinterpret_cast<sockaddr*>(&local_address), local_address_length) != 0) {
					return mint_network::last_socket_error_code();
				}

				if (!ConnectEx(socket_fd, _remote_address, _remote_address_length, nullptr, 0, nullptr, this)) {
					switch (WSAGetLastError()) {
					case WSA_IO_PENDING:
						break;
					default:
						return mint_network::last_socket_error_code();
					}
				}
			}
			catch (const std::system_error& error) {
				return error.code();
			}
#elifdef MINT_ASYNC_BACKEND_KQUEUE
			filter = EVFILT_WRITE;
			if (pending) {
				int socket_error = 0;
				socklen_t socket_error_length = sizeof(socket_error);
				if (getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &socket_error, &socket_error_length) < 0) {
					return mint::last_error_code();
				}
				if (socket_error != 0) {
					return {socket_error, std::generic_category()};
				}
				result = 0;
				return {};
			}
			if (connect(socket_fd, _remote_address, _remote_address_length) < 0) {
				const auto error = errno;
				if (error != EINPROGRESS && error != EALREADY && error != EWOULDBLOCK) {
					return mint::last_error_code();
				}
				return {EAGAIN, std::generic_category()};
			}
#elifdef MINT_OS_LINUX
			return std::visit(mint::Overloaded {
#ifdef MINT_ASYNC_BACKEND_IO_URING
			                      [&](mint::IoUringOperation& self) -> std::error_code {
				                      io_uring_prep_connect(self.sqe, socket_fd, _remote_address,
				                          _remote_address_length);
				                      return {};
			                      },
#endif
#ifdef MINT_ASYNC_BACKEND_EPOLL
			                      [&](mint::EPollOperation& self) -> std::error_code {
				                      self.events = EPOLLOUT;
				                      if (self.started) {
					                      int socket_error = 0;
					                      socklen_t socket_error_length = sizeof(socket_error);
					                      if (getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &socket_error,
					                              &socket_error_length)
					                          < 0) {
						                      return mint::last_error_code();
					                      }
					                      if (socket_error != 0) {
						                      return {socket_error, std::generic_category()};
					                      }
					                      self.result = 0;
					                      return {};
				                      }
				                      self.started = true;
				                      const auto _ = mint_network::SocketBlockingModeGuard<false>(socket_fd);
				                      if (connect(socket_fd, _remote_address, _remote_address_length) < 0) {
					                      const auto error = errno;
					                      if (error != EINPROGRESS && error != EALREADY && error != EWOULDBLOCK) {
						                      return mint::last_error_code();
					                      }
					                      return {EAGAIN, std::generic_category()};
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

		void complete(std::error_code error, std::size_t /*bytes_transferred*/) override {
			if (error) {
				done(mint::create_iterator_from(_cursor,
				    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(), mint_network::symbols::io_error),
				    mint::create_number(error.value())));
			}
			else {
#ifdef MINT_ASYNC_BACKEND_IOCP
				const auto socket_fd = reinterpret_cast<SOCKET>(get_handle());
				try {
					mint_network::set_socket_option(socket_fd, SO_UPDATE_CONNECT_CONTEXT, nullptr);
					done(mint::create_iterator_from(_cursor,
					    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
					        mint_network::symbols::io_success)));
				}
				catch (const std::system_error& error) {
					done(mint::create_iterator_from(_cursor,
					    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
					        mint_network::symbols::io_error),
					    mint::create_number(error.code().value())));
				}
#elifdef MINT_OS_LINUX
				done(mint::create_iterator_from(_cursor,
				    mint::get_global_ignore_visibility(_io_status.data<mint::Object>(),
				        mint_network::symbols::io_success)));
#else
#error "This operation is not implemented for this platform"
#endif
			}
		}
	};

	try {

		const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
		const auto [target, length] = mint_network::to_sockaddr(endpoint);

		return mint::create_iterator_from(helper.cursor(), mint::create_number(0),
		    mint::create_async_operation(helper.cursor().ast(),
		        new AsyncConnectOperation(helper, std::move(self), socket_fd, target, length)));
	}
	catch (const std::system_error& error) {
		return mint::create_iterator_from(helper.cursor(), mint::create_number(error.code().value()));
	}
}

mint::Reference mint_socket_finalize_connection(mint::FunctionHelper& helper, const mint::Reference& socket) {

	mint::Reference result = mint::create_iterator(helper.cursor().ast());

	int socket_error = EINVAL;
	const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
	auto io_status = helper.reference(mint_network::symbols::network)
	                     .member(mint_network::symbols::socket)
	                     .member(mint_network::symbols::io_status);

	try {
		socket_error = mint_network::get_socket_option<int>(socket_fd, SO_ERROR);
	}
	catch (const std::system_error& error) {
		socket_error = error.code().value();
	}

	switch (socket_error) {
	case 0:
		iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
		    io_status.member(mint_network::symbols::io_success).share());
		break;
	case EALREADY:
	case EINPROGRESS:
	case EWOULDBLOCK:
		iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
		    io_status.member(mint_network::symbols::io_would_block).share());
		mint_network::SocketManager::instance().set_socket_blocked(socket_fd, true);
		break;
	default:
		iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
		    io_status.member(mint_network::symbols::io_error).share());
		iterator_yield(helper.cursor(), result.data<mint::Iterator>(), mint::create_number(socket_error));
		break;
	}

	return result;
}

mint::Reference mint_socket_listen(mint::Cursor& cursor, const mint::Reference& socket, const mint::Reference& backlog) {

	const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));

	mint_network::SocketManager::instance().set_socket_listening(socket_fd, true);

	if (::listen(socket_fd, to_integer<int>(cursor, backlog)) != 0) {
		return mint::create_number(mint_network::errno_from_socket_last_error());
	}

	return {};
}

mint::Reference mint_socket_shutdown(mint::FunctionHelper& helper, const mint::Reference& socket) {

	mint::Reference result = mint::create_iterator(helper.cursor().ast());

#ifdef MINT_OS_WINDOWS
	const int how = SD_BOTH;
#else
	const int how = SHUT_RDWR;
#endif
	const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
	auto io_status = helper.reference(mint_network::symbols::network)
	                     .member(mint_network::symbols::socket)
	                     .member(mint_network::symbols::io_status);

	mint::unlock_processor();
	const auto shutdown_result = ::shutdown(socket_fd, how);
	mint::lock_processor();

	if (shutdown_result == 0) {
		iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
		    io_status.member(mint_network::symbols::io_success).share());
	}
	else {
		switch (const int error = mint_network::errno_from_socket_last_error()) {
		case EINPROGRESS:
		case EWOULDBLOCK:
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
			    io_status.member(mint_network::symbols::io_would_block).share());
			mint_network::SocketManager::instance().set_socket_blocked(socket_fd, true);
			break;
		case ENOTCONN:
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
			    io_status.member(mint_network::symbols::io_closed).share());
			break;
		default:
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
			    io_status.member(mint_network::symbols::io_error).share());
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(), mint::create_number(error));
			break;
		}
	}

	return result;
}

mint::Reference mint_socket_close(mint::Cursor& /*cursor*/, const mint::Reference& socket) {

	const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));

	try {
		mint_network::SocketManager::instance().close_socket(socket_fd);
	}
	catch (const std::system_error& error) {
		return mint::create_number(error.code().value());
	}

	return {};
}

mint::Reference mint_socket_get_error(mint::Cursor& /*cursor*/, const mint::Reference& socket) {
	try {
		return mint::create_number(
		    mint_network::get_socket_option<int>(std::bit_cast<SOCKET>(mint::to_handle(socket)), SO_ERROR));
	}
	catch (const std::system_error& error) {
		return mint::create_number(error.code().value());
	}
}

mint::Reference mint_socket_strerror(mint::Cursor& cursor, const mint::Reference& error) {
	return mint::create_string(cursor.ast(), strerror(to_integer<int>(cursor, error)));
}

mint::Reference mint_socket_linger_create(mint::Cursor& cursor, const mint::Reference& enabled,
    mint::Reference& linger_time) {
	return mint::create_c_object(cursor.ast(), new linger {
	                                               .l_onoff = to_boolean(enabled),
	                                               .l_linger = to_integer<u_short>(cursor, linger_time),
	                                           });
}

mint::Reference mint_socket_linger_delete(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	delete d_ptr.data<mint::LibObject<linger>>().ptr;
	return {};
}

mint::Reference mint_socket_linger_get_onoff(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	return mint::create_boolean(d_ptr.data<mint::LibObject<linger>>().ptr->l_onoff);
}

mint::Reference mint_socket_linger_set_onoff(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr,
    mint::Reference& enabled) {
	d_ptr.data<mint::LibObject<linger>>().ptr->l_onoff = to_boolean(enabled);
	return {};
}

mint::Reference mint_socket_linger_get_linger(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	return mint::create_signed_number(d_ptr.data<mint::LibObject<linger>>().ptr->l_linger);
}

mint::Reference mint_socket_linger_set_linger(mint::Cursor& cursor, const mint::Reference& d_ptr,
    mint::Reference& linger_time) {
	d_ptr.data<mint::LibObject<linger>>().ptr->l_linger = to_integer<u_short>(cursor, linger_time);
	return {};
}

mint::Reference mint_socket_timeval_create(mint::Cursor& cursor, const mint::Reference& sec,
    const mint::Reference& usec) {
	return mint::create_c_object(cursor.ast(), new timeval {
	                                               .tv_sec = to_integer<long>(cursor, sec),
	                                               .tv_usec = to_integer<long>(cursor, usec),
	                                           });
}

mint::Reference mint_socket_timeval_delete(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	delete d_ptr.data<mint::LibObject<timeval>>().ptr;
	return {};
}

mint::Reference mint_socket_timeval_get_sec(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	return mint::create_signed_number(d_ptr.data<mint::LibObject<timeval>>().ptr->tv_sec);
}

mint::Reference mint_socket_timeval_set_sec(mint::Cursor& cursor, const mint::Reference& d_ptr,
    const mint::Reference& sec) {
	d_ptr.data<mint::LibObject<timeval>>().ptr->tv_sec = to_integer<long>(cursor, sec);
	return {};
}

mint::Reference mint_socket_timeval_get_usec(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	return mint::create_signed_number(d_ptr.data<mint::LibObject<timeval>>().ptr->tv_usec);
}

mint::Reference mint_socket_timeval_set_usec(mint::Cursor& cursor, const mint::Reference& d_ptr,
    const mint::Reference& usec) {
	d_ptr.data<mint::LibObject<timeval>>().ptr->tv_usec = to_integer<long>(cursor, usec);
	return {};
}

}

MINT_EXPORT_FUNCTION(mint_socket_is_non_blocking, 1);
MINT_EXPORT_FUNCTION(mint_socket_set_non_blocking, 2);
MINT_EXPORT_FUNCTION(mint_socket_setup_options, 1);
MINT_EXPORT_FUNCTION(mint_socket_get_option_number, 2);
MINT_EXPORT_FUNCTION(mint_socket_set_option_number, 3);
MINT_EXPORT_FUNCTION(mint_socket_get_option_boolean, 2);
MINT_EXPORT_FUNCTION(mint_socket_set_option_boolean, 3);
MINT_EXPORT_FUNCTION(mint_socket_get_option_linger, 2);
MINT_EXPORT_FUNCTION(mint_socket_set_option_linger, 3);
MINT_EXPORT_FUNCTION(mint_socket_get_option_timeval, 2);
MINT_EXPORT_FUNCTION(mint_socket_set_option_timeval, 3);
MINT_EXPORT_FUNCTION(mint_socket_bind, 2);
MINT_EXPORT_FUNCTION(mint_socket_connect, 2);
MINT_EXPORT_FUNCTION(mint_socket_connect_async, 3);
MINT_EXPORT_FUNCTION(mint_socket_finalize_connection, 1);
MINT_EXPORT_FUNCTION(mint_socket_listen, 2);
MINT_EXPORT_FUNCTION(mint_socket_shutdown, 1);
MINT_EXPORT_FUNCTION(mint_socket_close, 1);
MINT_EXPORT_FUNCTION(mint_socket_get_error, 1);
MINT_EXPORT_FUNCTION(mint_socket_strerror, 1);

MINT_EXPORT_FUNCTION(mint_socket_linger_create, 2);
MINT_EXPORT_FUNCTION(mint_socket_linger_delete, 1);
MINT_EXPORT_FUNCTION(mint_socket_linger_get_onoff, 1);
MINT_EXPORT_FUNCTION(mint_socket_linger_set_onoff, 2);
MINT_EXPORT_FUNCTION(mint_socket_linger_get_linger, 1);
MINT_EXPORT_FUNCTION(mint_socket_linger_set_linger, 2);

MINT_EXPORT_FUNCTION(mint_socket_timeval_create, 2);
MINT_EXPORT_FUNCTION(mint_socket_timeval_delete, 1);
MINT_EXPORT_FUNCTION(mint_socket_timeval_get_sec, 1);
MINT_EXPORT_FUNCTION(mint_socket_timeval_set_sec, 2);
MINT_EXPORT_FUNCTION(mint_socket_timeval_get_usec, 1);
MINT_EXPORT_FUNCTION(mint_socket_timeval_set_usec, 2);
