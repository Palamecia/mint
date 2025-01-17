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

#ifdef OS_UNIX
#include <fcntl.h>
#include <sys/ioctl.h>
#endif

using namespace mint;

#ifdef OS_WINDOWS
#define SOCKOPT_CAST(__value) reinterpret_cast<char *>(__value)
#else
#define SOCKOPT_CAST(__value) (__value)
#endif

bool mint::get_socket_option(SOCKET socket, int option, int *value) {
	return get_socket_option(socket, SOL_SOCKET, option, value);
}

bool mint::set_socket_option(SOCKET socket, int option, int value) {
	return set_socket_option(socket, SOL_SOCKET, option, value);
}

bool mint::get_socket_option(SOCKET socket, int option, sockopt_bool *value) {
	return get_socket_option(socket, SOL_SOCKET, option, value);
}

bool mint::set_socket_option(SOCKET socket, int option, sockopt_bool value) {
	return set_socket_option(socket, SOL_SOCKET, option, value);
}

bool mint::get_socket_option(SOCKET socket, int option, linger *value) {
	return get_socket_option(socket, SOL_SOCKET, option, value);
}

bool mint::set_socket_option(SOCKET socket, int option, const linger *value) {
	return set_socket_option(socket, SOL_SOCKET, option, value);
}

bool mint::get_socket_option(SOCKET socket, int option, timeval *value) {
	return get_socket_option(socket, SOL_SOCKET, option, value);
}

bool mint::set_socket_option(SOCKET socket, int option, const timeval *value) {
	return set_socket_option(socket, SOL_SOCKET, option, value);
}

bool mint::get_socket_option(SOCKET socket, int level, int option, int *value) {
	socklen_t len = sizeof(int);
	return getsockopt(socket, level, option, SOCKOPT_CAST(value), &len) == 0;
}

bool mint::set_socket_option(SOCKET socket, int level, int option, int value) {
	return setsockopt(socket, level, option, reinterpret_cast<const char *>(&value), sizeof(value)) == 0;
}

bool mint::get_socket_option(SOCKET socket, int level, int option, u_char *value) {
	socklen_t len = sizeof(u_char);
	return getsockopt(socket, level, option, SOCKOPT_CAST(value), &len) == 0;
}

bool mint::set_socket_option(SOCKET socket, int level, int option, u_char value) {
	return setsockopt(socket, level, option, reinterpret_cast<const char *>(&value), sizeof(value)) == 0;
}

bool mint::get_socket_option(SOCKET socket, int level, int option, sockopt_bool *value) {
	socklen_t len = sizeof(sockopt_bool);
	return getsockopt(socket, level, option, SOCKOPT_CAST(value), &len) == 0;
}

bool mint::set_socket_option(SOCKET socket, int level, int option, sockopt_bool value) {
	return setsockopt(socket, level, option, reinterpret_cast<const char *>(&value), sizeof(value)) == 0;
}

bool mint::get_socket_option(SOCKET socket, int level, int option, void *value, socklen_t len) {
	return getsockopt(socket, level, option, SOCKOPT_CAST(value), &len) == 0;
}

bool mint::set_socket_option(SOCKET socket, int level, int option, const void *value, socklen_t len) {
	return setsockopt(socket, level, option, reinterpret_cast<const char *>(value), len) == 0;
}

MINT_FUNCTION(mint_socket_is_non_blocking, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &socket = helper.pop_parameter();

	bool status = false;
	const SOCKET socket_fd = to_integer(cursor, socket);

#ifdef OS_WINDOWS
	status = Scheduler::instance().is_socket_blocking(socket_fd);
#else
	int flags = 0;

	if ((flags = fcntl(socket_fd, F_GETFL, 0)) != -1) {
		status = flags & O_NONBLOCK;
	}
#endif

	helper.return_value(create_boolean(status));
}

MINT_FUNCTION(mint_socket_set_non_blocking, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &enabled = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();

	bool success = false;
	const SOCKET socket_fd = to_integer(cursor, socket);

#ifdef OS_WINDOWS
	u_long value = static_cast<u_long>(to_boolean(enabled));

	if (ioctlsocket(socket_fd, FIONBIO, &value) != SOCKET_ERROR) {
		success = true;
	}
	else {
		helper.return_value(create_number(errno_from_io_last_error()));
	}
#else
	int value = static_cast<int>(to_boolean(enabled));

	if (ioctl(socket_fd, FIONBIO, &value) != -1) {
		success = true;
	}
	else {
		helper.return_value(create_number(errno));
	}
#endif

	if (success) {
		Scheduler::instance().set_socket_blocking(socket_fd, value != 0);
	}
}

MINT_FUNCTION(mint_socket_setup_options, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &SocketOption = helper.pop_parameter();

#define BIND_SO_VALUE(_enum, _option) \
	_enum.data<Object>()->metadata->globals()[#_option]->value.data<Number>()->value = SO_##_option
#define BIND_SO_DISABLE(_enum, _option) \
	_enum.data<Object>()->metadata->globals()[#_option]->value.move_data(WeakReference::create<None>())

#ifdef SO_BROADCAST
	BIND_SO_VALUE(SocketOption, BROADCAST);
#else
	BIND_SO_DISABLE(SocketOption, BROADCAST);
#endif
#ifdef SO_DEBUG
	BIND_SO_VALUE(SocketOption, DEBUG);
#else
	BIND_SO_DISABLE(SocketOption, DEBUG);
#endif
#ifdef SO_DONTROUTE
	BIND_SO_VALUE(SocketOption, DONTROUTE);
#else
	BIND_SO_DISABLE(SocketOption, DONTROUTE);
#endif
#ifdef SO_ERROR
	BIND_SO_VALUE(SocketOption, ERROR);
#else
	BIND_SO_DISABLE(SocketOption, ERROR);
#endif
#ifdef SO_KEEPALIVE
	BIND_SO_VALUE(SocketOption, KEEPALIVE);
#else
	BIND_SO_DISABLE(SocketOption, KEEPALIVE);
#endif
#ifdef SO_LINGER
	BIND_SO_VALUE(SocketOption, LINGER);
#else
	BIND_SO_DISABLE(SocketOption, LINGER);
#endif
#ifdef SO_OOBINLINE
	BIND_SO_VALUE(SocketOption, OOBINLINE);
#else
	BIND_SO_DISABLE(SocketOption, OOBINLINE);
#endif
#ifdef SO_RCVBUF
	BIND_SO_VALUE(SocketOption, RCVBUF);
#else
	BIND_SO_DISABLE(SocketOption, RCVBUF);
#endif
#ifdef SO_SNDBUF
	BIND_SO_VALUE(SocketOption, SNDBUF);
#else
	BIND_SO_DISABLE(SocketOption, SNDBUF);
#endif
#ifdef SO_RCVLOWAT
	BIND_SO_VALUE(SocketOption, RCVLOWAT);
#else
	BIND_SO_DISABLE(SocketOption, RCVLOWAT);
#endif
#ifdef SO_SNDLOWAT
	BIND_SO_VALUE(SocketOption, SNDLOWAT);
#else
	BIND_SO_DISABLE(SocketOption, SNDLOWAT);
#endif
#ifdef SO_RCVTIMEO
	BIND_SO_VALUE(SocketOption, RCVTIMEO);
#else
	BIND_SO_DISABLE(SocketOption, RCVTIMEO);
#endif
#ifdef SO_SNDTIMEO
	BIND_SO_VALUE(SocketOption, SNDTIMEO);
#else
	BIND_SO_DISABLE(SocketOption, SNDTIMEO);
#endif
#ifdef SO_REUSEADDR
	BIND_SO_VALUE(SocketOption, REUSEADDR);
#else
	BIND_SO_DISABLE(SocketOption, REUSEADDR);
#endif
#ifdef SO_REUSEPORT
	BIND_SO_VALUE(SocketOption, REUSEPORT);
#else
	BIND_SO_DISABLE(SocketOption, REUSEPORT);
#endif
#ifdef SO_TYPE
	BIND_SO_VALUE(SocketOption, TYPE);
#else
	BIND_SO_DISABLE(SocketOption, TYPE);
#endif
#ifdef SO_USELOOPBACK
	BIND_SO_VALUE(SocketOption, USELOOPBACK);
#else
	BIND_SO_DISABLE(SocketOption, USELOOPBACK);
#endif
}

MINT_FUNCTION(mint_socket_get_option_number, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();
	WeakReference result = create_iterator();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	int option_value = 0;

	if (get_socket_option(socket_fd, option_id, &option_value)) {
		iterator_yield(result.data<Iterator>(), create_number(option_value));
	}
	else {
		iterator_yield(result.data<Iterator>(), WeakReference::create<None>());
		iterator_yield(result.data<Iterator>(), create_number(errno_from_io_last_error()));
	}

	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_socket_set_option_number, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	Reference &value = helper.pop_parameter();
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	const int option_value = to_integer(cursor, value);

	if (!set_socket_option(socket_fd, option_id, option_value)) {
		helper.return_value(create_number(errno_from_io_last_error()));
	}
}

MINT_FUNCTION(mint_socket_get_option_boolean, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();
	WeakReference result = create_iterator();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	sockopt_bool option_value = SOCKOPT_FALSE;

	if (get_socket_option(socket_fd, option_id, &option_value)) {
		iterator_yield(result.data<Iterator>(), create_boolean(option_value != SOCKOPT_FALSE));
	}
	else {
		iterator_yield(result.data<Iterator>(), WeakReference::create<None>());
		iterator_yield(result.data<Iterator>(), create_number(errno_from_io_last_error()));
	}

	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_socket_set_option_boolean, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	Reference &value = helper.pop_parameter();
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	const sockopt_bool option_value = to_boolean(value) ? SOCKOPT_TRUE : SOCKOPT_FALSE;

	if (!set_socket_option(socket_fd, option_id, option_value)) {
		helper.return_value(create_number(errno_from_io_last_error()));
	}
}

MINT_FUNCTION(mint_socket_get_option_linger, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();
	WeakReference result = create_iterator();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	std::unique_ptr<linger> option_value(new linger);

	if (get_socket_option(socket_fd, option_id, option_value.get())) {
		iterator_yield(result.data<Iterator>(), create_object(option_value.release()));
	}
	else {
		iterator_yield(result.data<Iterator>(), WeakReference::create<None>());
		iterator_yield(result.data<Iterator>(), create_number(errno_from_io_last_error()));
	}

	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_socket_set_option_linger, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	Reference &value = helper.pop_parameter();
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	const linger *option_value = value.data<LibObject<linger>>()->impl;

	if (!set_socket_option(socket_fd, option_id, option_value)) {
		helper.return_value(create_number(errno_from_io_last_error()));
	}
}

MINT_FUNCTION(mint_socket_get_option_timeval, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();
	WeakReference result = create_iterator();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	std::unique_ptr<timeval> option_value(new timeval);

	if (get_socket_option(socket_fd, option_id, option_value.get())) {
		iterator_yield(result.data<Iterator>(), create_object(option_value.release()));
	}
	else {
		iterator_yield(result.data<Iterator>(), WeakReference::create<None>());
		iterator_yield(result.data<Iterator>(), create_number(errno_from_io_last_error()));
	}

	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_socket_set_option_timeval, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	Reference &value = helper.pop_parameter();
	Reference &option = helper.pop_parameter();
	Reference &socket = helper.pop_parameter();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const int option_id = to_integer(cursor, option);
	const timeval *option_value = value.data<LibObject<timeval>>()->impl;

	if (!set_socket_option(socket_fd, option_id, option_value)) {
		helper.return_value(create_number(errno_from_io_last_error()));
	}
}

MINT_FUNCTION(mint_socket_finalize_connection, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &socket = helper.pop_parameter();
	WeakReference result = create_iterator();

	int error = EINVAL;
	const SOCKET socket_fd = to_integer(cursor, socket);
	auto IOStatus = helper.reference(symbols::Network).member(symbols::EndPoint).member(symbols::IOStatus);

	if (!get_socket_option(socket_fd, SO_ERROR, &error)) {
		error = errno_from_io_last_error();
	}

	switch (error) {
	case 0:
		iterator_yield(result.data<Iterator>(), IOStatus.member(symbols::IOSuccess));
		break;
	case EALREADY:
	case EINPROGRESS:
	case EWOULDBLOCK:
		iterator_yield(result.data<Iterator>(), IOStatus.member(symbols::IOWouldBlock));
		Scheduler::instance().set_socket_blocked(socket_fd, true);
		break;
	default:
		iterator_yield(result.data<Iterator>(), IOStatus.member(symbols::IOError));
		iterator_yield(result.data<Iterator>(), create_number(error));
		break;
	}

	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_socket_shutdown, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &socket = helper.pop_parameter();
	WeakReference result = create_iterator();

#ifdef OS_WINDOWS
	const int how = SD_BOTH;
#else
	const int how = SHUT_RDWR;
#endif
	const SOCKET socket_fd = to_integer(cursor, socket);
	auto IOStatus = helper.reference(symbols::Network).member(symbols::EndPoint).member(symbols::IOStatus);

	if (::shutdown(socket_fd, how) == 0) {
		iterator_yield(result.data<Iterator>(), IOStatus.member(symbols::IOSuccess));
	}
	else {
		switch (int error = errno_from_io_last_error()) {
		case EINPROGRESS:
		case EWOULDBLOCK:
			iterator_yield(result.data<Iterator>(), IOStatus.member(symbols::IOWouldBlock));
			Scheduler::instance().set_socket_blocked(socket_fd, true);
			break;
		case ENOTCONN:
			iterator_yield(result.data<Iterator>(), IOStatus.member(symbols::IOClosed));
			break;
		default:
			iterator_yield(result.data<Iterator>(), IOStatus.member(symbols::IOError));
			iterator_yield(result.data<Iterator>(), create_number(error));
			break;
		}
	}

	helper.return_value(std::move(result));
}

MINT_FUNCTION(mint_socket_close, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &socket = helper.pop_parameter();

	const SOCKET socket_fd = to_integer(cursor, socket);

	if (Scheduler::Error error = Scheduler::instance().close_socket(socket_fd)) {
		helper.return_value(create_number(error.get_errno()));
	}
}

MINT_FUNCTION(mint_socket_get_error, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &socket = helper.pop_parameter();

	int error = 0;

	if (get_socket_option(to_integer(cursor, socket), SO_ERROR, &error)) {
		helper.return_value(create_number(error));
	}
	else {
		helper.return_value(create_number(errno_from_io_last_error()));
	}
}

MINT_FUNCTION(mint_socket_strerror, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &error = helper.pop_parameter();

	helper.return_value(create_string(strerror(to_integer(cursor, error))));
}

MINT_FUNCTION(mint_socket_linger_create, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &linger_time = helper.pop_parameter();
	Reference &enabled = helper.pop_parameter();

	helper.return_value(
		create_object(new linger {to_boolean(enabled), static_cast<u_short>(to_integer(cursor, linger_time))}));
}

MINT_FUNCTION(mint_socket_linger_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &d_ptr = helper.pop_parameter();

	delete d_ptr.data<LibObject<linger>>()->impl;
}

MINT_FUNCTION(mint_socket_linger_get_onoff, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &d_ptr = helper.pop_parameter();

	helper.return_value(create_boolean(d_ptr.data<LibObject<linger>>()->impl->l_onoff));
}

MINT_FUNCTION(mint_socket_linger_set_onoff, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &enabled = helper.pop_parameter();
	Reference &d_ptr = helper.pop_parameter();

	d_ptr.data<LibObject<linger>>()->impl->l_onoff = to_boolean(enabled);
}

MINT_FUNCTION(mint_socket_linger_get_linger, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &d_ptr = helper.pop_parameter();

	helper.return_value(create_boolean(d_ptr.data<LibObject<linger>>()->impl->l_linger));
}

MINT_FUNCTION(mint_socket_linger_set_linger, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &linger_time = helper.pop_parameter();
	Reference &d_ptr = helper.pop_parameter();

	d_ptr.data<LibObject<linger>>()->impl->l_linger = static_cast<u_short>(to_integer(cursor, linger_time));
}

MINT_FUNCTION(mint_socket_timeval_create, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &usec = helper.pop_parameter();
	Reference &sec = helper.pop_parameter();

	helper.return_value(create_object(
		new timeval {static_cast<long>(to_integer(cursor, sec)), static_cast<long>(to_integer(cursor, usec))}));
}

MINT_FUNCTION(mint_socket_timeval_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &d_ptr = helper.pop_parameter();

	delete d_ptr.data<LibObject<timeval>>()->impl;
}

MINT_FUNCTION(mint_socket_timeval_get_sec, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &d_ptr = helper.pop_parameter();

	helper.return_value(create_number(d_ptr.data<LibObject<timeval>>()->impl->tv_sec));
}

MINT_FUNCTION(mint_socket_timeval_set_sec, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &sec = helper.pop_parameter();
	Reference &d_ptr = helper.pop_parameter();

	d_ptr.data<LibObject<timeval>>()->impl->tv_sec = static_cast<long>(to_integer(cursor, sec));
}

MINT_FUNCTION(mint_socket_timeval_get_usec, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	const Reference &d_ptr = helper.pop_parameter();

	helper.return_value(create_boolean(d_ptr.data<LibObject<timeval>>()->impl->tv_usec));
}

MINT_FUNCTION(mint_socket_timeval_set_usec, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &usec = helper.pop_parameter();
	Reference &d_ptr = helper.pop_parameter();

	d_ptr.data<LibObject<timeval>>()->impl->tv_usec = static_cast<long>(to_integer(cursor, usec));
}
