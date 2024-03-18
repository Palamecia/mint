/**
 * Copyright (c) 2024 Gauvain CHERY.
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

#include <mint/memory/functiontool.h>
#include <mint/memory/casttool.h>
#include "scheduler.h"
#include "socket.h"
#include "ip.h"

#ifdef OS_LINUX
#include <linux/ipv6.h>
#endif

#ifdef OS_UNIX
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

using namespace mint;
using namespace std;

int mint::get_ip_socket_info(const sockaddr *socket, socklen_t socketlen, string *sock_addr, u_short *sock_port) {

	switch (socket->sa_family) {
	case AF_INET:
		{
			char buffer[INET_ADDRSTRLEN];
			const sockaddr_in *client = reinterpret_cast<const sockaddr_in *>(socket);
			if (const char *address = inet_ntop(socket->sa_family, &client->sin_addr, buffer, sizeof(buffer))) {
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
			char buffer[INET6_ADDRSTRLEN];
			const sockaddr_in6 *client = reinterpret_cast<const sockaddr_in6 *>(socket);
			if (const char *address = inet_ntop(socket->sa_family, &client->sin6_addr, buffer, sizeof(buffer))) {
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

MINT_FUNCTION(mint_ip_socket_bind, 4, cursor) {

	FunctionHelper helper(cursor, 4);
	Reference &ip_version = helper.pop_parameter();
	Reference &port = helper.pop_parameter();
	Reference &address = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const string address_str = to_string(address);

	unique_ptr<sockaddr> serv_addr;
	socklen_t length = sizeof(sockaddr);

	switch (to_integer(cursor, ip_version)) {
	case 4:
		length = sizeof(sockaddr_in);
		serv_addr.reset(reinterpret_cast<sockaddr *>(new sockaddr_in));
		memset(serv_addr.get(), 0, length);
		reinterpret_cast<sockaddr_in *>(serv_addr.get())->sin_family = AF_INET;
		reinterpret_cast<sockaddr_in *>(serv_addr.get())->sin_port = htons(static_cast<uint16_t>(to_integer(cursor, port)));
		switch (::inet_pton(AF_INET, address_str.c_str(), &reinterpret_cast<sockaddr_in *>(serv_addr.get())->sin_addr.s_addr)) {
		case 0:
			helper.return_value(create_number(EINVAL));
			return;
		case 1:
			break;
		default:
			helper.return_value(create_number(errno_from_io_last_error()));
			return;
		}
		break;
	case 6:
		length = sizeof(sockaddr_in6);
		serv_addr.reset(reinterpret_cast<sockaddr *>(new sockaddr_in6));
		memset(serv_addr.get(), 0, length);
		reinterpret_cast<sockaddr_in6 *>(serv_addr.get())->sin6_family = AF_INET6;
		reinterpret_cast<sockaddr_in6 *>(serv_addr.get())->sin6_port = htons(static_cast<uint16_t>(to_integer(cursor, port)));
		switch (::inet_pton(AF_INET6, address_str.c_str(), &reinterpret_cast<sockaddr_in6 *>(serv_addr.get())->sin6_addr.s6_addr)) {
		case 0:
			helper.return_value(create_number(EINVAL));
			return;
		case 1:
			break;
		default:
			helper.return_value(create_number(errno_from_io_last_error()));
			return;
		}
		break;
	default:
		helper.return_value(create_number(EOPNOTSUPP));
		return;
	}

	if (::bind(socket_fd, serv_addr.get(), length) != 0) {
		helper.return_value(create_number(errno_from_io_last_error()));
	}
}

MINT_FUNCTION(mint_ip_socket_connect, 4, cursor) {

	FunctionHelper helper(cursor, 4);
	Reference &ip_version = helper.pop_parameter();
	Reference &port = helper.pop_parameter();
	Reference &address = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();
	WeakReference result = create_iterator();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const string address_str = to_string(address);
	auto IOStatus = helper.reference(symbols::Network).member(symbols::EndPoint).member(symbols::IOStatus);

	unique_ptr<sockaddr> target;
	socklen_t length = sizeof(sockaddr);

	switch (to_integer(cursor, ip_version)) {
	case 4:
		length = sizeof(sockaddr_in);
		target.reset(reinterpret_cast<sockaddr *>(new sockaddr_in));
		memset(target.get(), 0, length);
		reinterpret_cast<sockaddr_in *>(target.get())->sin_family = AF_INET;
		reinterpret_cast<sockaddr_in *>(target.get())->sin_port = htons(static_cast<uint16_t>(to_integer(cursor, port)));
		switch (::inet_pton(AF_INET, address_str.c_str(), &reinterpret_cast<sockaddr_in *>(target.get())->sin_addr.s_addr)) {
		case 0:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOError));
			iterator_insert(result.data<Iterator>(), create_number(EINVAL));
			helper.return_value(std::move(result));
			return;
		case 1:
			break;
		default:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOError));
			iterator_insert(result.data<Iterator>(), create_number(errno_from_io_last_error()));
			helper.return_value(std::move(result));
			return;
		}
		break;
	case 6:
		length = sizeof(sockaddr_in6);
		target.reset(reinterpret_cast<sockaddr *>(new sockaddr_in6));
		memset(target.get(), 0, length);
		reinterpret_cast<sockaddr_in6 *>(target.get())->sin6_family = AF_INET6;
		reinterpret_cast<sockaddr_in6 *>(target.get())->sin6_port = htons(static_cast<uint16_t>(to_integer(cursor, port)));
		switch (::inet_pton(AF_INET6, address_str.c_str(), &reinterpret_cast<sockaddr_in6 *>(target.get())->sin6_addr.s6_addr)) {
		case 0:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOError));
			iterator_insert(result.data<Iterator>(), create_number(EINVAL));
			helper.return_value(std::move(result));
			return;
		case 1:
			break;
		default:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOError));
			iterator_insert(result.data<Iterator>(), create_number(errno_from_io_last_error()));
			helper.return_value(std::move(result));
			return;
		}
		break;
	default:
		iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOError));
		iterator_insert(result.data<Iterator>(), create_number(EOPNOTSUPP));
		helper.return_value(std::move(result));
		return;
	}
	
	Scheduler::instance().set_socket_listening(socket_fd, false);

	if (::connect(socket_fd, target.get(), length) == 0) {
		iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOSuccess));
	}
	else {
		switch (const int error = errno_from_io_last_error()) {
		case EINPROGRESS:
		case EWOULDBLOCK:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOWouldBlock));
			Scheduler::instance().set_socket_blocked(socket_fd, true);
			break;
		default:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOError));
			iterator_insert(result.data<Iterator>(), create_number(error));
			break;
		}
	}
	
	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_ip_socket_listen, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &backlog = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();

	SOCKET socket_fd = to_integer(cursor, socket);
	
	Scheduler::instance().set_socket_listening(socket_fd, true);

	if (::listen(socket_fd, static_cast<int>(to_integer(cursor, backlog))) != 0) {
		helper.return_value(create_number(errno_from_io_last_error()));
	}
}

MINT_FUNCTION(mint_ip_socket_accept, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &socket = helper.pop_parameter();
	WeakReference result = create_iterator();

	sockaddr cli_addr;
	socklen_t cli_len = sizeof(cli_addr);
	const SOCKET socket_fd = to_integer(cursor, socket);
	const SOCKET client_fd = ::accept(socket_fd, &cli_addr, &cli_len);

	if (client_fd != INVALID_SOCKET) {

		string address;
		u_short port;

		if (const int error = get_ip_socket_info(&cli_addr, cli_len, &address, &port)) {
			iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
			iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
			iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
			iterator_insert(result.data<Iterator>(), create_number(error));
		}
		else {
			iterator_insert(result.data<Iterator>(), create_number(client_fd));
			iterator_insert(result.data<Iterator>(), create_string(address));
			iterator_insert(result.data<Iterator>(), create_number(port));
			Scheduler::instance().accept_socket(client_fd);
		}
	}
	else {
		switch (int error = errno_from_io_last_error()) {
		case EINPROGRESS:
		case EWOULDBLOCK:
			Scheduler::instance().set_socket_blocked(socket_fd, true);
			break;
		default:
			iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
			iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
			iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
			iterator_insert(result.data<Iterator>(), create_number(error));
			break;
		}
	}
	
	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_socket_setup_ip_options, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &IpSocketOption = helper.pop_parameter();

#define BIND_MCAST_VALUE(_enum, _option) \
	_enum.data<Object>()->metadata->globals()[#_option]->value.data<Number>()->value = _option
#define BIND_MCAST_DISABLE(_enum, _option) \
	_enum.data<Object>()->metadata->globals()[#_option]->value.move(WeakReference::create<None>())

#ifdef MCAST_JOIN_GROUP
	BIND_MCAST_VALUE(IpSocketOption, MCAST_JOIN_GROUP);
#else
	BIND_MCAST_DISABLE(IpSocketOption, MCAST_JOIN_GROUP);
#endif
#ifdef MCAST_LEAVE_GROUP
	BIND_MCAST_VALUE(IpSocketOption, MCAST_LEAVE_GROUP);
#else
	BIND_MCAST_DISABLE(IpSocketOption, MCAST_LEAVE_GROUP);
#endif
#ifdef MCAST_BLOCK_SOURCE
	BIND_MCAST_VALUE(IpSocketOption, MCAST_BLOCK_SOURCE);
#else
	BIND_MCAST_DISABLE(IpSocketOption, MCAST_BLOCK_SOURCE);
#endif
#ifdef MCAST_UNBLOCK_SOURCE
	BIND_MCAST_VALUE(IpSocketOption, MCAST_UNBLOCK_SOURCE);
#else
	BIND_MCAST_DISABLE(IpSocketOption, MCAST_UNBLOCK_SOURCE);
#endif
#ifdef MCAST_JOIN_SOURCE_GROUP
	BIND_MCAST_VALUE(IpSocketOption, MCAST_JOIN_SOURCE_GROUP);
#else
	BIND_MCAST_DISABLE(IpSocketOption, MCAST_JOIN_SOURCE_GROUP);
#endif
#ifdef MCAST_LEAVE_SOURCE_GROUP
	BIND_MCAST_VALUE(IpSocketOption, MCAST_LEAVE_SOURCE_GROUP);
#else
	BIND_MCAST_DISABLE(IpSocketOption, MCAST_LEAVE_SOURCE_GROUP);
#endif
}

MINT_FUNCTION(mint_socket_setup_ipv4_options, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &IpV4SocketOption = helper.pop_parameter();

#define BIND_IP_VALUE(_enum, _option) \
	_enum.data<Object>()->metadata->globals()[#_option]->value.data<Number>()->value = IP_##_option
#define BIND_IP_DISABLE(_enum, _option) \
	_enum.data<Object>()->metadata->globals()[#_option]->value.move_data(WeakReference::create<None>())

#ifdef IP_HEADERSINCL
	BIND_IP_VALUE(IpV4SocketOption, HDRINCL);
#else
	BIND_IP_DISABLE(IpV4SocketOption, HDRINCL);
#endif
#ifdef IP_OPTIONS
	BIND_IP_VALUE(IpV4SocketOption, OPTIONS);
#else
	BIND_IP_DISABLE(IpV4SocketOption, OPTIONS);
#endif
#ifdef IP_RECVDSTADDR
	BIND_IP_VALUE(IpV4SocketOption, RECVDSTADDR);
#else
	BIND_IP_DISABLE(IpV4SocketOption, RECVDSTADDR);
#endif
#ifdef IP_RECVIF
	BIND_IP_VALUE(IpV4SocketOption, RECVIF);
#else
	BIND_IP_DISABLE(IpV4SocketOption, RECVIF);
#endif
#ifdef IP_TOS
	BIND_IP_VALUE(IpV4SocketOption, TOS);
#else
	BIND_IP_DISABLE(IpV4SocketOption, TOS);
#endif
#ifdef IP_TTL
	BIND_IP_VALUE(IpV4SocketOption, TTL);
#else
	BIND_IP_DISABLE(IpV4SocketOption, TTL);
#endif
#ifdef IP_MULTICAST_IF
	BIND_IP_VALUE(IpV4SocketOption, MULTICAST_IF);
#else
	BIND_IP_DISABLE(IpV4SocketOption, MULTICAST_IF);
#endif
#ifdef IP_MULTICAST_TTL
	BIND_IP_VALUE(IpV4SocketOption, MULTICAST_TTL);
#else
	BIND_IP_DISABLE(IpV4SocketOption, MULTICAST_TTL);
#endif
#ifdef IP_MULTICAST_LOOP
	BIND_IP_VALUE(IpV4SocketOption, MULTICAST_LOOP);
#else
	BIND_IP_DISABLE(IpV4SocketOption, MULTICAST_LOOP);
#endif
#ifdef IP_ADD_MEMBERSHIP
	BIND_IP_VALUE(IpV4SocketOption, ADD_MEMBERSHIP);
#else
	BIND_IP_DISABLE(IpV4SocketOption, ADD_MEMBERSHIP);
#endif
#ifdef IP_DROP_MEMBERSHIP
	BIND_IP_VALUE(IpV4SocketOption, DROP_MEMBERSHIP);
#else
	BIND_IP_DISABLE(IpV4SocketOption, DROP_MEMBERSHIP);
#endif
#ifdef IP_BLOCK_SOURCE
	BIND_IP_VALUE(IpV4SocketOption, BLOCK_SOURCE);
#else
	BIND_IP_DISABLE(IpV4SocketOption, BLOCK_SOURCE);
#endif
#ifdef IP_UNBLOCK_SOURCE
	BIND_IP_VALUE(IpV4SocketOption, UNBLOCK_SOURCE);
#else
	BIND_IP_DISABLE(IpV4SocketOption, UNBLOCK_SOURCE);
#endif
#ifdef IP_ADD_SOURCE_MEMBERSHIP
	BIND_IP_VALUE(IpV4SocketOption, ADD_SOURCE_MEMBERSHIP);
#else
	BIND_IP_DISABLE(IpV4SocketOption, ADD_SOURCE_MEMBERSHIP);
#endif
#ifdef IP_DROP_SOURCE_MEMBERSHIP
	BIND_IP_VALUE(IpV4SocketOption, DROP_SOURCE_MEMBERSHIP);
#else
	BIND_IP_DISABLE(IpV4SocketOption, DROP_SOURCE_MEMBERSHIP);
#endif
}

MINT_FUNCTION(mint_socket_get_ipv4_option_number, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();
	WeakReference result = create_iterator();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	int option_value = 0;

	if (get_socket_option(socket_fd, IPPROTO_IP, option_id, &option_value)) {
		iterator_insert(result.data<Iterator>(), create_number(option_value));
	}
	else {
		iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
		iterator_insert(result.data<Iterator>(), create_number(errno_from_io_last_error()));
	}
	
	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_socket_set_ipv4_option_number, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	Reference &value = helper.pop_parameter();
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	const int option_value = to_integer(cursor, value);

	if (!set_socket_option(socket_fd, IPPROTO_IP, option_id, option_value)) {
		helper.return_value(create_number(errno_from_io_last_error()));
	}
}

MINT_FUNCTION(mint_socket_get_ipv4_option_boolean, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();
	WeakReference result = create_iterator();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	sockopt_bool option_value = sockopt_false;

	if (get_socket_option(socket_fd, IPPROTO_IP, option_id, &option_value)) {
		iterator_insert(result.data<Iterator>(), create_boolean(option_value != sockopt_false));
	}
	else {
		iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
		iterator_insert(result.data<Iterator>(), create_number(errno_from_io_last_error()));
	}
	
	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_socket_set_ipv4_option_boolean, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	Reference &value = helper.pop_parameter();
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	const sockopt_bool option_value = to_boolean(cursor, value) ? sockopt_true : sockopt_false;

	if (!set_socket_option(socket_fd, IPPROTO_IP, option_id, option_value)) {
		helper.return_value(create_number(errno_from_io_last_error()));
	}
}

MINT_FUNCTION(mint_socket_get_ipv4_option_byte, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();
	WeakReference result = create_iterator();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	u_char option_value = 0;

	if (get_socket_option(socket_fd, IPPROTO_IP, option_id, &option_value)) {
		iterator_insert(result.data<Iterator>(), create_number(option_value));
	}
	else {
		iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
		iterator_insert(result.data<Iterator>(), create_number(errno_from_io_last_error()));
	}
	
	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_socket_set_ipv4_option_byte, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	Reference &value = helper.pop_parameter();
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	const u_char option_value = to_integer(cursor, value);

	if (!set_socket_option(socket_fd, IPPROTO_IP, option_id, option_value)) {
		helper.return_value(create_number(errno_from_io_last_error()));
	}
}

MINT_FUNCTION(mint_socket_get_ipv4_option_flag, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();
	WeakReference result = create_iterator();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	u_char option_value = 0;

	if (get_socket_option(socket_fd, IPPROTO_IP, option_id, &option_value)) {
		iterator_insert(result.data<Iterator>(), create_boolean(option_value != 0));
	}
	else {
		iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
		iterator_insert(result.data<Iterator>(), create_number(errno_from_io_last_error()));
	}
	
	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_socket_set_ipv4_option_flag, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	Reference &value = helper.pop_parameter();
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	const u_char option_value = to_boolean(cursor, value) ? 1 : 0;

	if (!set_socket_option(socket_fd, IPPROTO_IP, option_id, option_value)) {
		helper.return_value(create_number(errno_from_io_last_error()));
	}
}

MINT_FUNCTION(mint_socket_get_ipv4_option_addr, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();
	WeakReference result = create_iterator();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	in_addr option_value;

	if (get_socket_option(socket_fd, IPPROTO_IP, option_id, &option_value)) {
		char buffer[INET_ADDRSTRLEN];
		if (const char *address = inet_ntop(AF_INET, &option_value, buffer, sizeof(buffer))) {
			iterator_insert(result.data<Iterator>(), create_string(address));
		}
		else {
			iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
			iterator_insert(result.data<Iterator>(), create_number(errno_from_io_last_error()));
		}
	}
	else {
		iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
		iterator_insert(result.data<Iterator>(), create_number(errno_from_io_last_error()));
	}
	
	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_socket_set_ipv4_option_addr, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	Reference &value = helper.pop_parameter();
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	const string address_str = to_string(value);
	in_addr option_value;

	switch (::inet_pton(AF_INET, address_str.c_str(), &option_value)) {
	case 0:
		helper.return_value(create_number(EINVAL));
		break;
	case 1:
		if (!set_socket_option(socket_fd, IPPROTO_IP, option_id, &option_value)) {
			helper.return_value(create_number(errno_from_io_last_error()));
		}
		break;
	default:
		helper.return_value(create_number(errno_from_io_last_error()));
		break;
	}
}

MINT_FUNCTION(mint_socket_get_ipv4_option_mreq, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();
	WeakReference result = create_iterator();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	std::unique_ptr<ip_mreq> option_value(new ip_mreq);

	if (get_socket_option(socket_fd, IPPROTO_IP, option_id, option_value.get())) {
		iterator_insert(result.data<Iterator>(), create_object(option_value.release()));
	}
	else {
		iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
		iterator_insert(result.data<Iterator>(), create_number(errno_from_io_last_error()));
	}
	
	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_socket_set_ipv4_option_mreq, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	Reference &value = helper.pop_parameter();
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	const ip_mreq *option_value = value.data<LibObject<ip_mreq>>()->impl;

	if (!set_socket_option(socket_fd, IPPROTO_IP, option_id, option_value)) {
		helper.return_value(create_number(errno_from_io_last_error()));
	}
}

MINT_FUNCTION(mint_socket_get_ipv4_option_mreq_source, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();
	WeakReference result = create_iterator();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	std::unique_ptr<ip_mreq_source> option_value(new ip_mreq_source);

	if (get_socket_option(socket_fd, IPPROTO_IP, option_id, option_value.get())) {
		iterator_insert(result.data<Iterator>(), create_object(option_value.release()));
	}
	else {
		iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
		iterator_insert(result.data<Iterator>(), create_number(errno_from_io_last_error()));
	}
	
	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_socket_set_ipv4_option_mreq_source, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	Reference &value = helper.pop_parameter();
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	const ip_mreq_source *option_value = value.data<LibObject<ip_mreq_source>>()->impl;

	if (!set_socket_option(socket_fd, IPPROTO_IP, option_id, option_value)) {
		helper.return_value(create_number(errno_from_io_last_error()));
	}
}

MINT_FUNCTION(mint_socket_ipv4_mreq_create, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &imr_interface = helper.pop_parameter();
	Reference &imr_multiaddr = helper.pop_parameter();

	std::unique_ptr<ip_mreq> group(new ip_mreq);
	if (!inet_pton(AF_INET, to_string(imr_multiaddr).c_str(), &group->imr_multiaddr)) {
		return;
	}
	if (!inet_pton(AF_INET, to_string(imr_interface).c_str(), &group->imr_interface)) {
		return;
	}
	helper.return_value(create_object(group.release()));
}

MINT_FUNCTION(mint_socket_ipv4_mreq_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &d_ptr = helper.pop_parameter();

	delete d_ptr.data<LibObject<ip_mreq>>()->impl;
}

MINT_FUNCTION(mint_socket_ipv4_mreq_get_multiaddr, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &d_ptr = helper.pop_parameter();

	char buffer[INET_ADDRSTRLEN];
	if (const char *address = inet_ntop(AF_INET, &d_ptr.data<LibObject<ip_mreq>>()->impl->imr_multiaddr, buffer, sizeof(buffer))) {
		helper.return_value(create_string(address));
	}
}

MINT_FUNCTION(mint_socket_ipv4_mreq_set_multiaddr, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &address = helper.pop_parameter();
	Reference &d_ptr = helper.pop_parameter();
	
	helper.return_value(create_boolean(inet_pton(AF_INET, to_string(address).c_str(), &d_ptr.data<LibObject<ip_mreq>>()->impl->imr_multiaddr)));
}

MINT_FUNCTION(mint_socket_ipv4_mreq_get_interface, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &d_ptr = helper.pop_parameter();

	char buffer[INET_ADDRSTRLEN];
	if (const char *address = inet_ntop(AF_INET, &d_ptr.data<LibObject<ip_mreq>>()->impl->imr_interface, buffer, sizeof(buffer))) {
		helper.return_value(create_string(address));
	}
}

MINT_FUNCTION(mint_socket_ipv4_mreq_set_interface, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &address = helper.pop_parameter();
	Reference &d_ptr = helper.pop_parameter();
	
	helper.return_value(create_boolean(inet_pton(AF_INET, to_string(address).c_str(), &d_ptr.data<LibObject<ip_mreq>>()->impl->imr_interface)));
}

MINT_FUNCTION(mint_socket_ipv4_mreq_source_create, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	Reference &imr_interface = helper.pop_parameter();
	Reference &imr_sourceaddr = helper.pop_parameter();
	Reference &imr_multiaddr = helper.pop_parameter();

	std::unique_ptr<ip_mreq_source> group(new ip_mreq_source);
	if (!inet_pton(AF_INET, to_string(imr_multiaddr).c_str(), &group->imr_multiaddr)) {
		return;
	}
	if (!inet_pton(AF_INET, to_string(imr_sourceaddr).c_str(), &group->imr_sourceaddr)) {
		return;
	}
	if (!inet_pton(AF_INET, to_string(imr_interface).c_str(), &group->imr_interface)) {
		return;
	}
	helper.return_value(create_object(group.release()));
}

MINT_FUNCTION(mint_socket_ipv4_mreq_source_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &d_ptr = helper.pop_parameter();

	delete d_ptr.data<LibObject<ip_mreq_source>>()->impl;
}

MINT_FUNCTION(mint_socket_ipv4_mreq_source_get_multiaddr, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &d_ptr = helper.pop_parameter();

	char buffer[INET_ADDRSTRLEN];
	if (const char *address = inet_ntop(AF_INET, &d_ptr.data<LibObject<ip_mreq_source>>()->impl->imr_multiaddr, buffer, sizeof(buffer))) {
		helper.return_value(create_string(address));
	}
}

MINT_FUNCTION(mint_socket_ipv4_mreq_source_set_multiaddr, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &address = helper.pop_parameter();
	Reference &d_ptr = helper.pop_parameter();
	
	helper.return_value(create_boolean(inet_pton(AF_INET, to_string(address).c_str(), &d_ptr.data<LibObject<ip_mreq_source>>()->impl->imr_multiaddr)));
}

MINT_FUNCTION(mint_socket_ipv4_mreq_source_get_sourceaddr, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &d_ptr = helper.pop_parameter();

	char buffer[INET_ADDRSTRLEN];
	if (const char *address = inet_ntop(AF_INET, &d_ptr.data<LibObject<ip_mreq_source>>()->impl->imr_sourceaddr, buffer, sizeof(buffer))) {
		helper.return_value(create_string(address));
	}
}

MINT_FUNCTION(mint_socket_ipv4_mreq_source_set_sourceaddr, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &address = helper.pop_parameter();
	Reference &d_ptr = helper.pop_parameter();
	
	helper.return_value(create_boolean(inet_pton(AF_INET, to_string(address).c_str(), &d_ptr.data<LibObject<ip_mreq_source>>()->impl->imr_sourceaddr)));
}

MINT_FUNCTION(mint_socket_ipv4_mreq_source_get_interface, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &d_ptr = helper.pop_parameter();

	char buffer[INET_ADDRSTRLEN];
	if (const char *address = inet_ntop(AF_INET, &d_ptr.data<LibObject<ip_mreq_source>>()->impl->imr_interface, buffer, sizeof(buffer))) {
		helper.return_value(create_string(address));
	}
}

MINT_FUNCTION(mint_socket_ipv4_mreq_source_set_interface, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &address = helper.pop_parameter();
	Reference &d_ptr = helper.pop_parameter();
	
	helper.return_value(create_boolean(inet_pton(AF_INET, to_string(address).c_str(), &d_ptr.data<LibObject<ip_mreq_source>>()->impl->imr_interface)));
}

MINT_FUNCTION(mint_socket_setup_ipv6_options, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &IpV6SocketOption = helper.pop_parameter();

#define BIND_IPV6_VALUE(_enum, _option) \
	_enum.data<Object>()->metadata->globals()[#_option]->value.data<Number>()->value = IPV6_##_option
#define BIND_IPV6_DISABLE(_enum, _option) \
	_enum.data<Object>()->metadata->globals()[#_option]->value.move_data(WeakReference::create<None>())

#ifdef IPV6_CHECKSUM
	BIND_IPV6_VALUE(IpV6SocketOption, CHECKSUM);
#else
	BIND_IPV6_DISABLE(IpV6SocketOption, CHECKSUM);
#endif
#ifdef IPV6_DONTFRAG
	BIND_IPV6_VALUE(IpV6SocketOption, DONTFRAG);
#else
	BIND_IPV6_DISABLE(IpV6SocketOption, DONTFRAG);
#endif
#ifdef IPV6_NEXTHOP
	BIND_IPV6_VALUE(IpV6SocketOption, NEXTHOP);
#else
	BIND_IPV6_DISABLE(IpV6SocketOption, NEXTHOP);
#endif
#ifdef IPV6_PATHMTU
	BIND_IPV6_VALUE(IpV6SocketOption, PATHMTU);
#else
	BIND_IPV6_DISABLE(IpV6SocketOption, PATHMTU);
#endif
#ifdef IPV6_RECVDSTOPTS
	BIND_IPV6_VALUE(IpV6SocketOption, RECVDSTOPTS);
#else
	BIND_IPV6_DISABLE(IpV6SocketOption, RECVDSTOPTS);
#endif
#ifdef IPV6_RECVHOPLIMIT
	BIND_IPV6_VALUE(IpV6SocketOption, RECVHOPLIMIT);
#else
	BIND_IPV6_DISABLE(IpV6SocketOption, RECVHOPLIMIT);
#endif
#ifdef IPV6_RECVHOPOPTS
	BIND_IPV6_VALUE(IpV6SocketOption, RECVHOPOPTS);
#else
	BIND_IPV6_DISABLE(IpV6SocketOption, RECVHOPOPTS);
#endif
#ifdef IPV6_RECVPATHMTU
	BIND_IPV6_VALUE(IpV6SocketOption, RECVPATHMTU);
#else
	BIND_IPV6_DISABLE(IpV6SocketOption, RECVPATHMTU);
#endif
#ifdef IPV6_RECVPKTINFO
	BIND_IPV6_VALUE(IpV6SocketOption, RECVPKTINFO);
#else
	BIND_IPV6_DISABLE(IpV6SocketOption, RECVPKTINFO);
#endif
#ifdef IPV6_RECVRTHDR
	BIND_IPV6_VALUE(IpV6SocketOption, RECVRTHDR);
#else
	BIND_IPV6_DISABLE(IpV6SocketOption, RECVRTHDR);
#endif
#ifdef IPV6_RECVTCLASS
	BIND_IPV6_VALUE(IpV6SocketOption, RECVTCLASS);
#else
	BIND_IPV6_DISABLE(IpV6SocketOption, RECVTCLASS);
#endif
#ifdef IPV6_UNICAT_HOPS
	BIND_IPV6_VALUE(IpV6SocketOption, UNICAT_HOPS);
#else
	BIND_IPV6_DISABLE(IpV6SocketOption, UNICAT_HOPS);
#endif
#ifdef IPV6_USE_MIN_MTU
	BIND_IPV6_VALUE(IpV6SocketOption, USE_MIN_MTU);
#else
	BIND_IPV6_DISABLE(IpV6SocketOption, USE_MIN_MTU);
#endif
#ifdef IPV6_V6ONLY
	BIND_IPV6_VALUE(IpV6SocketOption, V6ONLY);
#else
	BIND_IPV6_DISABLE(IpV6SocketOption, V6ONLY);
#endif
#ifdef IPV6_XXX
	BIND_IPV6_VALUE(IpV6SocketOption, XXX);
#else
	BIND_IPV6_DISABLE(IpV6SocketOption, XXX);
#endif
#ifdef IPV6_MULTICAST_IF
	BIND_IPV6_VALUE(IpV6SocketOption, MULTICAST_IF);
#else
	BIND_IPV6_DISABLE(IpV6SocketOption, MULTICAST_IF);
#endif
#ifdef IPV6_MULTICAST_HOPS
	BIND_IPV6_VALUE(IpV6SocketOption, MULTICAST_HOPS);
#else
	BIND_IPV6_DISABLE(IpV6SocketOption, MULTICAST_HOPS);
#endif
#ifdef IPV6_MULTICAST_LOOP
	BIND_IPV6_VALUE(IpV6SocketOption, MULTICAST_LOOP);
#else
	BIND_IPV6_DISABLE(IpV6SocketOption, MULTICAST_LOOP);
#endif
#ifdef IPV6_JOIN_GROUP
	BIND_IPV6_VALUE(IpV6SocketOption, JOIN_GROUP);
#else
	BIND_IPV6_DISABLE(IpV6SocketOption, JOIN_GROUP);
#endif
#ifdef IPV6_LEAVE_GROUP
	BIND_IPV6_VALUE(IpV6SocketOption, LEAVE_GROUP);
#else
	BIND_IPV6_DISABLE(IpV6SocketOption, LEAVE_GROUP);
#endif
}

MINT_FUNCTION(mint_socket_get_ipv6_option_number, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();
	WeakReference result = create_iterator();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	int option_value = 0;

	if (get_socket_option(socket_fd, IPPROTO_IPV6, option_id, &option_value)) {
		iterator_insert(result.data<Iterator>(), create_number(option_value));
	}
	else {
		iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
		iterator_insert(result.data<Iterator>(), create_number(errno_from_io_last_error()));
	}
	
	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_socket_set_ipv6_option_number, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	Reference &value = helper.pop_parameter();
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	const int option_value = to_integer(cursor, value);

	if (!set_socket_option(socket_fd, IPPROTO_IPV6, option_id, option_value)) {
		helper.return_value(create_number(errno_from_io_last_error()));
	}
}

MINT_FUNCTION(mint_socket_get_ipv6_option_boolean, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();
	WeakReference result = create_iterator();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	sockopt_bool option_value = sockopt_false;

	if (get_socket_option(socket_fd, IPPROTO_IPV6, option_id, &option_value)) {
		iterator_insert(result.data<Iterator>(), create_boolean(option_value != sockopt_false));
	}
	else {
		iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
		iterator_insert(result.data<Iterator>(), create_number(errno_from_io_last_error()));
	}
	
	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_socket_set_ipv6_option_boolean, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	Reference &value = helper.pop_parameter();
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	const sockopt_bool option_value = to_boolean(cursor, value) ? sockopt_true : sockopt_false;

	if (!set_socket_option(socket_fd, IPPROTO_IPV6, option_id, option_value)) {
		helper.return_value(create_number(errno_from_io_last_error()));
	}
}

MINT_FUNCTION(mint_socket_get_ipv6_option_addr, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();
	WeakReference result = create_iterator();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	std::unique_ptr<sockaddr_in6> option_value(new sockaddr_in6);

	if (get_socket_option(socket_fd, IPPROTO_IPV6, option_id, option_value.get())) {
		iterator_insert(result.data<Iterator>(), create_object(option_value.release()));
	}
	else {
		iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
		iterator_insert(result.data<Iterator>(), create_number(errno_from_io_last_error()));
	}
	
	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_socket_set_ipv6_option_addr, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	Reference &value = helper.pop_parameter();
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	const sockaddr_in6 *option_value = value.data<LibObject<sockaddr_in6>>()->impl;

	if (!set_socket_option(socket_fd, IPPROTO_IPV6, option_id, option_value)) {
		helper.return_value(create_number(errno_from_io_last_error()));
	}
}

MINT_FUNCTION(mint_socket_get_ipv6_option_mtuinfo, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();
	WeakReference result = create_iterator();

#if defined(OS_LINUX) && defined(__UAPI_DEF_IP6_MTUINFO)
	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	std::unique_ptr<ip6_mtuinfo> option_value(new ip6_mtuinfo);

	if (get_socket_option(socket_fd, IPPROTO_IPV6, option_id, option_value.get())) {
		iterator_insert(result.data<Iterator>(), create_object(option_value.release()));
	}
	else {
		iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
		iterator_insert(result.data<Iterator>(), create_number(errno_from_io_last_error()));
	}
#else
	((void)option);
	((void)socket);
	iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
	iterator_insert(result.data<Iterator>(), create_number(ENOTSUP));
#endif
	
	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_socket_set_ipv6_option_mtuinfo, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	Reference &value = helper.pop_parameter();
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();

#if defined(OS_LINUX) && defined(__UAPI_DEF_IP6_MTUINFO)
	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	const ip6_mtuinfo *option_value = value.data<LibObject<ip6_mtuinfo>>()->impl;

	if (!set_socket_option(socket_fd, IPPROTO_IPV6, option_id, option_value)) {
		helper.return_value(create_number(errno_from_io_last_error()));
	}
#else
	((void)value);
	((void)option);
	((void)socket);
	helper.return_value(create_number(ENOTSUP));
#endif
}

MINT_FUNCTION(mint_socket_get_ipv6_option_mreq, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();
	WeakReference result = create_iterator();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	std::unique_ptr<ipv6_mreq> option_value(new ipv6_mreq);

	if (get_socket_option(socket_fd, IPPROTO_IPV6, option_id, option_value.get())) {
		iterator_insert(result.data<Iterator>(), create_object(option_value.release()));
	}
	else {
		iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
		iterator_insert(result.data<Iterator>(), create_number(errno_from_io_last_error()));
	}
	
	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_socket_set_ipv6_option_mreq, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	Reference &value = helper.pop_parameter();
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	const ipv6_mreq *option_value = value.data<LibObject<ipv6_mreq>>()->impl;

	if (!set_socket_option(socket_fd, IPPROTO_IPV6, option_id, option_value)) {
		helper.return_value(create_number(errno_from_io_last_error()));
	}
}

MINT_FUNCTION(mint_socket_ipv6_mreq_create, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &ipv6mr_interface = helper.pop_parameter();
	Reference &ipv6mr_multiaddr = helper.pop_parameter();

	std::unique_ptr<ipv6_mreq> group(new ipv6_mreq);
	if (!inet_pton(AF_INET6, to_string(ipv6mr_multiaddr).c_str(), &group->ipv6mr_multiaddr)) {
		return;
	}
#ifdef OS_WINDOWS
	group->ipv6mr_interface = to_integer(cursor, ipv6mr_interface);
#else
	group->ipv6mr_ifindex = to_integer(cursor, ipv6mr_interface);
#endif
	helper.return_value(create_object(group.release()));
}

MINT_FUNCTION(mint_socket_ipv6_mreq_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &d_ptr = helper.pop_parameter();

	delete d_ptr.data<LibObject<ipv6_mreq>>()->impl;
}

MINT_FUNCTION(mint_socket_ipv6_req_get_multiaddr, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &d_ptr = helper.pop_parameter();

	char buffer[INET_ADDRSTRLEN];
	if (const char *address = inet_ntop(AF_INET6, &d_ptr.data<LibObject<ipv6_mreq>>()->impl->ipv6mr_multiaddr, buffer, sizeof(buffer))) {
		helper.return_value(create_string(address));
	}
}

MINT_FUNCTION(mint_socket_ipv6_mreq_set_multiaddr, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &address = helper.pop_parameter();
	Reference &d_ptr = helper.pop_parameter();
	
	helper.return_value(create_boolean(inet_pton(AF_INET6, to_string(address).c_str(), &d_ptr.data<LibObject<ipv6_mreq>>()->impl->ipv6mr_multiaddr)));
}

MINT_FUNCTION(mint_socket_ipv6_mreq_get_interface, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &d_ptr = helper.pop_parameter();

#ifdef OS_WINDOWS
	helper.return_value(create_number(d_ptr.data<LibObject<ipv6_mreq>>()->impl->ipv6mr_interface));
#else
	helper.return_value(create_number(d_ptr.data<LibObject<ipv6_mreq>>()->impl->ipv6mr_ifindex));
#endif
}

MINT_FUNCTION(mint_socket_ipv6_mreq_set_interface, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &index = helper.pop_parameter();
	Reference &d_ptr = helper.pop_parameter();

#ifdef OS_WINDOWS
	d_ptr.data<LibObject<ipv6_mreq>>()->impl->ipv6mr_interface = to_integer(cursor, index);
#else
	d_ptr.data<LibObject<ipv6_mreq>>()->impl->ipv6mr_ifindex = to_integer(cursor, index);
#endif
}
