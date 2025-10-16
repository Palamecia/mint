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

#include "mint/memory/memorytool.h"
#include "mint/memory/reference.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/casttool.h"

#ifdef MINT_OS_WINDOWS
#include <Windows.h>
#else
#include <sys/eventfd.h>
#include <poll.h>
#include <unistd.h>
#endif

namespace {

mint::WeakReference mint_event_create(mint::Cursor& cursor) {
#ifdef MINT_OS_WINDOWS
	if (HANDLE handle = CreateEvent(nullptr, TRUE, FALSE, nullptr); handle != INVALID_HANDLE_VALUE) {
		return mint::create_handle(cursor.ast(), handle);
	}
#else
	if (int fd = eventfd(0, EFD_NONBLOCK); fd != -1) {
		return mint::create_number(fd);
	}
#endif
	return {};
}

mint::WeakReference mint_event_close(mint::Cursor& /*cursor*/, const mint::Reference& handle) {
#ifdef MINT_OS_WINDOWS
	CloseHandle(mint::to_handle(handle));
#else
	close(mint::to_handle(handle));
#endif
	return {};
}

mint::WeakReference mint_event_is_set(mint::Cursor& /*cursor*/, const mint::Reference& handle) {
#ifdef MINT_OS_WINDOWS
	return mint::create_boolean(WaitForSingleObject(mint::to_handle(handle), 0) == WAIT_OBJECT_0);
#else
	uint64_t value = 0;
	int fd = mint::to_handle(handle);
	read(fd, &value, sizeof(value));
	write(fd, &value, sizeof(value));
	return mint::create_boolean(value);
#endif
}

mint::WeakReference mint_event_set(mint::Cursor& /*cursor*/, const mint::Reference& handle) {
#ifdef MINT_OS_WINDOWS
	return mint::create_boolean(SetEvent(to_handle(handle)));
#else
	uint64_t value = 1;
	int fd = mint::to_handle(handle);
	return mint::create_boolean(write(fd, &value, sizeof(value)) == sizeof(value));
#endif
}

mint::WeakReference mint_event_clear(mint::Cursor& /*cursor*/, const mint::Reference& handle) {
#ifdef MINT_OS_WINDOWS
	ResetEvent(mint::to_handle(handle));
#else
	uint64_t value = 0;
	int fd = mint::to_handle(handle);
	read(fd, &value, sizeof(value));
#endif
	return {};
}

mint::WeakReference mint_event_wait(mint::Cursor& cursor, const mint::Reference& handle,
    const mint::Reference& timeout) {
#ifdef MINT_OS_WINDOWS

	const DWORD time_ms = mint::is_instance_of(timeout, mint::Data::none_format)
	                          ? INFINITE
	                          : mint::to_integer<DWORD>(cursor, timeout);

	if (WaitForSingleObject(mint::to_handle(handle), time_ms) == WAIT_OBJECT_0) {
		ResetEvent(mint::to_handle(handle));
		return mint::create_boolean(true);
	}

	return mint::create_boolean(false);
#else
	pollfd fds {
	    .fd = mint::to_handle(handle),
	    .events = POLLIN,
	};

	const int time_ms = is_instance_of(timeout, mint::Data::none_format) ? -1 : to_integer<int>(cursor, timeout);

	if (int ret = poll(&fds, 1, time_ms); (ret > 0) && (fds.revents & POLLIN)) {
		uint64_t value = 0;
		read(fds.fd, &value, sizeof(value));
		return mint::create_boolean(value != 0);
	}

	return mint::create_boolean(false);
#endif
}

}

MINT_EXPORT_FUNCTION(mint_event_create, 0);
MINT_EXPORT_FUNCTION(mint_event_close, 1);
MINT_EXPORT_FUNCTION(mint_event_is_set, 1);
MINT_EXPORT_FUNCTION(mint_event_set, 1);
MINT_EXPORT_FUNCTION(mint_event_clear, 1);
MINT_EXPORT_FUNCTION(mint_event_wait, 2);
