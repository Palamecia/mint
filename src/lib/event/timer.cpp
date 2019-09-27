#include "memory/functiontool.h"
#include "memory/casttool.h"

#ifdef OS_WINDOWS

#else
#include <sys/timerfd.h>
#endif

using namespace mint;

MINT_FUNCTION(mint_timer_create, 1, cursor) {

	FunctionHelper helper(cursor, 1);

	SharedReference clock_type = helper.popParameter();

#ifdef OS_WINDOWS

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

	int fd = static_cast<int>(to_number(cursor, helper.popParameter()));

#ifdef OS_WINDOWS

#else
	helper.returnValue(create_number(fd));
#endif
}
