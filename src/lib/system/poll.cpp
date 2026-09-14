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
#include "mint/ast/symbol.h"
#include "mint/memory/builtin/iterator.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/data.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/memory_tools.h"
#include "mint/memory/cast_tools.h"
#include "mint/system/async_io.h"
#include "mint/system/errno.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <ranges>
#include <span>
#include <vector>

#ifdef MINT_OS_WINDOWS
#include <Windows.h>
#include <handleapi.h>
#include <minwindef.h>
#include <synchapi.h>
#include <winbase.h>
#elifdef MINT_OS_LINUX
#include <sys/epoll.h>
#elifdef MINT_OS_UNIX
#include <unistd.h>
#endif

namespace symbols {

static const auto event = mint::Symbol("event");

}

namespace {

#ifdef MINT_OS_WINDOWS
struct WaitObjectEvent {
	HANDLE handle = INVALID_HANDLE_VALUE;
	void* data = nullptr;
};
#endif

mint::Reference mint_poll_new(mint::Cursor& cursor) {
#ifdef MINT_OS_WINDOWS
	return mint::create_c_object(cursor.ast(), new std::vector<WaitObjectEvent>());
#elifdef MINT_OS_LINUX
	if (auto fd = ::epoll_create1(EPOLL_CLOEXEC); fd != -1) {
		return mint::create_handle(cursor.ast(), fd);
	}
#elifdef MINT_OS_UNIX
	if (auto fd = ::kqueue(); fd != -1) {
		return mint::create_handle(cursor.ast(), fd);
	}
#else
#error "Poll is not implemented for this platform"
#endif
	return {};
}

mint::Reference mint_poll_delete(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
#ifdef MINT_OS_WINDOWS
	delete d_ptr.data<mint::LibObject<std::vector<WaitObjectEvent>>>().ptr;
#elifdef MINT_OS_LINUX
	::close(mint::to_handle(d_ptr));
#elifdef MINT_OS_UNIX
	::close(mint::to_handle(d_ptr));
#else
#error "Poll is not implemented for this platform"
#endif
	return {};
}

mint::Reference mint_poll_watch(mint::Cursor& cursor, const mint::Reference& d_ptr, const mint::Reference& event) {
#ifdef MINT_OS_WINDOWS
	auto* context = d_ptr.data<mint::LibObject<std::vector<WaitObjectEvent>>>().ptr;
	auto* handle = mint::get_member_ignore_visibility(cursor.ast(), event, symbols::event)
	                   .data<mint::LibObject<mint::poll_event_t>>()
	                   .ptr;
	const auto found = std::ranges::any_of(*context, [handle](const auto& event) {
		return event.handle == handle;
	});
	if (found) {
		return mint::create_number(EEXIST);
	}
	context->emplace_back(WaitObjectEvent {
	    .handle = *handle,
	    .data = &event.data(),
	});
	return mint::create_number(0);
#elifdef MINT_OS_LINUX

	auto* poll_event = mint::get_member_ignore_visibility(cursor.ast(), event, symbols::event)
	                       .data<mint::LibObject<mint::poll_event_t>>()
	                       .ptr;
	auto e = epoll_event {
	    .events = poll_event->events,
	    .data {
	        .ptr = &event.data(),
	    },
	};

	if (::epoll_ctl(mint::to_handle(d_ptr), EPOLL_CTL_ADD, poll_event->fd, &e) == -1) {
		return mint::create_number(errno);
	}

	return mint::create_number(0);

#elifdef MINT_OS_UNIX

	auto* poll_event = mint::get_member_ignore_visibility(cursor.ast(), event, symbols::event)
	                       .data<mint::LibObject<mint::poll_event_t>>()
	                       .ptr;
	struct kevent kev {
	    .ident = static_cast<std::uintptr_t>(poll_event->fd),
	    .filter = poll_event->filter,
	    .flags = poll_event->flags,
	    .fflags = poll_event->fflags,
	    .udata = &event.data(),
	};

	if (::kevent(mint::to_handle(d_ptr), &kev, 1, nullptr, 0, nullptr) == -1) {
		return mint::create_number(errno);
	}

	return mint::create_number(0);

#else
#error "Poll is not implemented for this platform"
#endif
}

mint::Reference mint_poll_unwatch(mint::Cursor& cursor, const mint::Reference& d_ptr, const mint::Reference& event) {
#ifdef MINT_OS_WINDOWS

	auto* context = d_ptr.data<mint::LibObject<std::vector<WaitObjectEvent>>>().ptr;
	auto* poll_event = mint::get_member_ignore_visibility(cursor.ast(), event, symbols::event)
	                       .data<mint::LibObject<mint::poll_event_t>>()
	                       .ptr;

	auto [it, end] = std::ranges::remove(*context, poll_event, &WaitObjectEvent::handle);
	if (it == end) {
		return mint::create_number(ENOENT);
	}
	context->erase(it, end);
	return mint::create_number(0);
#elifdef MINT_OS_LINUX

	auto* poll_event = mint::get_member_ignore_visibility(cursor.ast(), event, symbols::event)
	                       .data<mint::LibObject<mint::poll_event_t>>()
	                       .ptr;

	if (::epoll_ctl(mint::to_handle(d_ptr), EPOLL_CTL_DEL, poll_event->fd, nullptr) == -1) {
		return mint::create_number(errno);
	}

	return mint::create_number(0);

#elifdef MINT_OS_UNIX

	auto* poll_event = mint::get_member_ignore_visibility(cursor.ast(), event, symbols::event)
	                       .data<mint::LibObject<mint::poll_event_t>>()
	                       .ptr;
	struct kevent kev {
	    .ident = static_cast<std::uintptr_t>(poll_event->fd),
	    .filter = poll_event->filter,
	    .flags = EV_DELETE,
	};

	if (::kevent(mint::to_handle(d_ptr), &kev, 1, nullptr, 0, nullptr) == -1) {
		return mint::create_number(errno);
	}

	return mint::create_number(0);

#else
#error "Poll is not implemented for this platform"
#endif
}

mint::Reference mint_poll(mint::Cursor& cursor, const mint::Reference& d_ptr, const mint::Reference& triggered,
    const mint::Reference& timeout) {

#ifdef MINT_OS_WINDOWS

	auto* context = d_ptr.data<mint::LibObject<std::vector<WaitObjectEvent>>>().ptr;
	auto events = std::vector(std::from_range, std::views::transform(*context, &WaitObjectEvent::handle));
	const DWORD timeout_ms = mint::is_instance_of(timeout, mint::Data::Format::none)
	                             ? INFINITE
	                             : mint::to_integer<DWORD>(cursor, timeout);

	DWORD status = WaitForMultipleObjectsEx(static_cast<DWORD>(events.size()), events.data(), false, timeout_ms, true);
	if (status == WAIT_FAILED) {
		return mint::create_number(mint::errno_from_last_error());
	}

	while (status == WAIT_IO_COMPLETION) {
		status = WaitForMultipleObjectsEx(static_cast<DWORD>(events.size()), events.data(), false, 0, true);
	}

	for (auto& event : std::views::drop(*context, status - WAIT_OBJECT_0 + 1)) {
		if (WaitForSingleObjectEx(event.handle, 0, true) == WAIT_OBJECT_0) {
			mint::iterator_yield(cursor, triggered.data<mint::Iterator>(),
			    mint::Reference(mint::Reference::temporary, *static_cast<mint::Object*>(event.data)));
		}
	}

	return mint::create_number(0);

#elifdef MINT_OS_LINUX

	auto events = std::array<epoll_event, 64>();
	const int timeout_ms = is_instance_of(timeout, mint::Data::Format::none) ? -1 : to_integer<int>(cursor, timeout);

	const auto count = ::epoll_wait(mint::to_handle(d_ptr), events.data(), events.size(), timeout_ms);
	if (count == -1) {
		return mint::create_number(errno);
	}

	for (auto& event : std::span(events.data(), static_cast<std::size_t>(count))) {
		mint::iterator_yield(cursor, triggered.data<mint::Iterator>(),
		    mint::Reference(mint::Reference::temporary, *static_cast<mint::Object*>(event.data.ptr)));
	}

	return mint::create_number(0);

#elifdef MINT_OS_UNIX

	auto events = std::array<struct kevent, 64>();
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

	const int count = ::kevent(mint::to_handle(d_ptr), nullptr, 0, events.data(), events.size(),
	    timeout_ts ? std::to_address(timeout_ts) : nullptr);

	if (count == -1) {
		return mint::create_number(errno);
	}

	for (auto& event : std::span(events.data(), static_cast<std::size_t>(count))) {
		mint::iterator_yield(cursor, triggered.data<mint::Iterator>(),
		    mint::Reference(mint::Reference::temporary, *static_cast<mint::Object*>(event.udata)));
	}

	return mint::create_number(0);
#else
#error "Poll is not implemented for this platform"
#endif
}

mint::Reference mint_poll_event_from_handle(mint::Cursor& cursor, const mint::Reference& handle) {
#ifdef MINT_OS_WINDOWS
	return mint::create_c_object(cursor.ast(), new mint::poll_event_t(to_handle(handle)));
#elifdef MINT_OS_LINUX
	return mint::create_c_object(cursor.ast(), new mint::poll_event_t {
	                                               .fd = to_handle(handle),
	                                               .events = EPOLLIN,
	                                           });
#elifdef MINT_OS_UNIX
	return mint::create_c_object(cursor.ast(), new mint::poll_event_t {
	                                               .fd = to_handle(handle),
	                                               .filter = EVFILT_READ,
	                                           });
#else
#error "Poll is not implemented for this platform"
#endif
}

}

MINT_EXPORT_FUNCTION(mint_poll_new, 0)
MINT_EXPORT_FUNCTION(mint_poll_delete, 1)
MINT_EXPORT_FUNCTION(mint_poll_watch, 2)
MINT_EXPORT_FUNCTION(mint_poll_unwatch, 2)
MINT_EXPORT_FUNCTION(mint_poll, 3)
MINT_EXPORT_FUNCTION(mint_poll_event_from_handle, 1)
