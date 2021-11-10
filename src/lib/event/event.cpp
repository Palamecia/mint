#include <memory/functiontool.h>
#include <memory/casttool.h>

#ifdef OS_WINDOWS
#include <Windows.h>
using handle_data_t = std::remove_pointer<HANDLE>::type;
#else
#include <sys/eventfd.h>
#include <poll.h>
#include <unistd.h>
#endif

using namespace mint;
using namespace std;

MINT_FUNCTION(mint_event_create, 0, cursor) {

	FunctionHelper helper(cursor, 0);

#ifdef OS_WINDOWS
	helper.returnValue(create_object(CreateEvent(nullptr, TRUE, FALSE, nullptr)));
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
	CloseHandle(helper.popParameter().data<LibObject<handle_data_t>>()->impl);
#else
	close(static_cast<int>(to_integer(cursor, helper.popParameter())));
#endif
}

MINT_FUNCTION(mint_event_is_set, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	HANDLE handle = helper.popParameter().data<LibObject<handle_data_t>>()->impl;
	helper.returnValue(create_boolean(WaitForSingleObject(handle, 0) == WAIT_OBJECT_0));
#else
	int fd = static_cast<int>(to_integer(cursor, helper.popParameter()));

	uint64_t value = 0;
	read(fd, &value, sizeof (value));
	write(fd, &value, sizeof (value));
	helper.returnValue(create_boolean(value));
#endif
}

MINT_FUNCTION(mint_event_set, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	SetEvent(helper.popParameter().data<LibObject<handle_data_t>>()->impl);
#else
	int fd = static_cast<int>(to_integer(cursor, helper.popParameter()));

	uint64_t value = 1;
	helper.returnValue(create_boolean(write(fd, &value, sizeof (value)) == sizeof (value)));
#endif
}

MINT_FUNCTION(mint_event_clear, 1, cursor) {

	FunctionHelper helper(cursor, 1);

#ifdef OS_WINDOWS
	ResetEvent(helper.popParameter().data<LibObject<handle_data_t>>()->impl);
#else
	int fd = static_cast<int>(to_integer(cursor, helper.popParameter()));

	uint64_t value = 0;
	read(fd, &value, sizeof (value));
#endif
}

MINT_FUNCTION(mint_event_wait, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	WeakReference timeout = move(helper.popParameter());

#ifdef OS_WINDOWS

	DWORD time_ms = INFINITE;
	HANDLE handle = helper.popParameter().data<LibObject<handle_data_t>>()->impl;

	if (timeout.data()->format != Data::fmt_none) {
		time_ms = static_cast<int>(to_integer(cursor, timeout));
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
	fds.fd = static_cast<int>(to_integer(cursor, helper.popParameter()));

	int time_ms = -1;

	if (timeout.data()->format != Data::fmt_none) {
		time_ms = static_cast<int>(to_integer(cursor, timeout));
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
