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
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/data.h"
#include "mint/memory/memory_tools.h"
#include "mint/memory/reference.h"
#include "mint/memory/function_tools.h"
#include "mint/memory/cast_tools.h"
#include "mint/system/async_io.h"

#include <chrono>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <type_traits>

#ifdef MINT_OS_WINDOWS
#include <Windows.h>
#include <handleapi.h>
#include <minwindef.h>
#include <synchapi.h>
#include <winbase.h>
#include <winnt.h>
#else
#ifdef MINT_OS_LINUX
#include <sys/epoll.h>
#include <sys/time.h>
#else
#include <sys/event.h>
#endif
#ifndef MINT_OS_MAC
#include <sys/timerfd.h>
#endif
#include <unistd.h>
#endif

namespace {

enum class ClockType : std::uint8_t {
	monotonic
};

ClockType to_clock_type(mint::Cursor& cursor, const mint::Reference& value) {
	return static_cast<ClockType>(to_integer<std::underlying_type_t<ClockType>>(cursor, value));
}

struct TimerData {
	mint::poll_event_t event = {};
#ifdef MINT_OS_WINDOWS
	ClockType clock_type = ClockType::monotonic;
	bool periodic = false;
	bool running = false;
#elifdef MINT_OS_MAC
	bool running = false;
#endif
};

#ifdef MINT_OS_WINDOWS
constexpr inline LONGLONG to_milliseconds = 10000LL;
#endif

mint::Reference mint_timer_create(mint::Cursor& cursor, const mint::Reference& clock_type) {

#ifdef MINT_OS_WINDOWS

	auto timer = std::make_unique<TimerData>(TimerData {
	    .event =
	        {
	            .handle = CreateWaitableTimer(nullptr, false, nullptr),
	        },
	    .clock_type = to_clock_type(cursor, clock_type),
	});
	if (timer->event.handle != INVALID_HANDLE_VALUE) {
		return mint::create_c_object(cursor.ast(), timer.release());
	}

#elifdef MINT_OS_LINUX

	int clock_id = CLOCK_MONOTONIC;

	switch (to_clock_type(cursor, clock_type)) {
	case ClockType::monotonic:
		clock_id = CLOCK_MONOTONIC;
		break;
	}

	auto timer = std::make_unique<TimerData>(TimerData {
	    .event =
	        {
	            .fd = timerfd_create(clock_id, TFD_NONBLOCK),
	            .events = EPOLLIN,
	            .on_signal =
	                [](mint::poll_event_t& event) {
		                auto expirations = uint64_t();
		                read(event.fd, &expirations, sizeof(expirations));
	                },
	        },
	});
	if (timer->event.fd != -1) {
		return mint::create_c_object(cursor.ast(), timer.release());
	}

#elifdef MINT_OS_FREE_BSD

	int clock_id = CLOCK_MONOTONIC;

	switch (to_clock_type(cursor, clock_type)) {
	case ClockType::monotonic:
		clock_id = CLOCK_MONOTONIC;
		break;
	}

	auto timer = std::make_unique<TimerData>(TimerData {
	    .event =
	        {
	            .fd = timerfd_create(clock_id, TFD_NONBLOCK),
	            .filter = EVFILT_TIMER,
	        },
	});
	if (timer->event.fd != -1) {
		return mint::create_c_object(cursor.ast(), timer.release());
	}

#elifdef MINT_OS_MAC

	int clock_id = CLOCK_MONOTONIC;

	switch (to_clock_type(cursor, clock_type)) {
	case ClockType::monotonic:
		clock_id = CLOCK_MONOTONIC;
		break;
	}

	auto timer = std::make_unique<TimerData>(TimerData {
	    .event =
	        {
	            .fd = timerfd_create(clock_id, TFD_NONBLOCK),
	            .filter = EVFILT_TIMER,
	        },
	});
	if (timer->event.fd != -1) {
		return mint::create_c_object(cursor.ast(), timer.release());
	}

#endif
	return {};
}

mint::Reference mint_timer_delete(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
#ifdef MINT_OS_WINDOWS
	CloseHandle(d_ptr.data<mint::LibObject<TimerData>>().ptr->event.handle);
#else
	close(d_ptr.data<mint::LibObject<TimerData>>().ptr->event.fd);
#endif
	delete d_ptr.data<mint::LibObject<TimerData>>().ptr;
	return {};
}

mint::Reference mint_timer_get_handle(mint::Cursor& cursor, const mint::Reference& d_ptr) {
#ifdef MINT_OS_WINDOWS
	return mint::create_handle(cursor.ast(), d_ptr.data<mint::LibObject<TimerData>>().ptr->event.handle);
#elifdef MINT_OS_UNIX
	return mint::create_handle(cursor.ast(), d_ptr.data<mint::LibObject<TimerData>>().ptr->event.fd);
#endif
}

mint::Reference mint_timer_get_poll_event(mint::Cursor& cursor, const mint::Reference& d_ptr) {
	return mint::create_c_object(cursor.ast(), &d_ptr.data<mint::LibObject<TimerData>>().ptr->event);
}

mint::Reference mint_timer_start(mint::Cursor& cursor, const mint::Reference& d_ptr, const mint::Reference& delay) {
#ifdef MINT_OS_WINDOWS

	auto* timer = d_ptr.data<mint::LibObject<TimerData>>().ptr;
	const auto timer_spec = LARGE_INTEGER {
	    .QuadPart = -(mint::to_integer<LONGLONG>(cursor, delay) * to_milliseconds),
	};

	if (SetWaitableTimer(timer->event.handle, &timer_spec, 0, nullptr, nullptr, 0)) {
		timer->periodic = false;
		timer->running = true;
		return mint::create_boolean(true);
	}

	return mint::create_boolean(false);

#elifdef MINT_OS_MAC
#else

	const auto fd = d_ptr.data<mint::LibObject<TimerData>>().ptr->event.fd;
	const std::intmax_t msec = to_signed_integer(cursor, delay);

	const auto timer_spec = itimerspec {
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

mint::Reference mint_timer_start_periodic(mint::Cursor& cursor, const mint::Reference& d_ptr,
    const mint::Reference& delay, const mint::Reference& period) {
#ifdef MINT_OS_WINDOWS

	auto* timer = d_ptr.data<mint::LibObject<TimerData>>().ptr;
	const auto timer_spec = LARGE_INTEGER {
	    .QuadPart = -(mint::to_integer<LONGLONG>(cursor, delay) * to_milliseconds),
	};

	const auto period_ms = mint::to_integer<LONG>(cursor, period);

	if (SetWaitableTimer(timer->event.handle, &timer_spec, period_ms, nullptr, nullptr, 0)) {
		timer->periodic = true;
		timer->running = true;
		return mint::create_boolean(true);
	}

	return mint::create_boolean(false);

#elifdef MINT_OS_MAC
#else
	const auto fd = d_ptr.data<mint::LibObject<TimerData>>().ptr->event.fd;
	const std::intmax_t delay_ms = to_signed_integer(cursor, delay);
	const std::intmax_t period_ms = to_signed_integer(cursor, period);

	auto timer_spec = itimerspec {
	    .it_interval =
	        {
	            .tv_sec = period_ms / 1000,
	            .tv_nsec = (period_ms % 1000) * 1000000,
	        },
	    .it_value =
	        {
	            .tv_sec = delay_ms / 1000,
	            .tv_nsec = (delay_ms % 1000) * 1000000,
	        },
	};

	return mint::create_boolean(timerfd_settime(fd, 0, &timer_spec, nullptr) == 0);

#endif
}

mint::Reference mint_timer_stop(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
#ifdef MINT_OS_WINDOWS
	mint::handle_t handle = d_ptr.data<mint::LibObject<TimerData>>().ptr->event.handle;
	if (CancelWaitableTimer(handle)) {
		d_ptr.data<mint::LibObject<TimerData>>().ptr->running = false;
	}
#else
	mint::handle_t fd = d_ptr.data<mint::LibObject<TimerData>>().ptr->event.fd;

	itimerspec timer_spec;
	memset(&timer_spec, 0, sizeof(timer_spec));

	return mint::create_boolean(timerfd_settime(fd, 0, &timer_spec, nullptr) == 0);
#endif
	return {};
}

mint::Reference mint_timer_is_running(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
#ifdef MINT_OS_WINDOWS

	auto* timer = d_ptr.data<mint::LibObject<TimerData>>().ptr;
	if (timer->running && !timer->periodic) {
		if (WaitForSingleObject(timer->event.handle, 0) == WAIT_OBJECT_0) {
			timer->running = false;
		}
	}
	return mint::create_boolean(timer->running);

#elifdef MINT_OS_UNIX

	auto fd = d_ptr.data<mint::LibObject<TimerData>>().ptr->event.fd;
	auto timer_spec = itimerspec();
	if (timerfd_gettime(fd, &timer_spec) == -1) {
		return mint::create_boolean(false);
	}
	if (timer_spec.it_value.tv_sec == 0 && timer_spec.it_value.tv_nsec == 0) {
		return mint::create_boolean(false);
	}
	return mint::create_boolean(true);

#endif
}

mint::Reference mint_timer_clear(mint::Cursor& /*cursor*/, const mint::Reference& d_ptr) {
#ifdef MINT_OS_WINDOWS
	ResetEvent(d_ptr.data<mint::LibObject<TimerData>>().ptr->event.handle);
#else
	mint::handle_t fd = d_ptr.data<mint::LibObject<TimerData>>().ptr->event.fd;
	std::uint64_t value = 0;

	read(fd, &value, sizeof(value));
#endif
	return {};
}

mint::Reference mint_timer_wait(mint::Cursor& cursor, const mint::Reference& d_ptr, const mint::Reference& timeout) {

#ifdef MINT_OS_WINDOWS

	mint::handle_t handle = d_ptr.data<mint::LibObject<TimerData>>().ptr->event.handle;
	const DWORD time_ms = mint::is_instance_of(timeout, mint::Data::Format::none)
	                          ? INFINITE
	                          : mint::to_integer<DWORD>(cursor, timeout);

	if (WaitForSingleObject(handle, time_ms) == WAIT_OBJECT_0) {
		ResetEvent(handle);
		return mint::create_boolean(true);
	}

	return mint::create_boolean(false);

#elifdef MINT_OS_LINUX

	auto* timer = d_ptr.data<mint::LibObject<TimerData>>().ptr;
	const int timeout_ms = is_instance_of(timeout, mint::Data::Format::none) ? -1 : to_integer<int>(cursor, timeout);

	int context = epoll_create1(EPOLL_CLOEXEC);
	if (context == -1) {
		return mint::create_boolean(false);
	}

	auto changelist = epoll_event {
	    .events = timer->event.events,
	    .data =
	        {
	            .fd = timer->event.fd,
	        },
	};

	const auto ret = epoll_wait(context, &changelist, 1, timeout_ms);
	if (ret > 0 && changelist.events & EPOLLIN) {
		timer->event.on_signal(timer->event);
	}

	::close(context);
	return mint::create_boolean(ret > 0);

#elifdef MINT_OS_UNIX

	auto* timer = d_ptr.data<mint::LibObject<TimerData>>().ptr;
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
	    .ident = static_cast<std::uintptr_t>(timer->event.fd),
	    .filter = EVFILT_READ,
	};

	struct kevent eventlist {};
	const auto ret = kevent(context, &changelist, 1, &eventlist, 1, timeout_ts ? std::to_address(timeout_ts) : nullptr);

	::close(context);
	return mint::create_boolean(ret > 0);

#endif
}

}

MINT_EXPORT_FUNCTION(mint_timer_create, 1)
MINT_EXPORT_FUNCTION(mint_timer_delete, 1)
MINT_EXPORT_FUNCTION(mint_timer_get_handle, 1)
MINT_EXPORT_FUNCTION(mint_timer_get_poll_event, 1)
MINT_EXPORT_FUNCTION(mint_timer_start, 2)
MINT_EXPORT_FUNCTION(mint_timer_start_periodic, 3)
MINT_EXPORT_FUNCTION(mint_timer_stop, 1)
MINT_EXPORT_FUNCTION(mint_timer_is_running, 1)
MINT_EXPORT_FUNCTION(mint_timer_clear, 1)
MINT_EXPORT_FUNCTION(mint_timer_wait, 2)
