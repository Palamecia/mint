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
#include <mint/system/filesystem.h>

#ifdef OS_WINDOWS
#include <Windows.h>
#else
#include <sys/file.h>
#include <sys/inotify.h>
#include <stdio_ext.h>
#include <unistd.h>
#include <poll.h>
#endif

using namespace mint;

enum Changes {
	name = 0x01,
	data = 0x02,
	attributes = 0x04
};

#ifdef OS_UNIX
static bool mint_sflags(const char *mode, int *optr) {

	int m, o;

	switch (*mode++) {
	case 'r':	/* open for reading */
		m = O_RDONLY;
		o = 0;
		break;
	case 'w':	/* open for writing */
		m = O_WRONLY;
		o = O_CREAT | O_TRUNC;
		break;
	case 'a':	/* open for appending */
		m = O_WRONLY;
		o = O_CREAT | O_APPEND;
		break;
	default:	/* illegal mode */
		errno = EINVAL;
		return false;
	}

	switch (*mode) {
	case '+':	/* [rwa]\+ means read and write */
		m = O_RDWR;
		break;
	case '\0':	/* no more flags */
		break;
	default:	/* illegal mode */
		errno = EINVAL;
		return false;
	}

	*optr = m | o;
	return true;
}

bool reset_event(int event_fd) {

	size_t len = 0;
	bool reseted = false;
	uint8_t read_buffer[BUFSIZ];

	while (ssize_t count = read(event_fd, read_buffer, sizeof(read_buffer))) {

		if (count < 0) {
			break;
		}

		for (uint8_t *ptr = read_buffer; ptr < read_buffer + count; ptr += len) {
			const inotify_event *event = reinterpret_cast<inotify_event *>(ptr);
			reseted = reseted || (event->mask != 0);
			len = sizeof(inotify_event) + event->len;
		}
	}

	return reseted;
}
#else
static bool mint_sflags(const char *mode, DWORD *dwDesiredAccess, DWORD *dwCreationDisposition) {

	switch (*mode++) {
	case 'r':	/* open for reading */
		*dwDesiredAccess = GENERIC_READ;
		*dwCreationDisposition = OPEN_EXISTING;
		break;
	case 'w':	/* open for writing */
		*dwDesiredAccess = GENERIC_WRITE;
		*dwCreationDisposition = TRUNCATE_EXISTING;
		break;
	case 'a':	/* open for appending */
		*dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
		*dwCreationDisposition = CREATE_ALWAYS;
		break;
	default:	/* illegal mode */
		errno = EINVAL;
		return false;
	}

	/* [rwa]\+ or [rwa]b\+ means read and write */
	if (*mode == '+' || (*mode == 'b' && mode[1] == '+')) {
		*dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
	}

	return true;
}
#endif

MINT_FUNCTION(mint_file_create, 3, cursor) {

	FunctionHelper helper(cursor, 3);

	WeakReference flags = std::move(helper.pop_parameter());
	WeakReference mode = std::move(helper.pop_parameter());
	WeakReference path = std::move(helper.pop_parameter());

	WeakReference handles = create_iterator();

#ifdef OS_WINDOWS
	DWORD dwDesiredAccess, dwCreationDisposition, dwNotifyFilter;

	dwNotifyFilter = 0;

	if (static_cast<intmax_t>(to_number(cursor, flags)) & Changes::name) {
		dwNotifyFilter |= FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME;
	}

	if (static_cast<intmax_t>(to_number(cursor, flags)) & Changes::data) {
		dwNotifyFilter |= FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE;
	}

	if (static_cast<intmax_t>(to_number(cursor, flags)) & Changes::attributes) {
		dwNotifyFilter |= FILE_NOTIFY_CHANGE_ATTRIBUTES;
	}

	std::string mode_str = to_string(mode);
	if (mint_sflags(mode_str.c_str(), &dwDesiredAccess, &dwCreationDisposition)) {
		std::wstring path_str = string_to_windows_path(to_string(path));
		HANDLE fd = CreateFileW(path_str.c_str(), dwDesiredAccess, dwDesiredAccess, nullptr, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (fd != INVALID_HANDLE_VALUE) {
			iterator_insert(handles.data<Iterator>(), create_handle(fd));
			HANDLE fe = FindFirstChangeNotificationW(path_str.c_str(), TRUE, dwNotifyFilter);
			if (fe != INVALID_HANDLE_VALUE) {
				iterator_insert(handles.data<Iterator>(), create_handle(fe));
			}
		}
	}
#else
	int open_flags = 0;
	uint32_t watch_flags = 0;

	if (static_cast<intmax_t>(to_number(cursor, flags)) & Changes::name) {
		watch_flags |= IN_MOVE;
	}

	if (static_cast<intmax_t>(to_number(cursor, flags)) & Changes::data) {
		watch_flags |= IN_CREATE | IN_MODIFY | IN_DELETE | IN_DELETE_SELF;
	}

	if (static_cast<intmax_t>(to_number(cursor, flags)) & Changes::attributes) {
		watch_flags |= IN_ATTRIB;
	}

	std::string mode_str = to_string(mode);
	if (mint_sflags(mode_str.c_str(), &open_flags)) {
		std::string path_str = to_string(path);
		int fd = open(path_str.c_str(), open_flags | O_NONBLOCK);
		if (fd != -1) {
			iterator_insert(handles.data<Iterator>(), create_handle(fd));
			int fe = inotify_init1(IN_NONBLOCK);
			if (fe != -1) {
				if (inotify_add_watch(fe, path_str.c_str(), watch_flags)) {
					iterator_insert(handles.data<Iterator>(), create_handle(fe));
				}
			}
		}
	}
#endif
	helper.return_value(std::move(handles));
}

MINT_FUNCTION(mint_file_close_file, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	mint::handle_t handle = to_handle(helper.pop_parameter());

#ifdef OS_WINDOWS
	CloseHandle(handle);
#else
	close(handle);
#endif
}

MINT_FUNCTION(mint_file_close_event, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	mint::handle_t handle = to_handle(helper.pop_parameter());

#ifdef OS_WINDOWS
	CloseHandle(handle);
#else
	close(handle);
#endif
}

MINT_FUNCTION(mint_file_read, 3, cursor) {

	FunctionHelper helper(cursor, 3);

	WeakReference stream = std::move(helper.pop_parameter());

#ifdef OS_WINDOWS
	DWORD count = 0;
	uint8_t read_buffer[BUFSIZ];
	mint::handle_t event_handle = to_handle(helper.pop_parameter());
	mint::handle_t file_handle = to_handle(helper.pop_parameter());
	std::vector<uint8_t> *stream_buffer = stream.data<LibObject<std::vector<uint8_t>>>()->impl;

	while (ReadFile(file_handle, read_buffer, sizeof(read_buffer), &count, nullptr)) {
		copy_n(read_buffer, count, back_inserter(*stream_buffer));
	}

	ResetEvent(event_handle);
#else
	uint8_t read_buffer[BUFSIZ];
	mint::handle_t fe = to_handle(helper.pop_parameter());
	mint::handle_t fd = to_handle(helper.pop_parameter());
	std::vector<uint8_t> *stream_buffer = stream.data<LibObject<std::vector<uint8_t>>>()->impl;

	while (ssize_t count = read(fd, read_buffer, sizeof (read_buffer))) {

		if (count < 0) {
			break;
		}

		copy_n(read_buffer, count, back_inserter(*stream_buffer));
	}

	reset_event(fe);
#endif
}

MINT_FUNCTION(mint_file_write, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	WeakReference stream = std::move(helper.pop_parameter());

#ifdef OS_WINDOWS
	mint::handle_t handle = to_handle(helper.pop_parameter());
	std::vector<uint8_t> *buffer = stream.data<LibObject<std::vector<uint8_t>>>()->impl;

	WriteFile(handle, buffer->data(), static_cast<DWORD>(buffer->size()), nullptr, nullptr);
#else
	mint::handle_t fd = to_handle(helper.pop_parameter());
	std::vector<uint8_t> *buffer = stream.data<LibObject<std::vector<uint8_t>>>()->impl;

	write(fd, buffer->data(), buffer->size());
#endif
}

MINT_FUNCTION(mint_file_wait, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	WeakReference timeout = std::move(helper.pop_parameter());

#ifdef OS_WINDOWS

	DWORD time_ms = INFINITE;
	mint::handle_t handle = to_handle(helper.pop_parameter());

	if (timeout.data()->format != Data::fmt_none) {
		time_ms = static_cast<int>(to_integer(cursor, timeout));
	}

	bool result = false;

	if (WaitForSingleObject(handle, time_ms) == WAIT_OBJECT_0) {
		ResetEvent(handle);
		result = true;
	}

	helper.return_value(create_boolean(result));
#else
	pollfd fds;
	fds.events = POLLIN;
	fds.fd = to_handle(helper.pop_parameter());

	int time_ms = -1;

	if (timeout.data()->format != Data::fmt_none) {
		time_ms = static_cast<int>(to_integer(cursor, timeout));
	}

	bool result = false;
	int ret = poll(&fds, 1, time_ms);

	if ((ret > 0) && (fds.revents & POLLIN)) {
		result = reset_event(fds.fd);
	}

	helper.return_value(create_boolean(result));
#endif
}
