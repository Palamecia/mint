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

#ifdef OS_UNIX
#include <sys/ioctl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <linux/sockios.h>
#endif

using namespace mint;

MINT_FUNCTION(mint_tcp_ip_socket_open, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &ip_version = helper.pop_parameter();
	WeakReference result = create_iterator();

	SOCKET socket_fd = INVALID_SOCKET;

	switch (to_integer(cursor, ip_version)) {
	case 4:
		socket_fd = Scheduler::instance().open_socket(AF_INET, SOCK_STREAM, 0);
		break;
	case 6:
		socket_fd = Scheduler::instance().open_socket(AF_INET6, SOCK_STREAM, 0);
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

MINT_FUNCTION(mint_tcp_ip_socket_send, 2, cursor) {

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
	int flags = MSG_NOSIGNAL;
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

MINT_FUNCTION(mint_tcp_ip_socket_recv, 2, cursor) {

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

		std::unique_ptr<uint8_t []> local_buffer(new uint8_t [length]);
		auto count = recv(socket_fd, reinterpret_cast<char *>(local_buffer.get()), static_cast<size_t>(length), 0);

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

MINT_FUNCTION(mint_socket_setup_tcp_options, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &TcpSocketOption = helper.pop_parameter();

#define BIND_TCP_VALUE(_enum, _option) \
	_enum.data<Object>()->metadata->globals()[#_option]->value.data<Number>()->value = TCP_##_option
#define BIND_TCP_DISABLE(_enum, _option) \
	_enum.data<Object>()->metadata->globals()[#_option]->value.move_data(WeakReference::create<None>())

#ifdef TCP_MAXSEG
	BIND_TCP_VALUE(TcpSocketOption, MAXSEG);
#else
	BIND_TCP_DISABLE(TcpSocketOption, MAXSEG);
#endif
#ifdef TCP_NODELAY
	BIND_TCP_VALUE(TcpSocketOption, NODELAY);
#else
	BIND_TCP_DISABLE(TcpSocketOption, NODELAY);
#endif
}

MINT_FUNCTION(mint_socket_get_tcp_option_number, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();
	WeakReference result = create_iterator();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	int option_value = 0;

	if (get_socket_option(socket_fd, IPPROTO_TCP, option_id, &option_value)) {
		iterator_insert(result.data<Iterator>(), create_number(option_value));
	}
	else {
		iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
		iterator_insert(result.data<Iterator>(), create_number(errno_from_io_last_error()));
	}
	
	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_socket_set_tcp_option_number, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	Reference &value = helper.pop_parameter();
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	const int option_value = to_integer(cursor, value);

	if (!set_socket_option(socket_fd, IPPROTO_TCP, option_id, option_value)) {
		helper.return_value(create_number(errno_from_io_last_error()));
	}
}

MINT_FUNCTION(mint_socket_get_tcp_option_boolean, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();
	WeakReference result = create_iterator();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	sockopt_bool option_value = SOCKOPT_FALSE;

	if (get_socket_option(socket_fd, IPPROTO_TCP, option_id, &option_value)) {
		iterator_insert(result.data<Iterator>(), create_boolean(option_value != SOCKOPT_FALSE));
	}
	else {
		iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
		iterator_insert(result.data<Iterator>(), create_number(errno_from_io_last_error()));
	}
	
	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_socket_set_tcp_option_boolean, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	Reference &value = helper.pop_parameter();
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	const sockopt_bool option_value = to_boolean(cursor, value) ? SOCKOPT_TRUE : SOCKOPT_FALSE;

	if (!set_socket_option(socket_fd, IPPROTO_TCP, option_id, option_value)) {
		helper.return_value(create_number(errno_from_io_last_error()));
	}
}
