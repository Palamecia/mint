#include <memory/functiontool.h>
#include <memory/casttool.h>

#ifdef OS_WINDOWS
#include <Windows.h>
#else
#include <sys/eventfd.h>
#include <poll.h>
#endif

#include <unistd.h>

using namespace mint;

MINT_FUNCTION(mint_event_create, 0, cursor) {

	FunctionHelper helper(cursor, 0);

#ifdef OS_WINDOWS
	helper.returnValue(create_object(CreateEvent()));
#else
	int fd = eventfd(0, EFD_NONBLOCK);
	if (fd != -1) {
		helper.returnValue(create_number(fd));
	}
#endif
}

MINT_FUNCTION(mint_event_close, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS

#else
	int fd = static_cast<int>(to_number(cursor, helper.popParameter()));
	close(fd);
#endif
}

MINT_FUNCTION(mint_event_is_set, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS

#else
	int fd = static_cast<int>(to_number(cursor, helper.popParameter()));

	uint64_t value = 0;
	read(fd, &value, sizeof (value));
	write(fd, &value, sizeof (value));
	helper.returnValue(create_boolean(value));
#endif
}

MINT_FUNCTION(mint_event_set, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS

#else
	int fd = static_cast<int>(to_number(cursor, helper.popParameter()));

	uint64_t value = 1;
	helper.returnValue(create_boolean(write(fd, &value, sizeof (value)) == sizeof (value)));
#endif
}

MINT_FUNCTION(mint_event_clear, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS

#else
	int fd = static_cast<int>(to_number(cursor, helper.popParameter()));

	uint64_t value = 0;
	read(fd, &value, sizeof (value));
#endif
}

MINT_FUNCTION(mint_event_wait, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	SharedReference timeout = helper.popParameter();

#ifdef OS_WINDOWS

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
