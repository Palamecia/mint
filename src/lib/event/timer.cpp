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

#include "mint/memory/functiontool.h"
#include "mint/memory/casttool.h"

#include <cstdint>

#ifdef OS_WINDOWS
#include <Windows.h>

struct TimerData {
	bool running;
};

static std::map<HANDLE, TimerData> g_timers;

VOID CALLBACK fnCompletionRoutine(LPVOID lpArgToCompletionRoutine, DWORD dwTimerLowValue, DWORD dwTimerHighValue) {
	auto *const handle = static_cast<HANDLE>(lpArgToCompletionRoutine);
	g_timers.at(handle).running = false;
}

#else
#include <sys/timerfd.h>
#include <poll.h>
#include <unistd.h>
#endif

using namespace mint;

enum ClockType : std::uint8_t {
	MONOTONIC
};

MINT_FUNCTION(mint_timer_create, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	WeakReference clock_type = std::move(helper.pop_parameter());

#ifdef OS_WINDOWS

	switch (static_cast<ClockType>(to_integer(cursor, clock_type))) {
	case MONOTONIC:
		/// @todo setup clock type
		break;
	}

	HANDLE handle = CreateWaitableTimer(nullptr, true, nullptr);

	if (handle != INVALID_HANDLE_VALUE) {
		g_timers.emplace(handle, TimerData({false}));
		helper.return_value(create_handle(handle));
	}
#else
	int clock_id = CLOCK_MONOTONIC;

	switch (static_cast<ClockType>(to_number(cursor, clock_type))) {
	case MONOTONIC:
		clock_id = CLOCK_MONOTONIC;
		break;
	}

	int fd = timerfd_create(clock_id, TFD_NONBLOCK);
	if (fd != -1) {
		helper.return_value(create_handle(fd));
	}
#endif
}

MINT_FUNCTION(mint_timer_close, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	mint::handle_t handle = to_handle(helper.pop_parameter());

#ifdef OS_WINDOWS
	CloseHandle(handle);
	g_timers.erase(handle);
#else
	close(handle);
#endif
}

MINT_FUNCTION(mint_timer_start, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	WeakReference duration = std::move(helper.pop_parameter());

#ifdef OS_WINDOWS
	mint::handle_t handle = to_handle(helper.pop_parameter());
	intmax_t msec = to_integer(cursor, duration);

	LARGE_INTEGER liDueTime;
	liDueTime.QuadPart = (-msec) * 10000LL;

	if (SetWaitableTimer(handle, &liDueTime, 0, &fnCompletionRoutine, handle, 0)) {
		g_timers.at(handle).running = true;
		helper.return_value(create_boolean(true));
	}
	else {
		helper.return_value(create_boolean(false));
	}
#else
	mint::handle_t fd = to_handle(helper.pop_parameter());
	intmax_t msec = to_integer(cursor, duration);

	itimerspec timer_spec;
	timer_spec.it_interval.tv_sec = 0;
	timer_spec.it_interval.tv_nsec = 0;
	timer_spec.it_value.tv_sec = msec / 1000;
	timer_spec.it_value.tv_nsec = (msec % 1000) * 1000000;

	helper.return_value(create_boolean(timerfd_settime(fd, 0, &timer_spec, nullptr) == 0));
#endif
}

MINT_FUNCTION(mint_timer_stop, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	mint::handle_t handle = to_handle(helper.pop_parameter());

	if (CancelWaitableTimer(handle)) {
		g_timers.at(handle).running = false;
	}
#else
	mint::handle_t fd = to_handle(helper.pop_parameter());

	itimerspec timer_spec;
	memset(&timer_spec, 0, sizeof(timer_spec));

	helper.return_value(create_boolean(timerfd_settime(fd, 0, &timer_spec, nullptr) == 0));
#endif
}

MINT_FUNCTION(mint_timer_is_running, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	mint::handle_t handle = to_handle(helper.pop_parameter());
	TimerData &data = g_timers.at(handle);

	if (data.running) {
		if (WaitForSingleObject(handle, 0) == WAIT_OBJECT_0) {
			data.running = false;
		}
	}

	helper.return_value(create_boolean(data.running));
#else
	mint::handle_t fd = to_handle(helper.pop_parameter());
	itimerspec timer_spec;

	timerfd_gettime(fd, &timer_spec);

	if (timer_spec.it_value.tv_sec == 0 && timer_spec.it_value.tv_nsec == 0) {
		helper.return_value(create_boolean(false));
	}
	else if (timer_spec.it_interval.tv_sec != 0 && timer_spec.it_interval.tv_nsec != 0) {
		helper.return_value(create_boolean(true));
	}
	else {

		pollfd fds;
		fds.events = POLLIN;
		fds.fd = fd;
		int ret = poll(&fds, 1, 0);

		if ((ret > 0) && (fds.revents & POLLIN)) {
			memset(&timer_spec, 0, sizeof(timer_spec));
			timerfd_settime(fd, 0, &timer_spec, nullptr);
			helper.return_value(create_boolean(false));
		}
		else {
			helper.return_value(create_boolean(true));
		}
	}
#endif
}

MINT_FUNCTION(mint_timer_clear, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	mint::handle_t handle = to_handle(helper.pop_parameter());
	ResetEvent(handle);
#else
	mint::handle_t fd = to_handle(helper.pop_parameter());

	uint64_t value = 0;
	read(fd, &value, sizeof(value));
#endif
}

MINT_FUNCTION(mint_timer_wait, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	WeakReference timeout = std::move(helper.pop_parameter());

#ifdef OS_WINDOWS

	DWORD time_ms = INFINITE;
	HANDLE handle = to_handle(helper.pop_parameter());

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
	fds.fd = to_handle(helper.pop_parameter());

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
