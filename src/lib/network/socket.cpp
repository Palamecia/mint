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

#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/casttool.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/processor.h"
#include "scheduler.h"
#include <cerrno>
#include <cstring>
#include <memory>
#include "socket.h"

#ifdef MINT_OS_WINDOWS
#include <bit>
#include <Windows.h>
#include <WinSock2.h>
#else
#include <asm-generic/ioctls.h>
#include <asm-generic/socket.h>
#include <bits/types/struct_timeval.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#ifdef MINT_OS_WINDOWS
#define SOCKOPT_CAST(__value) std::bit_cast<char*>(__value)
#else
#define SOCKOPT_CAST(__value) (__value)
#endif

bool mint::get_socket_option(SOCKET socket, int option, int* value) {
	return get_socket_option(socket, SOL_SOCKET, option, value);
}

bool mint::set_socket_option(SOCKET socket, int option, int value) {
	return set_socket_option(socket, SOL_SOCKET, option, value);
}

bool mint::get_socket_option(SOCKET socket, int option, sockopt_bool* value) {
	return get_socket_option(socket, SOL_SOCKET, option, value);
}

bool mint::set_socket_option(SOCKET socket, int option, sockopt_bool value) {
	return set_socket_option(socket, SOL_SOCKET, option, value);
}

bool mint::get_socket_option(SOCKET socket, int option, linger* value) {
	return get_socket_option(socket, SOL_SOCKET, option, value);
}

bool mint::set_socket_option(SOCKET socket, int option, const linger* value) {
	return set_socket_option(socket, SOL_SOCKET, option, value);
}

bool mint::get_socket_option(SOCKET socket, int option, timeval* value) {
	return get_socket_option(socket, SOL_SOCKET, option, value);
}

bool mint::set_socket_option(SOCKET socket, int option, const timeval* value) {
	return set_socket_option(socket, SOL_SOCKET, option, value);
}

bool mint::get_socket_option(SOCKET socket, int level, int option, int* value) {
	socklen_t len = sizeof(int);
	return getsockopt(socket, level, option, SOCKOPT_CAST(value), &len) == 0;
}

bool mint::set_socket_option(SOCKET socket, int level, int option, int value) {
	return setsockopt(socket, level, option, reinterpret_cast<const char*>(&value), sizeof(value)) == 0;
}

bool mint::get_socket_option(SOCKET socket, int level, int option, u_char* value) {
	socklen_t len = sizeof(u_char);
	return getsockopt(socket, level, option, SOCKOPT_CAST(value), &len) == 0;
}

bool mint::set_socket_option(SOCKET socket, int level, int option, u_char value) {
	return setsockopt(socket, level, option, reinterpret_cast<const char*>(&value), sizeof(value)) == 0;
}

bool mint::get_socket_option(SOCKET socket, int level, int option, sockopt_bool* value) {
	socklen_t len = sizeof(sockopt_bool);
	return getsockopt(socket, level, option, SOCKOPT_CAST(value), &len) == 0;
}

bool mint::set_socket_option(SOCKET socket, int level, int option, sockopt_bool value) {
	return setsockopt(socket, level, option, reinterpret_cast<const char*>(&value), sizeof(value)) == 0;
}

bool mint::get_socket_option(SOCKET socket, int level, int option, void* value, socklen_t len) {
	return getsockopt(socket, level, option, SOCKOPT_CAST(value), &len) == 0;
}

bool mint::set_socket_option(SOCKET socket, int level, int option, const void* value, socklen_t len) {
	return setsockopt(socket, level, option, reinterpret_cast<const char*>(value), len) == 0;
}

namespace {

mint::WeakReference mint_socket_is_non_blocking(mint::Cursor& cursor, const mint::Reference& socket) {

	bool status = false;
	const auto socket_fd = mint::to_integer<SOCKET>(cursor, socket);

#ifdef MINT_OS_WINDOWS
	status = Scheduler::instance().is_socket_blocking(socket_fd);
#else
	int flags = 0;

	if ((flags = fcntl(socket_fd, F_GETFL, 0)) != -1) {
		status = flags & O_NONBLOCK;
	}
#endif

	return mint::create_boolean(status);
}

mint::WeakReference mint_socket_set_non_blocking(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& enabled) {

	bool success = false;
	const auto socket_fd = mint::to_integer<SOCKET>(cursor, socket);

#ifdef MINT_OS_WINDOWS
	auto value = static_cast<u_long>(to_boolean(enabled));

	if (ioctlsocket(socket_fd, FIONBIO, &value) != SOCKET_ERROR) {
		success = true;
	}
	else {
		return mint::create_number(errno_from_io_last_error());
	}
#else
	int value = static_cast<int>(mint::to_boolean(enabled));

	if (ioctl(socket_fd, FIONBIO, &value) != -1) {
		success = true;
	}
	else {
		return mint::create_number(errno);
	}
#endif

	if (success) {
		Scheduler::instance().set_socket_blocking(socket_fd, value != 0);
	}

	return {};
}

mint::WeakReference mint_socket_setup_options(mint::Cursor& /*cursor*/, const mint::Reference& socket_option) {

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

mint::WeakReference mint_socket_get_option_number(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::WeakReference result = mint::create_iterator(cursor.ast());

	const auto socket_fd = mint::to_integer<SOCKET>(cursor, socket);
	const auto option_id = mint::to_integer<int>(cursor, option);
	int option_value = 0;

	if (mint::get_socket_option(socket_fd, option_id, &option_value)) {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(option_value));
	}
	else {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(errno_from_io_last_error()));
	}

	return result;
}

mint::WeakReference mint_socket_set_option_number(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	const auto socket_fd = mint::to_integer<SOCKET>(cursor, socket);
	const auto option_id = mint::to_integer<int>(cursor, option);
	const auto option_value = mint::to_integer<int>(cursor, value);

	if (!mint::set_socket_option(socket_fd, option_id, option_value)) {
		return mint::create_number(errno_from_io_last_error());
	}

	return {};
}

mint::WeakReference mint_socket_get_option_boolean(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::WeakReference result = mint::create_iterator(cursor.ast());

	const auto socket_fd = mint::to_integer<SOCKET>(cursor, socket);
	const auto option_id = mint::to_integer<int>(cursor, option);
	mint::sockopt_bool option_value = mint::sockopt_false;

	if (mint::get_socket_option(socket_fd, option_id, &option_value)) {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_boolean(option_value != mint::sockopt_false));
	}
	else {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(errno_from_io_last_error()));
	}

	return result;
}

mint::WeakReference mint_socket_set_option_boolean(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const auto option_id = to_integer<int>(cursor, option);
	const mint::sockopt_bool option_value = to_boolean(value) ? mint::sockopt_true : mint::sockopt_false;

	if (!mint::set_socket_option(socket_fd, option_id, option_value)) {
		return mint::create_number(errno_from_io_last_error());
	}

	return {};
}

mint::WeakReference mint_socket_get_option_linger(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::WeakReference result = mint::create_iterator(cursor.ast());

	const auto socket_fd = mint::to_integer<SOCKET>(cursor, socket);
	const auto option_id = mint::to_integer<int>(cursor, option);
	auto option_value = std::make_unique<linger>();

	if (mint::get_socket_option(socket_fd, option_id, option_value.get())) {
		iterator_yield(cursor, result.data<mint::Iterator>(),
		    mint::create_c_object(cursor.ast(), option_value.release()));
	}
	else {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(errno_from_io_last_error()));
	}

	return result;
}

mint::WeakReference mint_socket_set_option_linger(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const auto option_id = to_integer<int>(cursor, option);
	const linger* option_value = value.data<mint::LibObject<linger>>().ptr;

	if (!mint::set_socket_option(socket_fd, option_id, option_value)) {
		return mint::create_number(errno_from_io_last_error());
	}

	return {};
}

mint::WeakReference mint_socket_get_option_timeval(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::WeakReference result = mint::create_iterator(cursor.ast());

	const auto socket_fd = mint::to_integer<SOCKET>(cursor, socket);
	const auto option_id = mint::to_integer<int>(cursor, option);
	auto option_value = std::make_unique<timeval>();

	if (mint::get_socket_option(socket_fd, option_id, option_value.get())) {
		iterator_yield(cursor, result.data<mint::Iterator>(),
		    mint::create_c_object(cursor.ast(), option_value.release()));
	}
	else {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(errno_from_io_last_error()));
	}

	return result;
}

mint::WeakReference mint_socket_set_option_timeval(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	const auto socket_fd = mint::to_integer<SOCKET>(cursor, socket);
	const auto option_id = mint::to_integer<int>(cursor, option);
	const timeval* option_value = value.data<mint::LibObject<timeval>>().ptr;

	if (!mint::set_socket_option(socket_fd, option_id, option_value)) {
		return mint::create_number(errno_from_io_last_error());
	}

	return {};
}

mint::WeakReference mint_socket_finalize_connection(mint::FunctionHelper& helper, const mint::Reference& socket) {

	mint::WeakReference result = mint::create_iterator(helper.cursor().ast());

	int error = EINVAL;
	const auto socket_fd = mint::to_integer<SOCKET>(helper.cursor(), socket);
	auto io_status =
	    helper.reference(mint::symbols::network).member(mint::symbols::end_point).member(mint::symbols::io_status);

	if (!mint::get_socket_option(socket_fd, SO_ERROR, &error)) {
		error = errno_from_io_last_error();
	}

	switch (error) {
	case 0:
		iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
		    io_status.member(mint::symbols::io_success).share());
		break;
	case EALREADY:
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

	return result;
}

mint::WeakReference mint_socket_shutdown(mint::FunctionHelper& helper, const mint::Reference& socket) {

	mint::WeakReference result = mint::create_iterator(helper.cursor().ast());

#ifdef MINT_OS_WINDOWS
	const int how = SD_BOTH;
#else
	const int how = SHUT_RDWR;
#endif
	const auto socket_fd = mint::to_integer<SOCKET>(helper.cursor(), socket);
	auto io_status =
	    helper.reference(mint::symbols::network).member(mint::symbols::end_point).member(mint::symbols::io_status);

	mint::unlock_processor();
	const auto shutdown_result = ::shutdown(socket_fd, how);
	mint::lock_processor();

	if (shutdown_result == 0) {
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
		case ENOTCONN:
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
			    io_status.member(mint::symbols::io_closed).share());
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

mint::WeakReference mint_socket_close(mint::Cursor& cursor, const mint::Reference& socket) {

	const auto socket_fd = mint::to_integer<SOCKET>(cursor, socket);

	if (const auto error = Scheduler::instance().close_socket(socket_fd)) {
		return mint::create_number(error.get_errno());
	}

	return {};
}

mint::WeakReference mint_socket_get_error(mint::Cursor& cursor, const mint::Reference& socket) {

	if (int error = 0; mint::get_socket_option(mint::to_integer<SOCKET>(cursor, socket), SO_ERROR, &error)) {
		return mint::create_number(error);
	}

	return mint::create_number(errno_from_io_last_error());
}

mint::WeakReference mint_socket_strerror(mint::Cursor& cursor, const mint::Reference& error) {
	return mint::create_string(cursor.ast(), strerror(to_integer<int>(cursor, error)));
}

mint::WeakReference mint_socket_linger_create(mint::Cursor& cursor, const mint::Reference& enabled,
    mint::Reference& linger_time) {
	return mint::create_c_object(cursor.ast(), new linger {
	                                               .l_onoff = to_boolean(enabled),
	                                               .l_linger = to_integer<u_short>(cursor, linger_time),
	                                           });
}

mint::WeakReference mint_socket_linger_delete(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	delete d_ptr.data<mint::LibObject<linger>>().ptr;
	return {};
}

mint::WeakReference mint_socket_linger_get_onoff(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	return mint::create_boolean(d_ptr.data<mint::LibObject<linger>>().ptr->l_onoff);
}

mint::WeakReference mint_socket_linger_set_onoff(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr,
    mint::Reference& enabled) {
	d_ptr.data<mint::LibObject<linger>>().ptr->l_onoff = to_boolean(enabled);
	return {};
}

mint::WeakReference mint_socket_linger_get_linger(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	return mint::create_signed_number(d_ptr.data<mint::LibObject<linger>>().ptr->l_linger);
}

mint::WeakReference mint_socket_linger_set_linger(mint::Cursor& cursor, const mint::Reference& d_ptr,
    mint::Reference& linger_time) {
	d_ptr.data<mint::LibObject<linger>>().ptr->l_linger = to_integer<u_short>(cursor, linger_time);
	return {};
}

mint::WeakReference mint_socket_timeval_create(mint::Cursor& cursor, const mint::Reference& sec,
    const mint::Reference& usec) {
	return mint::create_c_object(cursor.ast(), new timeval {
	                                               .tv_sec = to_integer<long>(cursor, sec),
	                                               .tv_usec = to_integer<long>(cursor, usec),
	                                           });
}

mint::WeakReference mint_socket_timeval_delete(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	delete d_ptr.data<mint::LibObject<timeval>>().ptr;
	return {};
}

mint::WeakReference mint_socket_timeval_get_sec(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	return mint::create_signed_number(d_ptr.data<mint::LibObject<timeval>>().ptr->tv_sec);
}

mint::WeakReference mint_socket_timeval_set_sec(mint::Cursor& cursor, const mint::Reference& d_ptr,
    const mint::Reference& sec) {
	d_ptr.data<mint::LibObject<timeval>>().ptr->tv_sec = to_integer<long>(cursor, sec);
	return {};
}

mint::WeakReference mint_socket_timeval_get_usec(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	return mint::create_signed_number(d_ptr.data<mint::LibObject<timeval>>().ptr->tv_usec);
}

mint::WeakReference mint_socket_timeval_set_usec(mint::Cursor& cursor, const mint::Reference& d_ptr,
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
MINT_EXPORT_FUNCTION(mint_socket_finalize_connection, 1);
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
