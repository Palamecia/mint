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

#include "mint/ast/cursor.h"
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/cast_tools.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/reference.h"
#include "mint/system/async_io.h"
#include "socket.h"
#include <bit>
#include <cstdint>
#include <ranges>
#include <system_error>
#include <vector>

#ifdef MINT_OS_WINDOWS
#include <Windows.h>
#include <minwindef.h>
#include <winerror.h>
#include <WinSock2.h>
#else
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace {

#ifdef MINT_OS_WINDOWS
using poll_handle_t = mint::handle_t;
#else
using poll_handle_t = pollfd;
#endif

struct PollFd {
	using Event = std::uint16_t;
	static constexpr Event read_event = 0x0001;
	static constexpr Event write_event = 0x0002;
	static constexpr Event accept_event = 0x0004;
	static constexpr Event error_event = 0x0008;
	static constexpr Event close_event = 0x0010;

	SOCKET fd;
	Event events;
	Event revents;
	mint::handle_t handle;
};

poll_handle_t to_poll_handle(const PollFd& desc) {
#ifdef MINT_OS_UNIX
	poll_handle_t handle {
	    .fd = desc.fd,
	    .events = 0,
	    .revents = 0,
	};

	if (desc.events & PollFd::read_event) {
		handle.events |= (POLLIN | POLLPRI);
	}
	if (desc.events & PollFd::write_event) {
		handle.events |= POLLOUT;
	}
	if (desc.events & PollFd::accept_event) {
		handle.events |= POLLIN;
	}
	if (desc.events & PollFd::error_event) {
		handle.events |= (POLLERR | POLLNVAL);
	}
	if (desc.events & PollFd::close_event) {
		handle.events |= POLLHUP;
#ifdef POLLRDHUP
		handle.events |= POLLRDHUP;
#endif
	}
	return handle;
#else
	long events = 0;

	if (desc.events & PollFd::read_event) {
		events |= FD_READ;
	}
	if (desc.events & PollFd::write_event) {
		events |= FD_WRITE;
	}
	if (desc.events & PollFd::accept_event) {
		events |= FD_ACCEPT;
	}
	if (desc.events & PollFd::close_event) {
		events |= FD_CLOSE;
	}

	if (mint_network::SocketManager::instance().is_socket_blocked(desc.fd)) {
		events |= FD_WRITE;
	}

	if (WSAEventSelect(desc.fd, desc.handle, events) == SOCKET_ERROR) {
		throw std::system_error(mint_network::last_socket_error_code());
	}

	return desc.handle;
#endif
}

bool revents_from_poll_handle(PollFd& desc, [[maybe_unused]] const poll_handle_t& handle) {

	bool fake_event = false;
	desc.revents = 0;

#ifdef MINT_OS_UNIX
	if ((handle.revents & (POLLIN | POLLPRI))
	    && !mint_network::SocketManager::instance().is_socket_listening(handle.fd)) {
		desc.revents |= PollFd::read_event;
	}
	if (handle.revents & POLLOUT) {
		desc.revents |= PollFd::write_event;
	}
	if ((handle.revents & POLLIN) && mint_network::SocketManager::instance().is_socket_listening(handle.fd)) {
		desc.revents |= PollFd::accept_event;
	}
	if (handle.revents & (POLLERR | POLLNVAL)) {
		desc.revents |= PollFd::error_event;
	}
	if (handle.revents & POLLHUP) {
		desc.revents |= PollFd::close_event;
	}
#ifdef POLLRDHUP
	if (handle.revents & POLLRDHUP) {
		desc.revents |= PollFd::close_event;
	}
#endif
#else
	auto events = WSANETWORKEVENTS();
	if (WSAEnumNetworkEvents(desc.fd, desc.handle, &events) == SOCKET_ERROR) {
		throw std::system_error(mint_network::last_socket_error_code());
	}

	if (events.lNetworkEvents & FD_READ) {
		if ((desc.events & PollFd::error_event) && events.iErrorCode[FD_READ_BIT]) {
			desc.revents |= PollFd::error_event;
		}
		desc.revents |= PollFd::read_event;
	}
	if (events.lNetworkEvents & FD_WRITE) {
		if ((desc.events & PollFd::error_event) && events.iErrorCode[FD_WRITE_BIT]) {
			desc.revents |= PollFd::error_event;
		}
		desc.revents |= PollFd::write_event;
	}
	if (events.lNetworkEvents & FD_ACCEPT) {
		if ((desc.events & PollFd::error_event) && events.iErrorCode[FD_ACCEPT_BIT]) {
			desc.revents |= PollFd::error_event;
		}
		desc.revents |= PollFd::accept_event;
	}
	if (events.lNetworkEvents & FD_CLOSE) {
		desc.revents |= PollFd::close_event;
	}

	if (mint_network::SocketManager::instance().is_socket_blocked(desc.fd)) {
		mint_network::SocketManager::instance().set_socket_blocked(desc.fd, events.lNetworkEvents & FD_WRITE);
	}
	else if (desc.events & PollFd::write_event) {
		desc.revents |= PollFd::write_event;
		fake_event = true;
	}
#endif

	return fake_event;
}

bool poll(std::vector<PollFd>& fdset, int timeout) {

	auto handles = std::vector(std::from_range, std::views::transform(fdset, &to_poll_handle));

#ifdef MINT_OS_UNIX
	bool result = ::poll(handles.data(), handles.size(), timeout) != 0;
#else
	bool result = WSAWaitForMultipleEvents(static_cast<DWORD>(handles.size()), handles.data(), false,
	                  static_cast<DWORD>(timeout), true)
	              != WSA_WAIT_TIMEOUT;
#endif

	for (const auto& [fd, handle] : std::views::zip(fdset, handles)) {
		if (revents_from_poll_handle(fd, handle)) {
			result = true;
		}
	}

	return result;
}

mint::Reference mint_pollfd_new(mint::Cursor& cursor, const mint::Reference& socket) {
	const auto socket_fd = std::bit_cast<SOCKET>(mint::to_handle(socket));
	return mint::create_c_object(cursor.ast(),
	    new PollFd {
	        .fd = socket_fd,
	        .events = 0,
	        .revents = 0,
#ifdef MINT_OS_WINDOWS
	        .handle = mint_network::SocketManager::instance().is_native_socket(socket_fd)
	                      ? WSACreateEvent()
	                      : CreateEvent(nullptr, TRUE, FALSE, nullptr),
#endif
	    });
}

mint::Reference mint_pollfd_delete(mint::Cursor& /*cursor*/, const mint::Reference& fd) {
#ifdef MINT_OS_WINDOWS
	WSACloseEvent(fd.data<mint::LibObject<PollFd>>().ptr->handle);
#endif
	delete fd.data<mint::LibObject<PollFd>>().ptr;
	return {};
}

mint::Reference mint_set_events(mint::Cursor& cursor, const mint::Reference& fd, const mint::Reference& events) {
	fd.data<mint::LibObject<PollFd>>().ptr->events = mint::to_integer<PollFd::Event>(cursor, events);
	return {};
}

mint::Reference mint_get_events(mint::Cursor& /*cursor*/, const mint::Reference& fd) {
	return mint::create_number(fd.data<mint::LibObject<PollFd>>().ptr->events);
}

mint::Reference mint_get_revents(mint::Cursor& /*cursor*/, const mint::Reference& fd) {
	return mint::create_number(fd.data<mint::LibObject<PollFd>>().ptr->revents);
}

mint::Reference mint_poll(mint::Cursor& cursor, const mint::Reference& handles, const mint::Reference& timeout) {

	auto fdset = std::vector(std::from_range,
	    std::views::transform(handles.data<mint::Array>().values, [](const auto& fd) {
		    return *fd.template data<mint::LibObject<PollFd>>().ptr;
	    }));

	try {
		const auto result = poll(fdset, mint::to_integer<int>(cursor, timeout));
		for (const auto& [fd, poll_fd] : std::views::zip(handles.data<mint::Array>().values, fdset)) {
			fd.data<mint::LibObject<PollFd>>().ptr->revents = poll_fd.revents;
		}
		return mint::create_iterator_from(cursor, mint::create_number(0), mint::create_boolean(result));
	}
	catch (const std::system_error& error) {
		return mint::create_iterator_from(cursor, mint::create_number(error.code().value()));
	}
}

}

MINT_EXPORT_FUNCTION(mint_pollfd_new, 1);
MINT_EXPORT_FUNCTION(mint_pollfd_delete, 1);
MINT_EXPORT_FUNCTION(mint_set_events, 2);
MINT_EXPORT_FUNCTION(mint_get_events, 1);
MINT_EXPORT_FUNCTION(mint_get_revents, 1);
MINT_EXPORT_FUNCTION(mint_poll, 2);
