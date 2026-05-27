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
bool mint_sflags(const char* mode, int* optr) {

	int m, o;

	switch (*mode++) {
	case 'r': /* open for reading */
		m = O_RDONLY;
		o = 0;
		break;
	case 'w': /* open for writing */
		m = O_WRONLY;
		o = O_CREAT | O_TRUNC;
		break;
	case 'a': /* open for appending */
		m = O_WRONLY;
		o = O_CREAT | O_APPEND;
		break;
	default: /* illegal mode */
		errno = EINVAL;
		return false;
	}

	switch (*mode) {
	case '+': /* [rwa]\+ means read and write */
		m = O_RDWR;
		break;
	case '\0': /* no more flags */
		break;
	default: /* illegal mode */
		errno = EINVAL;
		return false;
	}

	*optr = m | o;
	return true;
}

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
#else
bool mint_sflags(const char* mode, DWORD* desired_access, DWORD* creation_disposition) {

	switch (*mode++) {
	case 'r': /* open for reading */
		*desired_access = GENERIC_READ;
		*creation_disposition = OPEN_EXISTING;
		break;
	case 'w': /* open for writing */
		*desired_access = GENERIC_WRITE;
		*creation_disposition = TRUNCATE_EXISTING;
		break;
	case 'a': /* open for appending */
		*desired_access = GENERIC_READ | GENERIC_WRITE;
		*creation_disposition = CREATE_ALWAYS;
		break;
	default: /* illegal mode */
		errno = EINVAL;
		return false;
	}

	/* [rwa]\+ or [rwa]b\+ means read and write */
	if (*mode == '+' || (*mode == 'b' && mode[1] == '+')) {
		*desired_access = GENERIC_READ | GENERIC_WRITE;
	}

	return true;
}
#endif

mint::Reference mint_file_create(mint::Cursor& cursor, const mint::Reference& path, const mint::Reference& mode,
    mint::Reference& flags) {

	mint::Reference handles = mint::create_iterator(cursor.ast());

#ifdef MINT_OS_WINDOWS
	DWORD desired_access = 0;
	DWORD creation_disposition = 0;
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

	const auto mode_str = to_string(mode);
	if (mint_sflags(mode_str.c_str(), &desired_access, &creation_disposition)) {
		const auto path_str = std::filesystem::path(to_string(path)).generic_wstring();
		HANDLE fd = CreateFileW(path_str.c_str(), desired_access, desired_access, nullptr, creation_disposition,
		    FILE_ATTRIBUTE_NORMAL, nullptr);
		if (fd != INVALID_HANDLE_VALUE) {
			iterator_yield(cursor, handles.data<mint::Iterator>(), mint::create_handle(cursor.ast(), fd));
			HANDLE fe = FindFirstChangeNotificationW(path_str.c_str(), TRUE, notify_filter);
			if (fe != INVALID_HANDLE_VALUE) {
				iterator_yield(cursor, handles.data<mint::Iterator>(), mint::create_handle(cursor.ast(), fe));
			}
		}
	}
#else
	int open_flags = 0;
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

	const auto mode_str = to_string(mode);
	if (mint_sflags(mode_str.c_str(), &open_flags)) {
		const auto path_str = to_string(path);
		if (const auto fd = open(path_str.c_str(), open_flags | O_NONBLOCK); fd != -1) {
			iterator_yield(cursor, handles.data<mint::Iterator>(), create_handle(cursor.ast(), fd));
			const auto fe = inotify_init1(IN_NONBLOCK);
			if (fe != -1) {
				if (inotify_add_watch(fe, path_str.c_str(), watch_flags)) {
					iterator_yield(cursor, handles.data<mint::Iterator>(), create_handle(cursor.ast(), fe));
				}
			}
		}
	}
#endif
	return handles;
}

mint::Reference mint_file_close_file(mint::Cursor& /*cursor*/, const mint::Reference& handle) {
#ifdef MINT_OS_WINDOWS
	CloseHandle(to_handle(handle));
#else
	close(to_handle(handle));
#endif
	return {};
}

mint::Reference mint_file_close_event(mint::Cursor& /*cursor*/, const mint::Reference& handle) {
#ifdef MINT_OS_WINDOWS
	CloseHandle(to_handle(handle));
#else
	close(to_handle(handle));
#endif
	return {};
}

mint::Reference mint_file_read(mint::Cursor& /*cursor*/, const mint::Reference& file_handle,
    const mint::Reference& event_handle, const mint::Reference& stream) {
#ifdef MINT_OS_WINDOWS
	DWORD count = 0;
	std::array<std::uint8_t, BUFSIZ> read_buffer = {};
	std::vector<std::uint8_t>* stream_buffer = stream.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;

	while (
	    ReadFile(to_handle(file_handle), read_buffer.data(), static_cast<DWORD>(read_buffer.size()), &count, nullptr)) {
		std::copy_n(read_buffer.data(), count, std::back_inserter(*stream_buffer));
	}

	ResetEvent(to_handle(event_handle));
#else
	std::array<std::uint8_t, BUFSIZ> read_buffer = {};
	std::vector<std::uint8_t>* stream_buffer = stream.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;

	while (auto count = read(to_handle(file_handle), read_buffer.data(), read_buffer.size())) {
		if (count < 0) {
			break;
		}
		std::copy_n(read_buffer.data(), count, std::back_inserter(*stream_buffer));
	}

	reset_event(to_handle(event_handle));
#endif
	return {};
}

mint::Reference mint_file_write(mint::Cursor& /*cursor*/, const mint::Reference& handle, const mint::Reference& stream) {
#ifdef MINT_OS_WINDOWS
	std::vector<std::uint8_t>* buffer = stream.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;
	WriteFile(to_handle(handle), buffer->data(), static_cast<DWORD>(buffer->size()), nullptr, nullptr);
#else
	std::vector<std::uint8_t>* buffer = stream.data<mint::LibObject<std::vector<std::uint8_t>>>().ptr;
	write(to_handle(handle), buffer->data(), buffer->size());
#endif
	return {};
}

mint::Reference mint_file_wait(mint::Cursor& cursor, const mint::Reference& handle, const mint::Reference& timeout) {
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

MINT_EXPORT_FUNCTION(mint_file_create, 3)
MINT_EXPORT_FUNCTION(mint_file_close_file, 1)
MINT_EXPORT_FUNCTION(mint_file_close_event, 1)
MINT_EXPORT_FUNCTION(mint_file_read, 3)
MINT_EXPORT_FUNCTION(mint_file_write, 2)
MINT_EXPORT_FUNCTION(mint_file_wait, 2)
