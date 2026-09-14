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
#include "mint/memory/data.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/cast_tools.h"
#include "mint/memory/memory_tools.h"
#include "mint/memory/reference.h"
#include "mint/system/async_io.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <cstdint>
#include <array>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#ifdef MINT_OS_WINDOWS
#include <Windows.h>
#include <fileapi.h>
#include <handleapi.h>
#include <minwindef.h>
#include <synchapi.h>
#include <winbase.h>
#include <winnt.h>
#else
#ifdef MINT_OS_LINUX
#include <stdio_ext.h>
#include <sys/file.h>
#include <sys/inotify.h>
#include <sys/epoll.h>
#else
#include <sys/event.h>
#endif
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#endif

namespace {

enum Changes : std::uint8_t {
	name = 0x01,
	data = 0x02,
	attributes = 0x04
};

mint::Reference mint_file_watcher_create(mint::Cursor& cursor, const mint::Reference& path, mint::Reference& flags) {
#ifdef MINT_OS_WINDOWS

	auto notify_filter = DWORD();

	if (to_unsigned_integer(cursor, flags) & Changes::name) {
		notify_filter |= FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME;
	}

	if (to_unsigned_integer(cursor, flags) & Changes::data) {
		notify_filter |= FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE;
	}

	if (to_unsigned_integer(cursor, flags) & Changes::attributes) {
		notify_filter |= FILE_NOTIFY_CHANGE_ATTRIBUTES;
	}

	const auto path_str = std::filesystem::path(to_string(path)).generic_wstring();
	auto event = std::make_unique<mint::poll_event_t>(
	    FindFirstChangeNotificationW(path_str.c_str(), TRUE, notify_filter));
	if (*event == INVALID_HANDLE_VALUE) {
		return {};
	}

	return mint::create_c_object(cursor.ast(), event.release());

#elifdef MINT_OS_LINUX

	auto event = std::make_unique<mint::poll_event_t>(mint::poll_event_t {
	    .fd = inotify_init1(IN_NONBLOCK),
	    .events = EPOLLIN,
	});

	if (event->fd == -1) {
		return {};
	}

	auto watch_flags = std::uint32_t();

	if (to_unsigned_integer(cursor, flags) & Changes::name) {
		watch_flags |= IN_MOVE;
	}

	if (to_unsigned_integer(cursor, flags) & Changes::data) {
		watch_flags |= IN_CREATE | IN_MODIFY | IN_DELETE | IN_DELETE_SELF;
	}

	if (to_unsigned_integer(cursor, flags) & Changes::attributes) {
		watch_flags |= IN_ATTRIB;
	}

	if (!inotify_add_watch(event->fd, to_string(path).c_str(), watch_flags)) {
		return {};
	}

	return mint::create_c_object(cursor.ast(), event.release());

#elifdef MINT_OS_UNIX

	auto event = std::make_unique<mint::poll_event_t>(mint::poll_event_t {
#ifdef MINT_OS_MACOS
	    .fd = open(to_string(path).c_str(), O_RDONLY | O_EVTONLY | O_NONBLOCK),
#else
	    .fd = open(to_string(path).c_str(), O_RDONLY | O_NONBLOCK),
#endif
	    .filter = EVFILT_VNODE,
	});

	if (event->fd == -1) {
		return {};
	}

	if (to_unsigned_integer(cursor, flags) & Changes::name) {
		event->fflags |= NOTE_RENAME;
	}
	if (to_unsigned_integer(cursor, flags) & Changes::data) {
		event->fflags |= NOTE_WRITE | NOTE_DELETE;
	}
	if (to_unsigned_integer(cursor, flags) & Changes::attributes) {
		event->fflags |= NOTE_ATTRIB;
	}

	return mint::create_c_object(cursor.ast(), event.release());
#endif
}

mint::Reference mint_file_watcher_close(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
	mint::poll_event_t* event = d_ptr.data<mint::LibObject<mint::poll_event_t>>().ptr;
#ifdef MINT_OS_WINDOWS
	CloseHandle(*event);
#else
	close(event->fd);
#endif
	delete event;
	return {};
}

mint::Reference mint_file_watcher_get_handle(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	mint::poll_event_t* event = d_ptr.data<mint::LibObject<mint::poll_event_t>>().ptr;
#ifdef MINT_OS_WINDOWS
	return mint::create_handle(cursor.ast(), *event);
#else
	return mint::create_handle(cursor.ast(), event->fd);
#endif
}

mint::Reference mint_file_watcher_wait(mint::Cursor& cursor, const mint::Reference& d_ptr,
    const mint::Reference& timeout) {

	mint::poll_event_t* event = d_ptr.data<mint::LibObject<mint::poll_event_t>>().ptr;

#ifdef MINT_OS_WINDOWS

	const DWORD timeout_ms = mint::is_instance_of(timeout, mint::Data::Format::none)
	                             ? INFINITE
	                             : mint::to_integer<DWORD>(cursor, timeout);

	if (WaitForSingleObject(to_handle(d_ptr), timeout_ms) == WAIT_OBJECT_0) {
		ResetEvent(to_handle(d_ptr));
		return mint::create_boolean(true);
	}

	return mint::create_boolean(false);

#elifdef MINT_OS_LINUX

	const int timeout_ms = is_instance_of(timeout, mint::Data::Format::none) ? -1 : to_integer<int>(cursor, timeout);

	int context = epoll_create1(EPOLL_CLOEXEC);
	if (context == -1) {
		return mint::create_boolean(false);
	}

	auto changelist = epoll_event {
	    .events = event->events,
	};

	const auto ret = epoll_wait(context, &changelist, 1, timeout_ms);

	::close(context);
	return mint::create_boolean(ret > 0);

#elifdef MINT_OS_UNIX

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
	    .filter = event->filter,
	    .flags = event->flags,
	    .fflags = event->fflags,
	};

	struct kevent eventlist {};
	const auto ret = kevent(context, &changelist, 1, &eventlist, 1, timeout_ts ? std::to_address(timeout_ts) : nullptr);

	::close(context);
	return mint::create_boolean(ret > 0);

#endif
}

}

MINT_EXPORT_FUNCTION(mint_file_watcher_create, 2)
MINT_EXPORT_FUNCTION(mint_file_watcher_close, 1)
MINT_EXPORT_FUNCTION(mint_file_watcher_get_handle, 1)
MINT_EXPORT_FUNCTION(mint_file_watcher_wait, 2)
