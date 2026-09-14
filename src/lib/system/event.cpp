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

#include "mint/memory/builtin/libobject.h"
#include "mint/memory/data.h"
#include "mint/memory/memory_tools.h"
#include "mint/memory/reference.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/cast_tools.h"
#include "mint/system/async_io.h"
#include <cstdint>

#ifdef MINT_OS_WINDOWS
#include <Windows.h>
#include <handleapi.h>
#include <minwindef.h>
#include <synchapi.h>
#include <winbase.h>
#else
#ifdef MINT_OS_LINUX
#include <sys/epoll.h>
#else
#include <sys/event.h>
#endif
#ifndef MINT_OS_MAC
#include <sys/eventfd.h>
#endif
#include <unistd.h>
#endif

namespace {

mint::Reference mint_event_create(mint::Cursor& cursor) {
#ifdef MINT_OS_WINDOWS

	if (HANDLE handle = CreateEvent(nullptr, false, false, nullptr); handle != INVALID_HANDLE_VALUE) {
		return mint::create_c_object(cursor.ast(), new mint::poll_event_t {
		                                               .handle = handle,
		                                           });
	}

#elifdef MINT_OS_LINUX

	if (const int fd = eventfd(0, EFD_NONBLOCK); fd != -1) {
		return mint::create_c_object(cursor.ast(), new mint::poll_event_t {
		                                               .fd = fd,
		                                               .events = EPOLLIN,
		                                               .on_signal =
		                                                   [](mint::poll_event_t& event) {
			                                                   auto counter = uint64_t();
			                                                   read(event.fd, &counter, sizeof(counter));
		                                                   },
		                                           });
	}

#elifdef MINT_OS_FREE_BSD

	if (const int fd = eventfd(0, EFD_NONBLOCK); fd != -1) {
		return mint::create_c_object(cursor.ast(), new mint::poll_event_t {
		                                               .fd = fd,
		                                               .filter = EVFILT_READ,
		                                           });
	}

#elifdef MINT_OS_MAC

	if (const int fd = eventfd(0, EFD_NONBLOCK); fd != -1) {
		return mint::create_c_object(cursor.ast(), new mint::poll_event_t {
		                                               .fd = fd,
		                                               .filter = EVFILT_READ,
		                                           });
	}

#endif
	return {};
}

mint::Reference mint_event_delete(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	auto* event = d_ptr.data<mint::LibObject<mint::poll_event_t>>().ptr;
#ifdef MINT_OS_WINDOWS
	CloseHandle(event->handle);
#else
	close(event->fd);
#endif
	delete event;
	return {};
}

mint::Reference mint_event_get_handle(mint::Cursor& cursor, const mint::Reference& d_ptr) {
#ifdef MINT_OS_WINDOWS
	return mint::create_handle(cursor.ast(), d_ptr.data<mint::LibObject<mint::poll_event_t>>().ptr->handle);
#elifdef MINT_OS_UNIX
	return mint::create_handle(cursor.ast(), d_ptr.data<mint::LibObject<mint::poll_event_t>>().ptr->fd);
#endif
}

mint::Reference mint_event_is_set(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
#ifdef MINT_OS_WINDOWS
	return mint::create_boolean(
	    WaitForSingleObject(d_ptr.data<mint::LibObject<mint::poll_event_t>>().ptr->handle, 0) == WAIT_OBJECT_0);
#else
	std::uint64_t value = 0;
	const auto fd = d_ptr.data<mint::LibObject<mint::poll_event_t>>().ptr->fd;
	read(fd, &value, sizeof(value));
	write(fd, &value, sizeof(value));
	return mint::create_boolean(value);
#endif
}

mint::Reference mint_event_set(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
#ifdef MINT_OS_WINDOWS
	return mint::create_boolean(SetEvent(d_ptr.data<mint::LibObject<mint::poll_event_t>>().ptr->handle));
#else
	std::uint64_t value = 1;
	const auto fd = d_ptr.data<mint::LibObject<mint::poll_event_t>>().ptr->fd;
	return mint::create_boolean(write(fd, &value, sizeof(value)) == sizeof(value));
#endif
}

mint::Reference mint_event_clear(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
#ifdef MINT_OS_WINDOWS
	ResetEvent(d_ptr.data<mint::LibObject<mint::poll_event_t>>().ptr->handle);
#else
	std::uint64_t value = 0;
	const auto fd = d_ptr.data<mint::LibObject<mint::poll_event_t>>().ptr->fd;
	read(fd, &value, sizeof(value));
#endif
	return {};
}

mint::Reference mint_event_wait(mint::Cursor& cursor, const mint::Reference& d_ptr, const mint::Reference& timeout) {
#ifdef MINT_OS_WINDOWS

	const DWORD time_ms = mint::is_instance_of(timeout, mint::Data::Format::none)
	                          ? INFINITE
	                          : mint::to_integer<DWORD>(cursor, timeout);

	if (WaitForSingleObject(d_ptr.data<mint::LibObject<mint::poll_event_t>>().ptr->handle, time_ms) == WAIT_OBJECT_0) {
		return mint::create_boolean(true);
	}

	return mint::create_boolean(false);

#elifdef MINT_OS_LINUX

	auto* event = d_ptr.data<mint::LibObject<mint::poll_event_t>>().ptr;
	const int timeout_ms = is_instance_of(timeout, mint::Data::Format::none) ? -1 : to_integer<int>(cursor, timeout);

	int context = epoll_create1(EPOLL_CLOEXEC);
	if (context == -1) {
		return mint::create_boolean(false);
	}

	auto changelist = epoll_event {
	    .events = event->events,
	    .data =
	        {
	            .fd = event->fd,
	        },
	};

	const auto ret = epoll_wait(context, &changelist, 1, timeout_ms);
	if (ret > 0 && changelist.events & EPOLLIN) {
		event->on_signal(*event);
	}

	::close(context);
	return mint::create_boolean(ret > 0);

#elifdef MINT_OS_UNIX

	auto* event = d_ptr.data<mint::LibObject<mint::poll_event_t>>().ptr;
	const auto timeout_ts = is_instance_of(timeout, mint::Data::Format::none)
	                            ? std::nullopt
	                            : std::optional<std::chrono::milliseconds>(to_integer<int>(cursor, timeout))
	                                  .transform([](std::chrono::milliseconds ms) {
		                                  const auto sec = std::chrono::floor<std::chrono::seconds>(ms);
		                                  const auto nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(
		                                      ms - sec);
		                                  return timespec {
		                                      .tv_sec = sec.count(),
		                                      .tv_nsec = nsec.count(),
		                                  };
	                                  });

	int context = kqueue();
	if (context == -1) {
		return mint::create_boolean(false);
	}

	struct kevent changelist {
	    .ident = static_cast<std::uintptr_t>(event->fd),
	    .filter = EVFILT_READ,
	};

	struct kevent eventlist {};
	const auto ret = kevent(context, &changelist, 1, &eventlist, 1, timeout_ts ? std::to_address(timeout_ts) : nullptr);

	::close(context);
	return mint::create_boolean(ret > 0);

#endif
}

}

MINT_EXPORT_FUNCTION(mint_event_create, 0);
MINT_EXPORT_FUNCTION(mint_event_delete, 1);
MINT_EXPORT_FUNCTION(mint_event_get_handle, 1);
MINT_EXPORT_FUNCTION(mint_event_is_set, 1);
MINT_EXPORT_FUNCTION(mint_event_set, 1);
MINT_EXPORT_FUNCTION(mint_event_clear, 1);
MINT_EXPORT_FUNCTION(mint_event_wait, 2);
