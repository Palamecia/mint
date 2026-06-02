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

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <cstdint>
#include <array>
#include <iterator>
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
#include <fcntl.h>
#include <poll.h>
#include <stdio_ext.h>
#include <sys/file.h>
#include <sys/inotify.h>
#include <sys/poll.h>
#include <unistd.h>
#endif

namespace {

enum Changes : std::uint8_t {
	name = 0x01,
	data = 0x02,
	attributes = 0x04
};

#ifdef MINT_OS_UNIX
bool reset_event(int event_fd) {

	std::size_t len = 0;
	bool reseted = false;
	auto read_buffer = std::array<std::uint8_t, BUFSIZ>();

	while (const auto count = read(event_fd, read_buffer.data(), read_buffer.size())) {

		if (count < 0) {
			break;
		}

		for (std::uint8_t* ptr = read_buffer.data(); ptr < read_buffer.data() + count; ptr += len) {
			const inotify_event* event = reinterpret_cast<inotify_event*>(ptr);
			reseted = reseted || (event->mask != 0);
			len = sizeof(inotify_event) + event->len;
		}
	}

	return reseted;
}
#endif

mint::Reference mint_file_watcher_create(mint::Cursor& cursor, const mint::Reference& path, mint::Reference& flags) {

#ifdef MINT_OS_WINDOWS
	DWORD notify_filter = 0;

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
	if (HANDLE fe = FindFirstChangeNotificationW(path_str.c_str(), TRUE, notify_filter); fe != INVALID_HANDLE_VALUE) {
		return mint::create_handle(cursor.ast(), fe);
	}
#else
	uint32_t watch_flags = 0;

	if (to_unsigned_integer(cursor, flags) & Changes::name) {
		watch_flags |= IN_MOVE;
	}

	if (to_unsigned_integer(cursor, flags) & Changes::data) {
		watch_flags |= IN_CREATE | IN_MODIFY | IN_DELETE | IN_DELETE_SELF;
	}

	if (to_unsigned_integer(cursor, flags) & Changes::attributes) {
		watch_flags |= IN_ATTRIB;
	}

	const auto path_str = to_string(path);
	if (const auto fe = inotify_init1(IN_NONBLOCK); fe != -1) {
		if (inotify_add_watch(fe, path_str.c_str(), watch_flags)) {
			return mint::create_handle(cursor.ast(), fe);
		}
	}
#endif
	return {};
}

mint::Reference mint_file_watcher_close(mint::Cursor& /*cursor*/, const mint::Reference& handle) {
#ifdef MINT_OS_WINDOWS
	CloseHandle(to_handle(handle));
#else
	close(to_handle(handle));
#endif
	return {};
}

mint::Reference mint_file_watcher_wait(mint::Cursor& cursor, const mint::Reference& handle,
    const mint::Reference& timeout) {
#ifdef MINT_OS_WINDOWS

	const DWORD time_ms = mint::is_instance_of(timeout, mint::Data::Format::none)
	                          ? INFINITE
	                          : mint::to_integer<DWORD>(cursor, timeout);

	if (WaitForSingleObject(to_handle(handle), time_ms) == WAIT_OBJECT_0) {
		ResetEvent(to_handle(handle));
		return mint::create_boolean(true);
	}

	return mint::create_boolean(false);
#else
	pollfd fds {
	    .fd = to_handle(handle),
	    .events = POLLIN,
	};

	const int time_ms = is_instance_of(timeout, mint::Data::Format::none) ? -1 : to_integer<int>(cursor, timeout);

	if (const auto ret = poll(&fds, 1, time_ms); (ret > 0) && (fds.revents & POLLIN)) {
		return mint::create_boolean(reset_event(fds.fd));
	}

	return mint::create_boolean(false);
#endif
}

}

MINT_EXPORT_FUNCTION(mint_file_watcher_create, 2)
MINT_EXPORT_FUNCTION(mint_file_watcher_close, 1)
MINT_EXPORT_FUNCTION(mint_file_watcher_wait, 2)
