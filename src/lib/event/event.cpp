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

#ifdef OS_WINDOWS
#include <Windows.h>
using handle_data_t = std::remove_pointer_t<HANDLE>;
#else
#include <sys/eventfd.h>
#include <poll.h>
#include <unistd.h>
#endif

using namespace mint;

MINT_FUNCTION(mint_event_create, 0, cursor) {

	FunctionHelper helper(cursor, 0);

#ifdef OS_WINDOWS
	helper.return_value(create_object(CreateEvent(nullptr, TRUE, FALSE, nullptr)));
#else
	int fd = eventfd(0, EFD_NONBLOCK);
	if (fd != -1) {
		helper.return_value(create_number(fd));
	}
#endif
}

MINT_FUNCTION(mint_event_close, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	CloseHandle(helper.pop_parameter().data<LibObject<handle_data_t>>()->impl);
#else
	close(static_cast<int>(to_integer(cursor, helper.pop_parameter())));
#endif
}

MINT_FUNCTION(mint_event_is_set, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	HANDLE handle = helper.pop_parameter().data<LibObject<handle_data_t>>()->impl;
	helper.return_value(create_boolean(WaitForSingleObject(handle, 0) == WAIT_OBJECT_0));
#else
	int fd = static_cast<int>(to_integer(cursor, helper.pop_parameter()));

	uint64_t value = 0;
	read(fd, &value, sizeof(value));
	write(fd, &value, sizeof(value));
	helper.return_value(create_boolean(value));
#endif
}

MINT_FUNCTION(mint_event_set, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	SetEvent(helper.pop_parameter().data<LibObject<handle_data_t>>()->impl);
#else
	int fd = static_cast<int>(to_integer(cursor, helper.pop_parameter()));

	uint64_t value = 1;
	helper.return_value(create_boolean(write(fd, &value, sizeof(value)) == sizeof(value)));
#endif
}

MINT_FUNCTION(mint_event_clear, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	ResetEvent(helper.pop_parameter().data<LibObject<handle_data_t>>()->impl);
#else
	int fd = static_cast<int>(to_integer(cursor, helper.pop_parameter()));

	uint64_t value = 0;
	read(fd, &value, sizeof(value));
#endif
}

MINT_FUNCTION(mint_event_wait, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	WeakReference timeout = std::move(helper.pop_parameter());

#ifdef OS_WINDOWS

	DWORD time_ms = INFINITE;
	HANDLE handle = helper.pop_parameter().data<LibObject<handle_data_t>>()->impl;

	if (timeout.data()->format != Data::FMT_NONE) {
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
	fds.fd = static_cast<int>(to_integer(cursor, helper.pop_parameter()));

	int time_ms = -1;

	if (timeout.data()->format != Data::FMT_NONE) {
		time_ms = static_cast<int>(to_integer(cursor, timeout));
	}

	bool result = false;
	int ret = poll(&fds, 1, time_ms);

	if ((ret > 0) && (fds.revents & POLLIN)) {
		uint64_t value = 0;
		read(fds.fd, &value, sizeof(value));
		result = value != 0;
	}

	helper.return_value(create_boolean(result));
#endif
}
