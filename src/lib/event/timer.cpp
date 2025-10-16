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
#include "mint/memory/reference.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/casttool.h"

#include <cstdint>
#include <type_traits>

#ifdef MINT_OS_WINDOWS
#include <Windows.h>
#else
#include <sys/timerfd.h>
#include <poll.h>
#include <unistd.h>
#endif

namespace {

#ifdef MINT_OS_WINDOWS
constexpr inline LONGLONG to_milliseconds = 10000LL;

struct TimerData {
	bool running = false;
};

std::map<HANDLE, TimerData> g_timers;

VOID CALLBACK fn_completion_routine(LPVOID arg, DWORD /*timer_low_value*/, DWORD /*timer_high_value*/) {
	auto* const handle = static_cast<HANDLE>(arg);
	g_timers.at(handle).running = false;
}
#endif

enum ClockType : std::uint8_t {
	monotonic
};

ClockType to_clock_type(mint::Cursor& cursor, const mint::Reference& value) {
	return static_cast<ClockType>(to_integer<std::underlying_type_t<ClockType>>(cursor, value));
}

mint::WeakReference mint_timer_create(mint::Cursor& cursor, const mint::Reference& clock_type) {

#ifdef MINT_OS_WINDOWS

	switch (to_clock_type(cursor, clock_type)) {
	case monotonic:
		/// @todo setup clock type
		break;
	}

	HANDLE handle = CreateWaitableTimer(nullptr, true, nullptr);
	if (handle != INVALID_HANDLE_VALUE) {
		g_timers.emplace(handle, TimerData({false}));
		return mint::create_handle(cursor.ast(), handle);
	}
#else
	int clock_id = CLOCK_MONOTONIC;

	switch (to_clock_type(cursor, clock_type)) {
	case monotonic:
		clock_id = CLOCK_MONOTONIC;
		break;
	}

	if (int fd = timerfd_create(clock_id, TFD_NONBLOCK); fd != -1) {
		return mint::create_handle(cursor.ast(), fd);
	}
#endif
	return {};
}

mint::WeakReference mint_timer_close(mint::Cursor& /*cursor*/, const mint::Reference& handle) {
#ifdef MINT_OS_WINDOWS
	CloseHandle(mint::to_handle(handle));
	g_timers.erase(mint::to_handle(handle));
#else
	close(mint::to_handle(handle));
#endif
	return {};
}

mint::WeakReference mint_timer_start(mint::Cursor& cursor, const mint::Reference& handle,
    const mint::Reference& duration) {
#ifdef MINT_OS_WINDOWS

	LARGE_INTEGER timer_spec {
	    .QuadPart = LONGLONG(-mint::to_signed_integer(cursor, duration)) * to_milliseconds,
	};

	if (SetWaitableTimer(mint::to_handle(handle), &timer_spec, 0, &fn_completion_routine, mint::to_handle(handle), 0)) {
		g_timers.at(mint::to_handle(handle)).running = true;
		return mint::create_boolean(true);
	}

	return mint::create_boolean(false);
#else
	mint::handle_t fd = to_handle(handle);
	const std::intmax_t msec = to_signed_integer(cursor, duration);

	itimerspec timer_spec {
	    .it_interval =
	        {
	            .tv_sec = 0,
	            .tv_nsec = 0,
	        },
	    .it_value =
	        {
	            .tv_sec = msec / 1000,
	            .tv_nsec = (msec % 1000) * 1000000,
	        },
	};

	return mint::create_boolean(timerfd_settime(fd, 0, &timer_spec, nullptr) == 0);
#endif
}

mint::WeakReference mint_timer_stop(mint::Cursor& /*cursor*/, const mint::Reference& handle) {
#ifdef MINT_OS_WINDOWS
	if (CancelWaitableTimer(mint::to_handle(handle))) {
		g_timers.at(mint::to_handle(handle)).running = false;
	}
#else
	mint::handle_t fd = mint::to_handle(handle);

	itimerspec timer_spec;
	memset(&timer_spec, 0, sizeof(timer_spec));

	return mint::create_boolean(timerfd_settime(fd, 0, &timer_spec, nullptr) == 0);
#endif
	return {};
}

mint::WeakReference mint_timer_is_running(mint::Cursor& /*cursor*/, const mint::Reference& handle) {
#ifdef MINT_OS_WINDOWS

	TimerData& data = g_timers.at(mint::to_handle(handle));

	if (data.running) {
		if (WaitForSingleObject(mint::to_handle(handle), 0) == WAIT_OBJECT_0) {
			data.running = false;
		}
	}

	return mint::create_boolean(data.running);
#else
	mint::handle_t fd = mint::to_handle(handle);
	itimerspec timer_spec;

	timerfd_gettime(fd, &timer_spec);

	if (timer_spec.it_value.tv_sec == 0 && timer_spec.it_value.tv_nsec == 0) {
		return mint::create_boolean(false);
	}
	if (timer_spec.it_interval.tv_sec != 0 && timer_spec.it_interval.tv_nsec != 0) {
		return mint::create_boolean(true);
	}

	pollfd fds {
	    .fd = fd,
	    .events = POLLIN,
	};

	if (int ret = poll(&fds, 1, 0); (ret > 0) && (fds.revents & POLLIN)) {
		memset(&timer_spec, 0, sizeof(timer_spec));
		timerfd_settime(fd, 0, &timer_spec, nullptr);
		return mint::create_boolean(false);
	}

	return mint::create_boolean(true);
#endif
}

mint::WeakReference mint_timer_clear(mint::Cursor& /*cursor*/, const mint::Reference& handle) {
#ifdef MINT_OS_WINDOWS
	ResetEvent(to_handle(handle));
#else
	mint::handle_t fd = to_handle(handle);
	std::uint64_t value = 0;

	read(fd, &value, sizeof(value));
#endif
	return {};
}

mint::WeakReference mint_timer_wait(mint::Cursor& cursor, const mint::Reference& handle,
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
	    .fd = to_handle(handle),
	    .events = POLLIN,
	};

	const int time_ms = is_instance_of(timeout, mint::Data::none_format) ? -1 : to_integer<int>(cursor, timeout);

	if (int ret = poll(&fds, 1, time_ms); (ret > 0) && (fds.revents & POLLIN)) {
		std::uint64_t value = 0;
		read(fds.fd, &value, sizeof(value));
		return mint::create_boolean(value != 0);
	}

	return mint::create_boolean(false);
#endif
}

}

MINT_EXPORT_FUNCTION(mint_timer_create, 1)
MINT_EXPORT_FUNCTION(mint_timer_close, 1)
MINT_EXPORT_FUNCTION(mint_timer_start, 2)
MINT_EXPORT_FUNCTION(mint_timer_stop, 1)
MINT_EXPORT_FUNCTION(mint_timer_is_running, 1)
MINT_EXPORT_FUNCTION(mint_timer_clear, 1)
MINT_EXPORT_FUNCTION(mint_timer_wait, 2)
