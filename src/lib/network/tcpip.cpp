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
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/casttool.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/scheduler/processor.h"
#include "scheduler.h"
#include "socket.h"
#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <iterator>
#include <vector>

#ifdef MINT_OS_WINDOWS
#include <cstdio>
#include <Windows.h>
#include <winsock2.h>
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

namespace {

mint::WeakReference mint_tcp_ip_socket_open(mint::Cursor& cursor, const mint::Reference& ip_version) {

	mint::WeakReference result = mint::create_iterator(cursor.ast());
	auto socket_fd = INVALID_SOCKET;

	switch (to_integer<int>(cursor, ip_version)) {
	case mint::ip_version_4:
		socket_fd = Scheduler::instance().open_socket(AF_INET, SOCK_STREAM, 0);
		break;
	case mint::ip_version_6:
		socket_fd = Scheduler::instance().open_socket(AF_INET6, SOCK_STREAM, 0);
		break;
	default:
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(EOPNOTSUPP));
		return result;
	}

	if (socket_fd != INVALID_SOCKET) {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_unsigned_number(socket_fd));
		if (mint::set_socket_option(socket_fd, SO_REUSEADDR, mint::sockopt_true)) {
			iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		}
		else {
			iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(errno));
		}
	}
	else {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(errno_from_io_last_error()));
	}

	return result;
}

mint::WeakReference mint_tcp_ip_socket_send(mint::FunctionHelper& helper, const mint::Reference& socket,
    mint::Reference& buffer) {

	mint::WeakReference result = mint::create_iterator(helper.cursor().ast());

	const auto socket_fd = to_integer<SOCKET>(helper.cursor(), socket);
	std::vector<std::uint8_t>* buf = buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;
	auto io_status =
	    helper.reference(mint::symbols::network).member(mint::symbols::end_point).member(mint::symbols::io_status);

#ifdef MINT_OS_WINDOWS
	const auto flags = 0;
#else
	const auto flags = MSG_NOSIGNAL;
#endif

	mint::unlock_processor();
	const auto count = send(socket_fd, reinterpret_cast<const char*>(buf->data()), static_cast<int>(buf->size()), flags);
	mint::lock_processor();

	switch (count) {
	case -1:
		switch (const int error = errno_from_io_last_error()) {
		case EINPROGRESS:
		case EWOULDBLOCK:
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
			    io_status.member(mint::symbols::io_would_block).share());
			Scheduler::instance().set_socket_blocked(socket_fd, true);
			break;

		case EPIPE:
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
			    io_status.member(mint::symbols::io_closed).share());
			break;

		default:
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
			    io_status.member(mint::symbols::io_error).share());
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(), mint::create_number(error));
			break;
		}
		break;
	case 0:
		iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
		    io_status.member(mint::symbols::io_closed).share());
		break;
	default:
		iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
		    io_status.member(mint::symbols::io_success).share());
		iterator_yield(helper.cursor(), result.data<mint::Iterator>(), mint::create_signed_number(count));
		break;
	}

	return result;
}

mint::WeakReference mint_tcp_ip_socket_recv(mint::FunctionHelper& helper, const mint::Reference& socket,
    mint::Reference& buffer) {

	mint::WeakReference result = mint::create_iterator(helper.cursor().ast());

	const auto socket_fd = to_integer<SOCKET>(helper.cursor(), socket);
	std::vector<std::uint8_t>* buf = buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;
	auto io_status =
	    helper.reference(mint::symbols::network).member(mint::symbols::end_point).member(mint::symbols::io_status);

	socklen_t length = 0;
#ifdef MINT_OS_UNIX
	if (ioctl(socket_fd, SIOCINQ, &length) != -1) {
#else
	length = BUFSIZ; /// @todo get better value
#endif

		auto local_buffer = std::make_unique<std::uint8_t[]>(length);
		mint::unlock_processor();
		auto count = recv(socket_fd, reinterpret_cast<char*>(local_buffer.get()), static_cast<int>(length), 0);
		mint::lock_processor();

		switch (count) {
		case -1:
			switch (const int error = errno_from_io_last_error()) {
			case EINPROGRESS:
			case EWOULDBLOCK:
				iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
				    io_status.member(mint::symbols::io_would_block).share());
				Scheduler::instance().set_socket_blocked(socket_fd, true);
				break;

			case EPIPE:
				iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
				    io_status.member(mint::symbols::io_closed).share());
				break;

			default:
				iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
				    io_status.member(mint::symbols::io_error).share());
				iterator_yield(helper.cursor(), result.data<mint::Iterator>(), mint::create_number(error));
				break;
			}
			break;
		case 0:
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
			    io_status.member(mint::symbols::io_closed).share());
			break;
		default:
			iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
			    io_status.member(mint::symbols::io_success).share());
			std::copy_n(local_buffer.get(), count, std::back_inserter(*buf));
			break;
		}
#ifdef MINT_OS_UNIX
	}
	else {
		iterator_yield(helper.cursor(), result.data<mint::Iterator>(),
		    io_status.member(mint::symbols::io_error).share());
		iterator_yield(helper.cursor(), result.data<mint::Iterator>(), mint::create_number(errno));
	}
#endif

	return result;
}

mint::WeakReference mint_socket_setup_tcp_options(mint::Cursor& /*cursor*/, const mint::Reference& tcp_socket_option) {

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

mint::WeakReference mint_socket_get_tcp_option_number(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::WeakReference result = mint::create_iterator(cursor.ast());

	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const auto option_id = to_integer<int>(cursor, option);
	int option_value = 0;

	if (mint::get_socket_option(socket_fd, IPPROTO_TCP, option_id, &option_value)) {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(option_value));
	}
	else {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(errno_from_io_last_error()));
	}

	return result;
}

mint::WeakReference mint_socket_set_tcp_option_number(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const auto option_id = to_integer<int>(cursor, option);
	const auto option_value = to_integer<int>(cursor, value);

	if (!mint::set_socket_option(socket_fd, IPPROTO_TCP, option_id, option_value)) {
		return mint::create_number(errno_from_io_last_error());
	}

	return {};
}

mint::WeakReference mint_socket_get_tcp_option_boolean(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option) {

	mint::WeakReference result = mint::create_iterator(cursor.ast());

	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const auto option_id = to_integer<int>(cursor, option);
	mint::sockopt_bool option_value = mint::sockopt_false;

	if (mint::get_socket_option(socket_fd, IPPROTO_TCP, option_id, &option_value)) {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_boolean(option_value != mint::sockopt_false));
	}
	else {
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(cursor, result.data<mint::Iterator>(), mint::create_number(errno_from_io_last_error()));
	}

	return result;
}

mint::WeakReference mint_socket_set_tcp_option_boolean(mint::Cursor& cursor, const mint::Reference& socket,
    mint::Reference& option, const mint::Reference& value) {

	const auto socket_fd = to_integer<SOCKET>(cursor, socket);
	const auto option_id = to_integer<int>(cursor, option);
	const mint::sockopt_bool option_value = to_boolean(value) ? mint::sockopt_true : mint::sockopt_false;

	if (!mint::set_socket_option(socket_fd, IPPROTO_TCP, option_id, option_value)) {
		return mint::create_number(errno_from_io_last_error());
	}

	return {};
}

}

MINT_EXPORT_FUNCTION(mint_tcp_ip_socket_open, 1);
MINT_EXPORT_FUNCTION(mint_tcp_ip_socket_send, 2);
MINT_EXPORT_FUNCTION(mint_tcp_ip_socket_recv, 2);
MINT_EXPORT_FUNCTION(mint_socket_setup_tcp_options, 1);
MINT_EXPORT_FUNCTION(mint_socket_get_tcp_option_number, 2);
MINT_EXPORT_FUNCTION(mint_socket_set_tcp_option_number, 3);
MINT_EXPORT_FUNCTION(mint_socket_get_tcp_option_boolean, 2);
MINT_EXPORT_FUNCTION(mint_socket_set_tcp_option_boolean, 3);
