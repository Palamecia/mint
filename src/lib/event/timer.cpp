#include "memory/functiontool.h"
#include "memory/casttool.h"

#ifdef OS_WINDOWS
#include <Windows.h>
using handle_data_t = std::remove_pointer<HANDLE>::type;

struct TimerData {
	bool running;
};

static std::map<HANDLE, TimerData> g_timers;

VOID CALLBACK fnCompletionRoutine(LPVOID lpArgToCompletionRoutine, DWORD dwTimerLowValue, DWORD dwTimerHighValue) {
	HANDLE handle = static_cast<HANDLE>(lpArgToCompletionRoutine);
	g_timers.at(handle).running = false;
}

#else
#include <sys/timerfd.h>
#include <poll.h>
#include <unistd.h>
#endif

using namespace mint;
using namespace std;

enum ClockType { monotonic };

MINT_FUNCTION(mint_timer_create, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	SharedReference clock_type = move(helper.popParameter());

#ifdef OS_WINDOWS

	switch (static_cast<ClockType>(static_cast<intmax_t>(to_number(cursor, clock_type)))) {
	case monotonic:
		/// @todo setup clock type
		break;
	}

	HANDLE handle = CreateWaitableTimer(nullptr, true, nullptr);

	if (handle != INVALID_HANDLE_VALUE) {
		g_timers.emplace(handle, TimerData({false}));
		helper.returnValue(create_object(handle));
	}
#else
	int clock_id = CLOCK_MONOTONIC;

	switch (static_cast<ClockType>(to_number(cursor, clock_type))) {
	case monotonic:
		clock_id = CLOCK_MONOTONIC;
		break;
	}

	int fd = timerfd_create(clock_id, TFD_NONBLOCK);
	if (fd != -1) {
		helper.returnValue(create_number(fd));
	}
#endif
}

MINT_FUNCTION(mint_timer_close, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	HANDLE handle = helper.popParameter()->data<LibObject<handle_data_t>>()->impl;
	CloseHandle(handle);
	g_timers.erase(handle);
#else
	int fd = static_cast<int>(to_number(cursor, helper.popParameter()));
	close(fd);
#endif
}

MINT_FUNCTION(mint_timer_start, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	SharedReference duration = move(helper.popParameter());

#ifdef OS_WINDOWS
	HANDLE handle = helper.popParameter()->data<LibObject<handle_data_t>>()->impl;
	intmax_t msec = static_cast<intmax_t>(to_number(cursor, duration));

	LARGE_INTEGER liDueTime;
	liDueTime.QuadPart = (-msec) * 10000LL;

	if (SetWaitableTimer(handle, &liDueTime, 0, &fnCompletionRoutine, handle, 0)) {
		g_timers.at(handle).running = true;
		helper.returnValue(create_boolean(true));
	}
	else {
		helper.returnValue(create_boolean(false));
	}
#else
	int fd = static_cast<int>(to_number(cursor, helper.popParameter()));
	intmax_t msec = static_cast<intmax_t>(to_number(cursor, duration));

	itimerspec timer_spec;
	timer_spec.it_interval.tv_sec = 0;
	timer_spec.it_interval.tv_nsec = 0;
	timer_spec.it_value.tv_sec = msec / 1000;
	timer_spec.it_value.tv_nsec = (msec % 1000) * 1000000;

	helper.returnValue(create_boolean(timerfd_settime(fd, 0, &timer_spec, nullptr) == 0));
#endif
}

MINT_FUNCTION(mint_timer_stop, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	HANDLE handle = helper.popParameter()->data<LibObject<handle_data_t>>()->impl;

	if (CancelWaitableTimer(handle)) {
		g_timers.at(handle).running = false;
	}
#else
	int fd = static_cast<int>(to_number(cursor, helper.popParameter()));

	itimerspec timer_spec;
	memset(&timer_spec, 0, sizeof(timer_spec));

	helper.returnValue(create_boolean(timerfd_settime(fd, 0, &timer_spec, nullptr) == 0));
#endif
}

MINT_FUNCTION(mint_timer_is_running, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	HANDLE handle = helper.popParameter()->data<LibObject<handle_data_t>>()->impl;
	TimerData &data = g_timers.at(handle);

	if (data.running) {
		if (WaitForSingleObject(handle, 0) == WAIT_OBJECT_0) {
			data.running = false;
		}
	}

	helper.returnValue(create_boolean(data.running));
#else
	int fd = static_cast<int>(to_number(cursor, helper.popParameter()));
	itimerspec timer_spec;

	timerfd_gettime(fd, &timer_spec);

	if (timer_spec.it_value.tv_sec == 0 &&  timer_spec.it_value.tv_nsec == 0) {
		helper.returnValue(create_boolean(false));
	}
	else if (timer_spec.it_interval.tv_sec != 0 &&  timer_spec.it_interval.tv_nsec != 0) {
		helper.returnValue(create_boolean(true));
	}
	else {

		pollfd fds;
		fds.events = POLLIN;
		fds.fd = fd;
		int ret = poll(&fds, 1, 0);

		if ((ret > 0) && (fds.revents & POLLIN)) {
			memset(&timer_spec, 0, sizeof(timer_spec));
			timerfd_settime(fd, 0, &timer_spec, nullptr);
			helper.returnValue(create_boolean(false));
		}
		else {
			helper.returnValue(create_boolean(true));
		}
	}
#endif
}

MINT_FUNCTION(mint_timer_clear, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	ResetEvent(helper.popParameter()->data<LibObject<handle_data_t>>()->impl);
#else
	int fd = static_cast<int>(to_number(cursor, helper.popParameter()));

	uint64_t value = 0;
	read(fd, &value, sizeof (value));
#endif
}

MINT_FUNCTION(mint_timer_wait, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	SharedReference timeout = move(helper.popParameter());

#ifdef OS_WINDOWS

	DWORD time_ms = INFINITE;
	HANDLE handle = helper.popParameter()->data<LibObject<handle_data_t>>()->impl;

	if (timeout->data()->format != Data::fmt_none) {
		time_ms = static_cast<int>(to_number(cursor, timeout));
	}

	bool result = false;

	if (WaitForSingleObject(handle, time_ms) == WAIT_OBJECT_0) {
		ResetEvent(handle);
		result = true;
	}

	helper.returnValue(create_boolean(result));
#else
	pollfd fds;
	fds.events = POLLIN;
	fds.fd = static_cast<int>(to_number(cursor, helper.popParameter()));

	int time_ms = -1;

	if (timeout->data()->format != Data::fmt_none) {
		time_ms = static_cast<int>(to_number(cursor, timeout));
	}

	bool result = false;
	int ret = poll(&fds, 1, time_ms);

	if ((ret > 0) && (fds.revents & POLLIN)) {
		uint64_t value = 0;
		read(fds.fd, &value, sizeof (value));
		result = value != 0;
	}

	helper.returnValue(create_boolean(result));
#endif
}
