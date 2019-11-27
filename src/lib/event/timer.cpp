#include "memory/functiontool.h"
#include "memory/casttool.h"

#ifdef OS_WINDOWS
#include <Windows.h>
using handle_data_t = std::remove_pointer<HANDLE>::type;
#else
#include <sys/timerfd.h>
#endif

using namespace mint;
using namespace std;

MINT_FUNCTION(mint_timer_create, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	SharedReference clock_type = move(helper.popParameter());

#ifdef OS_WINDOWS
	HANDLE handle = CreateWaitableTimer(nullptr, true, nullptr);

	/// \todo setup clock

	if (handle != INVALID_HANDLE_VALUE) {
		helper.returnValue(create_object(handle));
	}
#else
	int clock_id = CLOCK_MONOTONIC;

	switch (static_cast<int>(to_number(cursor, clock_type))) {
	case 0:
		clock_id = CLOCK_MONOTONIC;
		break;

	default:
		break;
	}

	int fd = timerfd_create(clock_id, TFD_NONBLOCK);
	if (fd != -1) {
		helper.returnValue(create_number(fd));
	}
#endif
}

MINT_FUNCTION(mint_timer_handle, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	helper.returnValue(move(helper.popParameter()));
#else
	helper.returnValue(create_number(static_cast<int>(to_number(cursor, helper.popParameter()))));
#endif
}
