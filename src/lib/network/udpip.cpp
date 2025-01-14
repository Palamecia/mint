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
#include <mint/memory/builtin/string.h>
#include "scheduler.h"
#include "socket.h"
#include "ip.h"

#ifdef OS_UNIX
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <linux/sockios.h>
#endif

using namespace mint;

MINT_FUNCTION(mint_udp_ip_socket_open, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &ip_version = helper.pop_parameter();
	WeakReference result = create_iterator();

	SOCKET socket_fd = INVALID_SOCKET;

	switch (to_integer(cursor, ip_version)) {
	case 4:
		socket_fd = Scheduler::instance().open_socket(AF_INET, SOCK_DGRAM, 0);
		break;
	case 6:
		socket_fd = Scheduler::instance().open_socket(AF_INET6, SOCK_DGRAM, 0);
		break;
	default:
		iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
		iterator_insert(result.data<Iterator>(), create_number(EOPNOTSUPP));
		helper.return_value(std::move(result));
		return;
	}

	if (socket_fd != INVALID_SOCKET) {
		iterator_insert(result.data<Iterator>(), create_number(socket_fd));
		if (set_socket_option(socket_fd, SO_REUSEADDR, SOCKOPT_TRUE)) {
			iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
		}
		else {
			iterator_insert(result.data<Iterator>(), create_number(errno));
		}
	}
	else {
		iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
		iterator_insert(result.data<Iterator>(), create_number(errno_from_io_last_error()));
	}

	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_udp_ip_socket_sendto, 5, cursor) {

	FunctionHelper helper(cursor, 5);
	Reference &buffer = helper.pop_parameter();
	Reference &ip_version = helper.pop_parameter();
	Reference &port = helper.pop_parameter();
	Reference &address = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();
	WeakReference result = create_iterator();

	SOCKET socket_fd = to_integer(cursor, socket);
	const std::string address_str = to_string(address);
	std::vector<uint8_t> *buf = buffer.data<LibObject<std::vector<uint8_t>>>()->impl;
	auto IOStatus = helper.reference(symbols::Network).member(symbols::EndPoint).member(symbols::IOStatus);

	std::unique_ptr<sockaddr> target;
	socklen_t targetlen = sizeof(sockaddr);

	switch (to_integer(cursor, ip_version)) {
	case 4:
		targetlen = sizeof(sockaddr_in);
		target.reset(reinterpret_cast<sockaddr *>(new sockaddr_in));
		memset(target.get(), 0, targetlen);
		reinterpret_cast<sockaddr_in *>(target.get())->sin_family = AF_INET;
		reinterpret_cast<sockaddr_in *>(target.get())->sin_port = htons(static_cast<uint16_t>(to_integer(cursor, port)));
		switch (::inet_pton(AF_INET, address_str.c_str(),
							&reinterpret_cast<sockaddr_in *>(target.get())->sin_addr.s_addr)) {
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
		targetlen = sizeof(sockaddr_in6);
		target.reset(reinterpret_cast<sockaddr *>(new sockaddr_in6));
		memset(target.get(), 0, targetlen);
		reinterpret_cast<sockaddr_in6 *>(target.get())->sin6_family = AF_INET6;
		reinterpret_cast<sockaddr_in6 *>(target.get())->sin6_port = htons(
			static_cast<uint16_t>(to_integer(cursor, port)));
		switch (::inet_pton(AF_INET6, address_str.c_str(),
							&reinterpret_cast<sockaddr_in6 *>(target.get())->sin6_addr.s6_addr)) {
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

#ifdef OS_WINDOWS
	int flags = 0;
#else
	int flags = MSG_CONFIRM;
#endif

	auto count = sendto(socket_fd, reinterpret_cast<const char *>(buf->data()), buf->size(), flags, target.get(),
						targetlen);

	switch (count) {
	case -1:
		switch (const int error = errno_from_io_last_error()) {
		case EINPROGRESS:
		case EWOULDBLOCK:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOWouldBlock));
			Scheduler::instance().set_socket_blocked(socket_fd, true);
			break;

		case EPIPE:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOClosed));
			break;

		default:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOError));
			iterator_insert(result.data<Iterator>(), create_number(error));
			break;
		}
		break;
	case 0:
		iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOClosed));
		break;
	default:
		iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOSuccess));
		iterator_insert(result.data<Iterator>(), create_number(count));
		break;
	}

	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_udp_ip_socket_recvfrom, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &buffer = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();
	WeakReference result = create_iterator();

	socklen_t length = 0;
	SOCKET socket_fd = to_integer(cursor, socket);
	std::vector<uint8_t> *buf = buffer.data<LibObject<std::vector<uint8_t>>>()->impl;
	auto IOStatus = helper.reference(symbols::Network).member(symbols::EndPoint).member(symbols::IOStatus);

	sockaddr source;
	socklen_t sourcelen = sizeof(source);
	std::string address;
	u_short port;

#ifdef OS_UNIX
	if (ioctl(socket_fd, SIOCINQ, &length) != -1) {
#else
	length = BUFSIZ; /// @todo get better value
#endif

		int flags = 0; // MSG_WAITALL;
		std::unique_ptr<uint8_t[]> local_buffer(new uint8_t[length]);
		auto count = recvfrom(socket_fd, reinterpret_cast<char *>(local_buffer.get()), static_cast<size_t>(length),
							  flags, &source, &sourcelen);

		switch (count) {
		case -1:
			switch (const int error = errno_from_io_last_error()) {
			case EINPROGRESS:
			case EWOULDBLOCK:
				iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOWouldBlock));
				Scheduler::instance().set_socket_blocked(socket_fd, true);
				break;

			case EPIPE:
				iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOClosed));
				break;

			default:
				iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOError));
				iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
				iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
				iterator_insert(result.data<Iterator>(), create_number(error));
				break;
			}
			break;
		case 0:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOClosed));
			break;
		default:
			if (const int error = get_ip_socket_info(&source, sourcelen, &address, &port)) {
				iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOError));
				iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
				iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
				iterator_insert(result.data<Iterator>(), create_number(error));
			}
			else {
				iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOSuccess));
				iterator_insert(result.data<Iterator>(), create_string(address));
				iterator_insert(result.data<Iterator>(), create_number(port));
				copy_n(local_buffer.get(), count, back_inserter(*buf));
			}
			break;
		}
#ifdef OS_UNIX
	}
	else {
		iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOError));
		iterator_insert(result.data<Iterator>(), create_number(errno));
	}
#endif

	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_udp_ip_socket_send, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &buffer = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();
	WeakReference result = create_iterator();

	SOCKET socket_fd = to_integer(cursor, socket);
	std::vector<uint8_t> *buf = buffer.data<LibObject<std::vector<uint8_t>>>()->impl;
	auto IOStatus = helper.reference(symbols::Network).member(symbols::EndPoint).member(symbols::IOStatus);

#ifdef OS_WINDOWS
	int flags = 0;
#else
	int flags = MSG_CONFIRM;
#endif

	auto count = send(socket_fd, reinterpret_cast<const char *>(buf->data()), buf->size(), flags);

	switch (count) {
	case -1:
		switch (const int error = errno_from_io_last_error()) {
		case EINPROGRESS:
		case EWOULDBLOCK:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOWouldBlock));
			Scheduler::instance().set_socket_blocked(socket_fd, true);
			break;

		case EPIPE:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOClosed));
			break;

		default:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOError));
			iterator_insert(result.data<Iterator>(), create_number(error));
			break;
		}
		break;
	case 0:
		iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOClosed));
		break;
	default:
		iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOSuccess));
		iterator_insert(result.data<Iterator>(), create_number(count));
		break;
	}

	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_udp_ip_socket_recv, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &buffer = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();
	WeakReference result = create_iterator();

	socklen_t length = 0;
	SOCKET socket_fd = to_integer(cursor, socket);
	std::vector<uint8_t> *buf = buffer.data<LibObject<std::vector<uint8_t>>>()->impl;
	auto IOStatus = helper.reference(symbols::Network).member(symbols::EndPoint).member(symbols::IOStatus);

#ifdef OS_UNIX
	if (ioctl(socket_fd, SIOCINQ, &length) != -1) {
#else
	length = BUFSIZ; /// @todo get better value
#endif

		int flags = MSG_WAITALL;
		std::unique_ptr<uint8_t[]> local_buffer(new uint8_t[length]);
		auto count = recv(socket_fd, reinterpret_cast<char *>(local_buffer.get()), static_cast<size_t>(length), flags);

		switch (count) {
		case -1:
			switch (const int error = errno_from_io_last_error()) {
			case EINPROGRESS:
			case EWOULDBLOCK:
				iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOWouldBlock));
				Scheduler::instance().set_socket_blocked(socket_fd, true);
				break;

			case EPIPE:
				iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOClosed));
				break;

			default:
				iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOError));
				iterator_insert(result.data<Iterator>(), create_number(error));
				break;
			}
			break;
		case 0:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOClosed));
			break;
		default:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOSuccess));
			copy_n(local_buffer.get(), count, back_inserter(*buf));
			break;
		}
#ifdef OS_UNIX
	}
	else {
		iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOError));
		iterator_insert(result.data<Iterator>(), create_number(errno));
	}
#endif

	helper.return_value(std::move(result));
}
