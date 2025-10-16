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
#include "mint/memory/reference.h"
#include "mint/system/errno.h"
#include "scheduler.h"
#include "socket.h"
#include "ip.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#ifdef MINT_OS_WINDOWS
#include <cstdio>
#include <Windows.h>
#include <in6addr.h>
#include <inaddr.h>
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

namespace {

mint::WeakReference mint_udp_ip_socket_open(mint::Cursor& cursor, const mint::Reference& ip_version) {

	mint::WeakReference result = mint::create_iterator(cursor.ast());
	auto socket_fd = INVALID_SOCKET;

	switch (mint::to_integer<int>(cursor, ip_version)) {
	case mint::ip_version_4:
		socket_fd = Scheduler::instance().open_socket(AF_INET, SOCK_DGRAM, 0);
		break;
	case mint::ip_version_6:
		socket_fd = Scheduler::instance().open_socket(AF_INET6, SOCK_DGRAM, 0);
		break;
	default:
		iterator_yield(result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(result.data<mint::Iterator>(), mint::create_number(EOPNOTSUPP));
		return result;
	}

	if (socket_fd != INVALID_SOCKET) {
		iterator_yield(result.data<mint::Iterator>(), mint::create_unsigned_number(socket_fd));
		if (mint::set_socket_option(socket_fd, SO_REUSEADDR, mint::sockopt_true)) {
			iterator_yield(result.data<mint::Iterator>(), mint::create_none());
		}
		else {
			iterator_yield(result.data<mint::Iterator>(), mint::create_number(errno));
		}
	}
	else {
		iterator_yield(result.data<mint::Iterator>(), mint::create_none());
		iterator_yield(result.data<mint::Iterator>(), mint::create_number(errno_from_io_last_error()));
	}

	return result;
}

mint::WeakReference mint_udp_ip_socket_sendto(mint::FunctionHelper& helper, const mint::Reference& socket,
    mint::Reference& address, const mint::Reference& port, const mint::Reference& ip_version,
    const mint::Reference& buffer) {

	mint::WeakReference result = mint::create_iterator(helper.cursor().ast());

	const auto socket_fd = to_integer<SOCKET>(helper.cursor(), socket);
	const std::string address_str = to_string(address);
	std::vector<std::uint8_t>* buf = buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;
	auto io_status =
	    helper.reference(mint::symbols::network).member(mint::symbols::end_point).member(mint::symbols::io_status);

	std::unique_ptr<sockaddr> target;
	socklen_t targetlen = sizeof(sockaddr);

	switch (to_integer<int>(helper.cursor(), ip_version)) {
	case mint::ip_version_4:
		targetlen = sizeof(sockaddr_in);
		target.reset(reinterpret_cast<sockaddr*>(new sockaddr_in));
		memset(target.get(), 0, targetlen);
		reinterpret_cast<sockaddr_in*>(target.get())->sin_family = AF_INET;
		reinterpret_cast<sockaddr_in*>(target.get())->sin_port = htons(to_integer<std::uint16_t>(helper.cursor(), port));
		switch (
		    ::inet_pton(AF_INET, address_str.c_str(), &reinterpret_cast<sockaddr_in*>(target.get())->sin_addr.s_addr)) {
		case 0:
			iterator_yield(result.data<mint::Iterator>(), io_status.member(mint::symbols::io_error).share());
			iterator_yield(result.data<mint::Iterator>(), mint::create_number(EINVAL));
			return result;
		case 1:
			break;
		default:
			iterator_yield(result.data<mint::Iterator>(), io_status.member(mint::symbols::io_error).share());
			iterator_yield(result.data<mint::Iterator>(), mint::create_number(errno_from_io_last_error()));
			return result;
		}
		break;
	case mint::ip_version_6:
		targetlen = sizeof(sockaddr_in6);
		target.reset(reinterpret_cast<sockaddr*>(new sockaddr_in6));
		memset(target.get(), 0, targetlen);
		reinterpret_cast<sockaddr_in6*>(target.get())->sin6_family = AF_INET6;
		reinterpret_cast<sockaddr_in6*>(target.get())->sin6_port = htons(
		    to_integer<std::uint16_t>(helper.cursor(), port));
		switch (::inet_pton(AF_INET6, address_str.c_str(),
		    &reinterpret_cast<sockaddr_in6*>(target.get())->sin6_addr.s6_addr)) {
		case 0:
			iterator_yield(result.data<mint::Iterator>(), io_status.member(mint::symbols::io_error).share());
			iterator_yield(result.data<mint::Iterator>(), mint::create_number(EINVAL));
			return result;
		case 1:
			break;
		default:
			iterator_yield(result.data<mint::Iterator>(), io_status.member(mint::symbols::io_error).share());
			iterator_yield(result.data<mint::Iterator>(), mint::create_number(errno_from_io_last_error()));
			return result;
		}
		break;
	default:
		iterator_yield(result.data<mint::Iterator>(), io_status.member(mint::symbols::io_error).share());
		iterator_yield(result.data<mint::Iterator>(), mint::create_number(EOPNOTSUPP));
		return result;
	}

#ifdef MINT_OS_WINDOWS
	const auto flags = 0;
#else
	const auto flags = MSG_CONFIRM;
#endif

	const auto count = sendto(socket_fd, reinterpret_cast<const char*>(buf->data()), static_cast<int>(buf->size()),
	    flags, target.get(), targetlen);

	switch (count) {
	case -1:
		switch (const int error = errno_from_io_last_error()) {
		case EINPROGRESS:
		case EWOULDBLOCK:
			iterator_yield(result.data<mint::Iterator>(), io_status.member(mint::symbols::io_would_block).share());
			Scheduler::instance().set_socket_blocked(socket_fd, true);
			break;

		case EPIPE:
			iterator_yield(result.data<mint::Iterator>(), io_status.member(mint::symbols::io_closed).share());
			break;

		default:
			iterator_yield(result.data<mint::Iterator>(), io_status.member(mint::symbols::io_error).share());
			iterator_yield(result.data<mint::Iterator>(), mint::create_number(error));
			break;
		}
		break;
	case 0:
		iterator_yield(result.data<mint::Iterator>(), io_status.member(mint::symbols::io_closed).share());
		break;
	default:
		iterator_yield(result.data<mint::Iterator>(), io_status.member(mint::symbols::io_success).share());
		iterator_yield(result.data<mint::Iterator>(), mint::create_signed_number(count));
		break;
	}

	return result;
}

mint::WeakReference mint_udp_ip_socket_recvfrom(mint::FunctionHelper& helper, const mint::Reference& socket,
    mint::Reference& buffer) {

	mint::WeakReference result = mint::create_iterator(helper.cursor().ast());

	const auto socket_fd = to_integer<SOCKET>(helper.cursor(), socket);
	std::vector<std::uint8_t>* buf = buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;
	auto io_status =
	    helper.reference(mint::symbols::network).member(mint::symbols::end_point).member(mint::symbols::io_status);

	sockaddr source {};
	socklen_t sourcelen = sizeof(source);
	std::string address;
	u_short port = 0;

	socklen_t length = 0;
#ifdef MINT_OS_UNIX
	if (ioctl(socket_fd, SIOCINQ, &length) != -1) {
#else
	length = BUFSIZ; /// @todo get better value
#endif

		const auto flags = 0; // MSG_WAITALL;
		auto local_buffer = std::make_unique<std::uint8_t[]>(length);
		auto count = recvfrom(socket_fd, reinterpret_cast<char*>(local_buffer.get()), static_cast<int>(length), flags,
		    &source, &sourcelen);

		switch (count) {
		case -1:
			switch (const int error = errno_from_io_last_error()) {
			case EINPROGRESS:
			case EWOULDBLOCK:
				iterator_yield(result.data<mint::Iterator>(), io_status.member(mint::symbols::io_would_block).share());
				Scheduler::instance().set_socket_blocked(socket_fd, true);
				break;

			case EPIPE:
				iterator_yield(result.data<mint::Iterator>(), io_status.member(mint::symbols::io_closed).share());
				break;

			default:
				iterator_yield(result.data<mint::Iterator>(), io_status.member(mint::symbols::io_error).share());
				iterator_yield(result.data<mint::Iterator>(), mint::create_none());
				iterator_yield(result.data<mint::Iterator>(), mint::create_none());
				iterator_yield(result.data<mint::Iterator>(), mint::create_number(error));
				break;
			}
			break;
		case 0:
			iterator_yield(result.data<mint::Iterator>(), io_status.member(mint::symbols::io_closed).share());
			break;
		default:
			if (const int error = mint::get_ip_socket_info(&source, sourcelen, &address, &port)) {
				iterator_yield(result.data<mint::Iterator>(), io_status.member(mint::symbols::io_error).share());
				iterator_yield(result.data<mint::Iterator>(), mint::create_none());
				iterator_yield(result.data<mint::Iterator>(), mint::create_none());
				iterator_yield(result.data<mint::Iterator>(), mint::create_number(error));
			}
			else {
				iterator_yield(result.data<mint::Iterator>(), io_status.member(mint::symbols::io_success).share());
				iterator_yield(result.data<mint::Iterator>(), mint::create_string(helper.cursor().ast(), address));
				iterator_yield(result.data<mint::Iterator>(), mint::create_number(port));
				std::copy_n(local_buffer.get(), count, std::back_inserter(*buf));
			}
			break;
		}
#ifdef MINT_OS_UNIX
	}
	else {
		iterator_yield(result.data<mint::Iterator>(), io_status.member(mint::symbols::io_error).share());
		iterator_yield(result.data<mint::Iterator>(), mint::create_number(errno));
	}
#endif

	return result;
}

mint::WeakReference mint_udp_ip_socket_send(mint::FunctionHelper& helper, const mint::Reference& socket,
    mint::Reference& buffer) {

	mint::WeakReference result = mint::create_iterator(helper.cursor().ast());

	const auto socket_fd = to_integer<SOCKET>(helper.cursor(), socket);
	std::vector<std::uint8_t>* buf = buffer.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;
	auto io_status =
	    helper.reference(mint::symbols::network).member(mint::symbols::end_point).member(mint::symbols::io_status);

#ifdef MINT_OS_WINDOWS
	const auto flags = 0;
#else
	const auto flags = MSG_CONFIRM;
#endif

	const auto count = send(socket_fd, reinterpret_cast<const char*>(buf->data()), static_cast<int>(buf->size()), flags);

	switch (count) {
	case -1:
		switch (const int error = errno_from_io_last_error()) {
		case EINPROGRESS:
		case EWOULDBLOCK:
			iterator_yield(result.data<mint::Iterator>(), io_status.member(mint::symbols::io_would_block).share());
			Scheduler::instance().set_socket_blocked(socket_fd, true);
			break;

		case EPIPE:
			iterator_yield(result.data<mint::Iterator>(), io_status.member(mint::symbols::io_closed).share());
			break;

		default:
			iterator_yield(result.data<mint::Iterator>(), io_status.member(mint::symbols::io_error).share());
			iterator_yield(result.data<mint::Iterator>(), mint::create_number(error));
			break;
		}
		break;
	case 0:
		iterator_yield(result.data<mint::Iterator>(), io_status.member(mint::symbols::io_closed).share());
		break;
	default:
		iterator_yield(result.data<mint::Iterator>(), io_status.member(mint::symbols::io_success).share());
		iterator_yield(result.data<mint::Iterator>(), mint::create_signed_number(count));
		break;
	}

	return result;
}

mint::WeakReference mint_udp_ip_socket_recv(mint::FunctionHelper& helper, const mint::Reference& socket,
    mint::Reference& buffer) {

	mint::WeakReference result = create_iterator(helper.cursor().ast());

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

		const auto flags = MSG_WAITALL;
		auto local_buffer = std::make_unique<std::uint8_t[]>(length);
		auto count = recv(socket_fd, reinterpret_cast<char*>(local_buffer.get()), static_cast<int>(length), flags);

		switch (count) {
		case -1:
			switch (const int error = errno_from_io_last_error()) {
			case EINPROGRESS:
			case EWOULDBLOCK:
				iterator_yield(result.data<mint::Iterator>(), io_status.member(mint::symbols::io_would_block).share());
				Scheduler::instance().set_socket_blocked(socket_fd, true);
				break;

			case EPIPE:
				iterator_yield(result.data<mint::Iterator>(), io_status.member(mint::symbols::io_closed).share());
				break;

			default:
				iterator_yield(result.data<mint::Iterator>(), io_status.member(mint::symbols::io_error).share());
				iterator_yield(result.data<mint::Iterator>(), mint::create_number(error));
				break;
			}
			break;
		case 0:
			iterator_yield(result.data<mint::Iterator>(), io_status.member(mint::symbols::io_closed).share());
			break;
		default:
			iterator_yield(result.data<mint::Iterator>(), io_status.member(mint::symbols::io_success).share());
			std::copy_n(local_buffer.get(), count, std::back_inserter(*buf));
			break;
		}
#ifdef MINT_OS_UNIX
	}
	else {
		iterator_yield(result.data<mint::Iterator>(), io_status.member(mint::symbols::io_error).share());
		iterator_yield(result.data<mint::Iterator>(), mint::create_number(errno));
	}
#endif

	return result;
}

}

MINT_EXPORT_FUNCTION(mint_udp_ip_socket_open, 1);
MINT_EXPORT_FUNCTION(mint_udp_ip_socket_sendto, 5);
MINT_EXPORT_FUNCTION(mint_udp_ip_socket_recvfrom, 2);
MINT_EXPORT_FUNCTION(mint_udp_ip_socket_send, 2);
MINT_EXPORT_FUNCTION(mint_udp_ip_socket_recv, 2);
