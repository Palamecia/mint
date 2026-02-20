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

#include <array>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/casttool.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "scheduler.h"
#include "socket.h"
#include "ip.h"

#ifdef MINT_OS_WINDOWS
#include <Windows.h>
#include <in6addr.h>
#include <inaddr.h>
#include <minwindef.h>
#include <WinSock2.h>
#include <ws2ipdef.h>
#include <ws2tcpip.h>
#else
#ifdef MINT_OS_LINUX
#include <linux/ipv6.h>
#include <linux/in6.h>
#endif
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

int mint::get_ip_socket_info(const sockaddr* socket, socklen_t socketlen, std::string* sock_addr, u_short* sock_port) {

	switch (socket->sa_family) {
	case AF_INET:
		{
			std::array<char, INET_ADDRSTRLEN> buffer {};
			const auto* client = reinterpret_cast<const sockaddr_in*>(socket);
			if (const char* address = inet_ntop(socket->sa_family, &client->sin_addr, buffer.data(), buffer.size())) {
				*sock_addr = address;
				*sock_port = htons(client->sin_port);
			}
			else {
				return errno_from_io_last_error();
			}
		}
		break;
	case AF_INET6:
		{
			std::array<char, INET6_ADDRSTRLEN> buffer {};
			const auto* client = reinterpret_cast<const sockaddr_in6*>(socket);
			if (const char* address = inet_ntop(socket->sa_family, &client->sin6_addr, buffer.data(), buffer.size())) {
				*sock_addr = address;
				*sock_port = htons(client->sin6_port);
			}
			else {
				return errno_from_io_last_error();
			}
		}
		break;
	default:
		return EOPNOTSUPP;
	}

	return 0;
}

namespace {

mint::WeakReference mint_ip_socket_bind(mint::Cursor& cursor, const mint::Reference& socket,
    const mint::Reference& address, mint::Reference& port, const mint::Reference& ip_version) {

	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const std::string address_str = to_string(address);

	std::unique_ptr<sockaddr> serv_addr;
	socklen_t length = sizeof(sockaddr);

	switch (to_integer<int>(cursor, ip_version)) {
	case mint::ip_version_4:
		length = sizeof(sockaddr_in);
		serv_addr.reset(reinterpret_cast<sockaddr*>(new sockaddr_in));
		memset(serv_addr.get(), 0, length);
		reinterpret_cast<sockaddr_in*>(serv_addr.get())->sin_family = AF_INET;
		reinterpret_cast<sockaddr_in*>(serv_addr.get())->sin_port = htons(to_integer<std::uint16_t>(cursor, port));
		switch (::inet_pton(AF_INET, address_str.c_str(),
		    &reinterpret_cast<sockaddr_in*>(serv_addr.get())->sin_addr.s_addr)) {
		case 0:
			return mint::create_number(EINVAL);
		case 1:
			break;
		default:
			return mint::create_number(errno_from_io_last_error());
		}
		break;
	case mint::ip_version_6:
		length = sizeof(sockaddr_in6);
		serv_addr.reset(reinterpret_cast<sockaddr*>(new sockaddr_in6));
		memset(serv_addr.get(), 0, length);
		reinterpret_cast<sockaddr_in6*>(serv_addr.get())->sin6_family = AF_INET6;
		reinterpret_cast<sockaddr_in6*>(serv_addr.get())->sin6_port = htons(to_integer<std::uint16_t>(cursor, port));
		switch (::inet_pton(AF_INET6, address_str.c_str(),
		    &reinterpret_cast<sockaddr_in6*>(serv_addr.get())->sin6_addr.s6_addr)) {
		case 0:
			return mint::create_number(EINVAL);
		case 1:
			break;
		default:
			return mint::create_number(errno_from_io_last_error());
		}
		break;
	default:
		return mint::create_number(EOPNOTSUPP);
	}

	if (::bind(socket_fd, serv_addr.get(), length) != 0) {
		return mint::create_number(errno_from_io_last_error());
	}

	return {};
}

mint::WeakReference mint_ip_socket_connect(mint::FunctionHelper& helper, const mint::Reference& socket,
    mint::Reference& address, const mint::Reference& port, const mint::Reference& ip_version) {

	mint::WeakReference result = mint::create_iterator(helper.cursor().ast());

	const auto socket_fd = to_integer<SOCKET>(helper.cursor(), socket);
	const std::string address_str = to_string(address);
	auto io_status =
	    helper.reference(mint::symbols::network).member(mint::symbols::end_point).member(mint::symbols::io_status);

	std::unique_ptr<sockaddr> target;
	socklen_t length = sizeof(sockaddr);

	switch (to_integer<int>(helper.cursor(), ip_version)) {
	case mint::ip_version_4:
		length = sizeof(sockaddr_in);
		target.reset(reinterpret_cast<sockaddr*>(new sockaddr_in));
		memset(target.get(), 0, length);
		reinterpret_cast<sockaddr_in*>(target.get())->sin_family = AF_INET;
		reinterpret_cast<sockaddr_in*>(target.get())->sin_port = htons(to_integer<std::uint16_t>(helper.cursor(), port));
		switch (
		    ::inet_pton(AF_INET, address_str.c_str(), &reinterpret_cast<sockaddr_in*>(target.get())->sin_addr.s_addr)) {
		case 0:
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
			    io_status.member(mint::symbols::io_error).share());
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(), mint::create_number(EINVAL));
			return result;
		case 1:
			break;
		default:
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
			    io_status.member(mint::symbols::io_error).share());
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
			    mint::create_number(errno_from_io_last_error()));
			return result;
		}
		break;
	case mint::ip_version_6:
		length = sizeof(sockaddr_in6);
		target.reset(reinterpret_cast<sockaddr*>(new sockaddr_in6));
		memset(target.get(), 0, length);
		reinterpret_cast<sockaddr_in6*>(target.get())->sin6_family = AF_INET6;
		reinterpret_cast<sockaddr_in6*>(target.get())->sin6_port = htons(
		    to_integer<std::uint16_t>(helper.cursor(), port));
		switch (::inet_pton(AF_INET6, address_str.c_str(),
		    &reinterpret_cast<sockaddr_in6*>(target.get())->sin6_addr.s6_addr)) {
		case 0:
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
			    io_status.member(mint::symbols::io_error).share());
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(), mint::create_number(EINVAL));
			return result;
		case 1:
			break;
		default:
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
			    io_status.member(mint::symbols::io_error).share());
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
			    mint::create_number(errno_from_io_last_error()));
			return result;
		}
		break;
	default:
		iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
		    io_status.member(mint::symbols::io_error).share());
		iterator_yield(helper.cursor(), result.data<mint::Iterator>(), mint::create_number(EOPNOTSUPP));
		return result;
	}

	Scheduler::instance().set_socket_listening(socket_fd, false);

	if (::connect(socket_fd, target.get(), length) == 0) {
		iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
		    io_status.member(mint::symbols::io_success).share());
	}
	else {
		switch (const int error = errno_from_io_last_error()) {
		case EINPROGRESS:
		case EWOULDBLOCK:
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
			    io_status.member(mint::symbols::io_would_block).share());
			Scheduler::instance().set_socket_blocked(socket_fd, true);
			break;
		default:
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
			    io_status.member(mint::symbols::io_error).share());
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(), mint::create_number(error));
			break;
		}
	}

	return result;
}

mint::WeakReference mint_ip_socket_listen(mint::Cursor& cursor, const mint::Reference& socket,
    const mint::Reference& backlog) {

	const auto socket_fd = to_integer<SOCKET>(cursor, socket);

	Scheduler::instance().set_socket_listening(socket_fd, true);

	if (::listen(socket_fd, to_integer<int>(cursor, backlog)) != 0) {
		return mint::create_number(errno_from_io_last_error());
	}

	return {};
}

mint::WeakReference mint_ip_socket_accept(mint::Cursor& cursor, const mint::Reference& socket) {

	mint::WeakReference result = mint::create_iterator(cursor.ast());

	sockaddr cli_addr {};
	socklen_t cli_len = sizeof(cli_addr);
	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const SOCKET client_fd = ::accept(socket_fd, &cli_addr, &cli_len);

	if (client_fd != INVALID_SOCKET) {

		std::string address;
		u_short port = 0;

		if (const int error = mint::get_ip_socket_info(&cli_addr, cli_len, &address, &port)) {
			iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
			iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
			iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
			iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(error));
		}
		else {
			iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_unsigned_number(client_fd));
			iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_string(cursor.ast(), address));
			iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(port));
			Scheduler::instance().accept_socket(client_fd);
		}
	}
	else {
		switch (const int error = errno_from_io_last_error()) {
		case EINPROGRESS:
		case EWOULDBLOCK:
			Scheduler::instance().set_socket_blocked(socket_fd, true);
			break;
		default:
			iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
			iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
			iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
			iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(error));
			break;
		}
	}

	return result;
}

mint::WeakReference mint_socket_setup_ip_options(mint::Cursor& /*cursor*/, const mint::Reference& ip_socket_option) {

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

mint::WeakReference mint_socket_setup_ipv4_options(mint::Cursor& /*cursor*/,
    const mint::Reference& ip_v4_socket_option) {

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

mint::WeakReference mint_socket_get_ipv4_option_number(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::WeakReference result = mint::create_iterator(cursor.ast());

	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const auto option_id = to_integer<int>(cursor, option);
	int option_value = 0;

	if (mint::get_socket_option(socket_fd, IPPROTO_IP, option_id, &option_value)) {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(option_value));
	}
	else {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(errno_from_io_last_error()));
	}

	return result;
}

mint::WeakReference mint_socket_set_ipv4_option_number(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const auto option_id = to_integer<int>(cursor, option);
	const auto option_value = to_integer<int>(cursor, value);

	if (!mint::set_socket_option(socket_fd, IPPROTO_IP, option_id, option_value)) {
		return mint::create_number(errno_from_io_last_error());
	}

	return {};
}

mint::WeakReference mint_socket_get_ipv4_option_boolean(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::WeakReference result = mint::create_iterator(cursor.ast());

	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const auto option_id = to_integer<int>(cursor, option);
	mint::sockopt_bool option_value = mint::sockopt_false;

	if (get_socket_option(socket_fd, IPPROTO_IP, option_id, &option_value)) {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_boolean(option_value != mint::sockopt_false));
	}
	else {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(errno_from_io_last_error()));
	}

	return result;
}

mint::WeakReference mint_socket_set_ipv4_option_boolean(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const auto option_id = to_integer<int>(cursor, option);
	const mint::sockopt_bool option_value = to_boolean(value) ? mint::sockopt_true : mint::sockopt_false;

	if (!set_socket_option(socket_fd, IPPROTO_IP, option_id, option_value)) {
		return mint::create_number(errno_from_io_last_error());
	}

	return {};
}

mint::WeakReference mint_socket_get_ipv4_option_byte(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::WeakReference result = mint::create_iterator(cursor.ast());

	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const auto option_id = to_integer<int>(cursor, option);
	u_char option_value = 0;

	if (mint::get_socket_option(socket_fd, IPPROTO_IP, option_id, &option_value)) {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(option_value));
	}
	else {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(errno_from_io_last_error()));
	}

	return result;
}

mint::WeakReference mint_socket_set_ipv4_option_byte(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const auto option_id = to_integer<int>(cursor, option);
	const auto option_value = to_integer<u_char>(cursor, value);

	if (!mint::set_socket_option(socket_fd, IPPROTO_IP, option_id, option_value)) {
		return mint::create_number(errno_from_io_last_error());
	}

	return {};
}

mint::WeakReference mint_socket_get_ipv4_option_flag(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::WeakReference result = mint::create_iterator(cursor.ast());

	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const auto option_id = to_integer<int>(cursor, option);
	u_char option_value = 0;

	if (mint::get_socket_option(socket_fd, IPPROTO_IP, option_id, &option_value)) {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_boolean(option_value != 0));
	}
	else {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(errno_from_io_last_error()));
	}

	return result;
}

mint::WeakReference mint_socket_set_ipv4_option_flag(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const auto option_id = to_integer<int>(cursor, option);
	const u_char option_value = to_boolean(value) ? 1 : 0;

	if (!mint::set_socket_option(socket_fd, IPPROTO_IP, option_id, option_value)) {
		return mint::create_number(errno_from_io_last_error());
	}

	return {};
}

mint::WeakReference mint_socket_get_ipv4_option_addr(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::WeakReference result = mint::create_iterator(cursor.ast());

	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const auto option_id = to_integer<int>(cursor, option);
	in_addr option_value {};

	if (mint::get_socket_option(socket_fd, IPPROTO_IP, option_id, &option_value)) {
		std::array<char, INET_ADDRSTRLEN> buffer {};
		if (const char* address = inet_ntop(AF_INET, &option_value, buffer.data(), buffer.size())) {
			iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_string(cursor.ast(), address));
		}
		else {
			iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
			iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(errno_from_io_last_error()));
		}
	}
	else {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(errno_from_io_last_error()));
	}

	return result;
}

mint::WeakReference mint_socket_set_ipv4_option_addr(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const auto option_id = to_integer<int>(cursor, option);
	const std::string address_str = to_string(value);
	in_addr option_value {};

	switch (::inet_pton(AF_INET, address_str.c_str(), &option_value)) {
	case 0:
		return mint::create_number(EINVAL);
	case 1:
		if (!mint::set_socket_option(socket_fd, IPPROTO_IP, option_id, &option_value)) {
			return mint::create_number(errno_from_io_last_error());
		}
		return {};
	default:
		return mint::create_number(errno_from_io_last_error());
	}
}

mint::WeakReference mint_socket_get_ipv4_option_mreq(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::WeakReference result = create_iterator(cursor.ast());

	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const auto option_id = to_integer<int>(cursor, option);
	auto option_value = std::make_unique<ip_mreq>();

	if (mint::get_socket_option(socket_fd, IPPROTO_IP, option_id, option_value.get())) {
		iterator_yield(cursor, result.data<mint::Iterator>(),
		    mint::create_c_object(cursor.ast(), option_value.release()));
	}
	else {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(errno_from_io_last_error()));
	}

	return result;
}

mint::WeakReference mint_socket_set_ipv4_option_mreq(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const auto option_id = to_integer<int>(cursor, option);
	const ip_mreq* option_value = value.data<mint::LibObject<ip_mreq>>().ptr;

	if (!mint::set_socket_option(socket_fd, IPPROTO_IP, option_id, option_value)) {
		return mint::create_number(errno_from_io_last_error());
	}

	return {};
}

mint::WeakReference mint_socket_get_ipv4_option_mreq_source(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::WeakReference result = mint::create_iterator(cursor.ast());

	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const auto option_id = to_integer<int>(cursor, option);
	auto option_value = std::make_unique<ip_mreq_source>();

	if (mint::get_socket_option(socket_fd, IPPROTO_IP, option_id, option_value.get())) {
		iterator_yield(cursor, result.data<mint::Iterator>(),
		    mint::create_c_object(cursor.ast(), option_value.release()));
	}
	else {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(errno_from_io_last_error()));
	}

	return result;
}

mint::WeakReference mint_socket_set_ipv4_option_mreq_source(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const auto option_id = to_integer<int>(cursor, option);
	const ip_mreq_source* option_value = value.data<mint::LibObject<ip_mreq_source>>().ptr;

	if (!mint::set_socket_option(socket_fd, IPPROTO_IP, option_id, option_value)) {
		return mint::create_number(errno_from_io_last_error());
	}

	return {};
}

mint::WeakReference mint_socket_ipv4_mreq_create(mint::Cursor& cursor, const mint::Reference& imr_multiaddr,
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

mint::WeakReference mint_socket_ipv4_mreq_delete(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	delete d_ptr.data<mint::LibObject<ip_mreq>>().ptr;
	return {};
}

mint::WeakReference mint_socket_ipv4_mreq_get_multiaddr(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	std::array<char, INET_ADDRSTRLEN> buffer {};
	if (const char* address = inet_ntop(AF_INET, &d_ptr.data<mint::LibObject<ip_mreq>>().ptr->imr_multiaddr,
	        buffer.data(), buffer.size())) {
		return create_string(cursor.ast(), address);
	}
	return {};
}

mint::WeakReference mint_socket_ipv4_mreq_set_multiaddr(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr,
    const mint::Reference& address) {
	return mint::create_boolean(
	    inet_pton(AF_INET, to_string(address).c_str(), &d_ptr.data<mint::LibObject<ip_mreq>>().ptr->imr_multiaddr));
}

mint::WeakReference mint_socket_ipv4_mreq_get_interface(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	std::array<char, INET_ADDRSTRLEN> buffer {};
	if (const char* address = inet_ntop(AF_INET, &d_ptr.data<mint::LibObject<ip_mreq>>().ptr->imr_interface,
	        buffer.data(), buffer.size())) {
		return create_string(cursor.ast(), address);
	}
	return {};
}

mint::WeakReference mint_socket_ipv4_mreq_set_interface(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr,
    const mint::Reference& address) {
	return mint::create_boolean(
	    inet_pton(AF_INET, to_string(address).c_str(), &d_ptr.data<mint::LibObject<ip_mreq>>().ptr->imr_interface));
}

mint::WeakReference mint_socket_ipv4_mreq_source_create(mint::Cursor& cursor, const mint::Reference& imr_multiaddr,
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

mint::WeakReference mint_socket_ipv4_mreq_source_delete(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	delete d_ptr.data<mint::LibObject<ip_mreq_source>>().ptr;
	return {};
}

mint::WeakReference mint_socket_ipv4_mreq_source_get_multiaddr(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	std::array<char, INET_ADDRSTRLEN> buffer {};
	if (const char* address = inet_ntop(AF_INET, &d_ptr.data<mint::LibObject<ip_mreq_source>>().ptr->imr_multiaddr,
	        buffer.data(), buffer.size())) {
		return create_string(cursor.ast(), address);
	}
	return {};
}

mint::WeakReference mint_socket_ipv4_mreq_source_set_multiaddr(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr,
    const mint::Reference& address) {
	return mint::create_boolean(inet_pton(AF_INET, to_string(address).c_str(),
	    &d_ptr.data<mint::LibObject<ip_mreq_source>>().ptr->imr_multiaddr));
}

mint::WeakReference mint_socket_ipv4_mreq_source_get_sourceaddr(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	std::array<char, INET_ADDRSTRLEN> buffer {};
	if (const char* address = inet_ntop(AF_INET, &d_ptr.data<mint::LibObject<ip_mreq_source>>().ptr->imr_sourceaddr,
	        buffer.data(), buffer.size())) {
		return create_string(cursor.ast(), address);
	}
	return {};
}

mint::WeakReference mint_socket_ipv4_mreq_source_set_sourceaddr(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr,
    const mint::Reference& address) {
	return mint::create_boolean(inet_pton(AF_INET, to_string(address).c_str(),
	    &d_ptr.data<mint::LibObject<ip_mreq_source>>().ptr->imr_sourceaddr));
}

mint::WeakReference mint_socket_ipv4_mreq_source_get_interface(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	std::array<char, INET_ADDRSTRLEN> buffer {};
	if (const char* address = inet_ntop(AF_INET, &d_ptr.data<mint::LibObject<ip_mreq_source>>().ptr->imr_interface,
	        buffer.data(), buffer.size())) {
		return create_string(cursor.ast(), address);
	}
	return {};
}

mint::WeakReference mint_socket_ipv4_mreq_source_set_interface(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr,
    const mint::Reference& address) {
	return mint::create_boolean(inet_pton(AF_INET, to_string(address).c_str(),
	    &d_ptr.data<mint::LibObject<ip_mreq_source>>().ptr->imr_interface));
}

mint::WeakReference mint_socket_setup_ipv6_options(mint::Cursor& /*cursor*/,
    const mint::Reference& ip_v6_socket_option) {

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

mint::WeakReference mint_socket_get_ipv6_option_number(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::WeakReference result = mint::create_iterator(cursor.ast());

	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const auto option_id = to_integer<int>(cursor, option);
	int option_value = 0;

	if (mint::get_socket_option(socket_fd, IPPROTO_IPV6, option_id, &option_value)) {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(option_value));
	}
	else {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(errno_from_io_last_error()));
	}

	return result;
}

mint::WeakReference mint_socket_set_ipv6_option_number(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const auto option_id = to_integer<int>(cursor, option);
	const auto option_value = to_integer<int>(cursor, value);

	if (!mint::set_socket_option(socket_fd, IPPROTO_IPV6, option_id, option_value)) {
		return mint::create_number(errno_from_io_last_error());
	}

	return {};
}

mint::WeakReference mint_socket_get_ipv6_option_boolean(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::WeakReference result = mint::create_iterator(cursor.ast());

	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const auto option_id = to_integer<int>(cursor, option);
	mint::sockopt_bool option_value = mint::sockopt_false;

	if (get_socket_option(socket_fd, IPPROTO_IPV6, option_id, &option_value)) {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_boolean(option_value != mint::sockopt_false));
	}
	else {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(errno_from_io_last_error()));
	}

	return result;
}

mint::WeakReference mint_socket_set_ipv6_option_boolean(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const auto option_id = to_integer<int>(cursor, option);
	const mint::sockopt_bool option_value = to_boolean(value) ? mint::sockopt_true : mint::sockopt_false;

	if (!mint::set_socket_option(socket_fd, IPPROTO_IPV6, option_id, option_value)) {
		return mint::create_number(errno_from_io_last_error());
	}

	return {};
}

mint::WeakReference mint_socket_get_ipv6_option_addr(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::WeakReference result = mint::create_iterator(cursor.ast());

	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const auto option_id = to_integer<int>(cursor, option);
	auto option_value = std::make_unique<sockaddr_in6>();

	if (mint::get_socket_option(socket_fd, IPPROTO_IPV6, option_id, option_value.get())) {
		iterator_yield(cursor, result.data<mint::Iterator>(),
		    mint::create_c_object(cursor.ast(), option_value.release()));
	}
	else {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(errno_from_io_last_error()));
	}

	return result;
}

mint::WeakReference mint_socket_set_ipv6_option_addr(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const auto option_id = to_integer<int>(cursor, option);
	const sockaddr_in6* option_value = value.data<mint::LibObject<sockaddr_in6>>().ptr;

	if (!mint::set_socket_option(socket_fd, IPPROTO_IPV6, option_id, option_value)) {
		return mint::create_number(errno_from_io_last_error());
	}

	return {};
}

mint::WeakReference mint_socket_get_ipv6_option_mtuinfo(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::WeakReference result = mint::create_iterator(cursor.ast());

#if defined(MINT_OS_LINUX) && defined(__UAPI_DEF_IP6_MTUINFO)
	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const auto option_id = to_integer<int>(cursor, option);
	auto option_value = std::make_unique<ip6_mtuinfo>();

	if (mint::get_socket_option(socket_fd, IPPROTO_IPV6, option_id, option_value.get())) {
		iterator_yield(cursor, result.data<mint::Iterator>(),
		    mint::create_c_object(cursor.ast(), option_value.release()));
	}
	else {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(errno_from_io_last_error()));
	}
#else
	((void)option);
	((void)socket);
	iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
	iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(ENOTSUP));
#endif

	return result;
}

mint::WeakReference mint_socket_set_ipv6_option_mtuinfo(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

#if defined(MINT_OS_LINUX) && defined(__UAPI_DEF_IP6_MTUINFO)
	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const auto option_id = to_integer<int>(cursor, option);
	const ip6_mtuinfo* option_value = value.data<mint::LibObject<ip6_mtuinfo>>().ptr;

	if (!mint::set_socket_option(socket_fd, IPPROTO_IPV6, option_id, option_value)) {
		return mint::create_number(errno_from_io_last_error());
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

mint::WeakReference mint_socket_get_ipv6_option_mreq(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::WeakReference result = mint::create_iterator(cursor.ast());

	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const auto option_id = to_integer<int>(cursor, option);
	auto option_value = std::make_unique<ipv6_mreq>();

	if (mint::get_socket_option(socket_fd, IPPROTO_IPV6, option_id, option_value.get())) {
		iterator_yield(cursor, result.data<mint::Iterator>(),
		    mint::create_c_object(cursor.ast(), option_value.release()));
	}
	else {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(errno_from_io_last_error()));
	}

	return result;
}

mint::WeakReference mint_socket_set_ipv6_option_mreq(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const auto option_id = to_integer<int>(cursor, option);
	const ipv6_mreq* option_value = value.data<mint::LibObject<ipv6_mreq>>().ptr;

	if (!mint::set_socket_option(socket_fd, IPPROTO_IPV6, option_id, option_value)) {
		return mint::create_number(errno_from_io_last_error());
	}

	return {};
}

mint::WeakReference mint_socket_ipv6_mreq_create(mint::Cursor& cursor, const mint::Reference& ipv6mr_multiaddr,
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

mint::WeakReference mint_socket_ipv6_mreq_delete(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	delete d_ptr.data<mint::LibObject<ipv6_mreq>>().ptr;
	return {};
}

mint::WeakReference mint_socket_ipv6_mreq_get_multiaddr(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	std::array<char, INET_ADDRSTRLEN> buffer {};
	if (const char* address = inet_ntop(AF_INET6, &d_ptr.data<mint::LibObject<ipv6_mreq>>().ptr->ipv6mr_multiaddr,
	        buffer.data(), buffer.size())) {
		return create_string(cursor.ast(), address);
	}
	return {};
}

mint::WeakReference mint_socket_ipv6_mreq_set_multiaddr(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr,
    const mint::Reference& address) {
	return mint::create_boolean(inet_pton(AF_INET6, to_string(address).c_str(),
	    &d_ptr.data<mint::LibObject<ipv6_mreq>>().ptr->ipv6mr_multiaddr));
}

mint::WeakReference mint_socket_ipv6_mreq_get_interface(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
#ifdef MINT_OS_WINDOWS
	return mint::create_number(d_ptr.data<mint::LibObject<ipv6_mreq>>().ptr->ipv6mr_interface);
#else
	return mint::create_number(d_ptr.data<mint::LibObject<ipv6_mreq>>().ptr->ipv6mr_ifindex);
#endif
}

mint::WeakReference mint_socket_ipv6_mreq_set_interface(mint::Cursor& cursor, const mint::Reference& d_ptr,
    mint::Reference& index) {
#ifdef MINT_OS_WINDOWS
	d_ptr.data<mint::LibObject<ipv6_mreq>>().ptr->ipv6mr_interface = to_integer<ULONG>(cursor, index);
#else
	d_ptr.data<mint::LibObject<ipv6_mreq>>().ptr->ipv6mr_ifindex = to_integer<int>(cursor, index);
#endif
	return {};
}

}

MINT_EXPORT_FUNCTION(mint_ip_socket_bind, 4);
MINT_EXPORT_FUNCTION(mint_ip_socket_connect, 4);
MINT_EXPORT_FUNCTION(mint_ip_socket_listen, 2);
MINT_EXPORT_FUNCTION(mint_ip_socket_accept, 1);
MINT_EXPORT_FUNCTION(mint_socket_setup_ip_options, 1);

MINT_EXPORT_FUNCTION(mint_socket_setup_ipv4_options, 1);
MINT_EXPORT_FUNCTION(mint_socket_get_ipv4_option_number, 2);
MINT_EXPORT_FUNCTION(mint_socket_set_ipv4_option_number, 3);
MINT_EXPORT_FUNCTION(mint_socket_get_ipv4_option_boolean, 2);
MINT_EXPORT_FUNCTION(mint_socket_set_ipv4_option_boolean, 3);
MINT_EXPORT_FUNCTION(mint_socket_get_ipv4_option_byte, 2);
MINT_EXPORT_FUNCTION(mint_socket_set_ipv4_option_byte, 3);
MINT_EXPORT_FUNCTION(mint_socket_get_ipv4_option_flag, 2);
MINT_EXPORT_FUNCTION(mint_socket_set_ipv4_option_flag, 3);
MINT_EXPORT_FUNCTION(mint_socket_get_ipv4_option_addr, 2);
MINT_EXPORT_FUNCTION(mint_socket_set_ipv4_option_addr, 3);
MINT_EXPORT_FUNCTION(mint_socket_get_ipv4_option_mreq, 2);
MINT_EXPORT_FUNCTION(mint_socket_set_ipv4_option_mreq, 3);
MINT_EXPORT_FUNCTION(mint_socket_get_ipv4_option_mreq_source, 2);
MINT_EXPORT_FUNCTION(mint_socket_set_ipv4_option_mreq_source, 3);

MINT_EXPORT_FUNCTION(mint_socket_ipv4_mreq_create, 2);
MINT_EXPORT_FUNCTION(mint_socket_ipv4_mreq_delete, 1);
MINT_EXPORT_FUNCTION(mint_socket_ipv4_mreq_get_multiaddr, 1);
MINT_EXPORT_FUNCTION(mint_socket_ipv4_mreq_set_multiaddr, 2);
MINT_EXPORT_FUNCTION(mint_socket_ipv4_mreq_get_interface, 1);
MINT_EXPORT_FUNCTION(mint_socket_ipv4_mreq_set_interface, 2);

MINT_EXPORT_FUNCTION(mint_socket_ipv4_mreq_source_create, 3);
MINT_EXPORT_FUNCTION(mint_socket_ipv4_mreq_source_delete, 1);
MINT_EXPORT_FUNCTION(mint_socket_ipv4_mreq_source_get_multiaddr, 1);
MINT_EXPORT_FUNCTION(mint_socket_ipv4_mreq_source_set_multiaddr, 2);
MINT_EXPORT_FUNCTION(mint_socket_ipv4_mreq_source_get_sourceaddr, 1);
MINT_EXPORT_FUNCTION(mint_socket_ipv4_mreq_source_set_sourceaddr, 2);
MINT_EXPORT_FUNCTION(mint_socket_ipv4_mreq_source_get_interface, 1);
MINT_EXPORT_FUNCTION(mint_socket_ipv4_mreq_source_set_interface, 2);

MINT_EXPORT_FUNCTION(mint_socket_setup_ipv6_options, 1);
MINT_EXPORT_FUNCTION(mint_socket_get_ipv6_option_number, 2);
MINT_EXPORT_FUNCTION(mint_socket_set_ipv6_option_number, 3);
MINT_EXPORT_FUNCTION(mint_socket_get_ipv6_option_boolean, 2);
MINT_EXPORT_FUNCTION(mint_socket_set_ipv6_option_boolean, 3);
MINT_EXPORT_FUNCTION(mint_socket_get_ipv6_option_addr, 2);
MINT_EXPORT_FUNCTION(mint_socket_set_ipv6_option_addr, 3);
MINT_EXPORT_FUNCTION(mint_socket_get_ipv6_option_mtuinfo, 2);
MINT_EXPORT_FUNCTION(mint_socket_set_ipv6_option_mtuinfo, 3);
MINT_EXPORT_FUNCTION(mint_socket_get_ipv6_option_mreq, 2);
MINT_EXPORT_FUNCTION(mint_socket_set_ipv6_option_mreq, 3);

MINT_EXPORT_FUNCTION(mint_socket_ipv6_mreq_create, 2);
MINT_EXPORT_FUNCTION(mint_socket_ipv6_mreq_delete, 1);
MINT_EXPORT_FUNCTION(mint_socket_ipv6_mreq_get_multiaddr, 1);
MINT_EXPORT_FUNCTION(mint_socket_ipv6_mreq_set_multiaddr, 2);
MINT_EXPORT_FUNCTION(mint_socket_ipv6_mreq_get_interface, 1);
MINT_EXPORT_FUNCTION(mint_socket_ipv6_mreq_set_interface, 2);
