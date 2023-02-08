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
#include <mint/memory/memorytool.h>
#include <mint/memory/casttool.h>

#ifdef OS_WINDOWS
#include <Windows.h>
#else
#include <poll.h>
#endif

using namespace std;
using namespace mint;

MINT_FUNCTION(mint_watcher_poll, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	WeakReference timeout = std::move(helper.pop_parameter());
	Array::values_type event_set = to_array(helper.pop_parameter());

#ifdef OS_WINDOWS
	vector<HANDLE> fdset;

	for (Array::values_type::value_type &item : event_set) {
		fdset.push_back(to_handle(get_object_member(cursor, item, "handle")));
	}

	DWORD time_ms = INFINITE;

	if (timeout.data()->format != Data::fmt_none) {
		time_ms = static_cast<int>(to_integer(cursor, timeout));
	}

	DWORD status = WaitForMultipleObjectsEx(fdset.size(), fdset.data(), false, time_ms, true);

	while (status == WAIT_IO_COMPLETION) {
		status = WaitForMultipleObjectsEx(fdset.size(), fdset.data(), false, 0, true);
	}

	for (size_t i = status - WAIT_OBJECT_0 + 1; i < fdset.size(); ++i) {
		get_object_member(cursor, event_set.at(i), "activated").data<Boolean>()->value = (WaitForSingleObjectEx(fdset[i], 0, true) == WAIT_OBJECT_0);
	}
#else
	vector<pollfd> fdset;

	for (Array::values_type::value_type &item : event_set) {
		pollfd fd;
		fd.fd = to_handle(get_object_member(cursor, item, "handle"));
		fd.events = POLLIN;
		fdset.push_back(fd);
	}

	int time_ms = -1;

	if (timeout.data()->format != Data::fmt_none) {
		time_ms = static_cast<int>(to_integer(cursor, timeout));
	}

	poll(fdset.data(), fdset.size(), time_ms);

	for (size_t i = 0; i < fdset.size(); ++i) {
		get_object_member(cursor, event_set.at(i), "activated").data<Boolean>()->value = fdset.at(i).revents & POLLIN;
	}
#endif
}
