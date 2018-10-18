#include <memory/functiontool.h>
#include <memory/casttool.h>

#include <vector>

#ifdef OS_UNIX
#include <poll.h>
#endif

#ifdef OS_WINDOWS
enum PollEvent {
	POLLIN   = 0x0001,
	POLLPRI  = 0x0002,
	POLLOUT  = 0x0004,
	POLLERR  = 0x0008,
	POLLHUP  = 0x0010,
	POLLNVAL = 0x0020
};

struct pollfd {
	HANDLE fd;
	short events;
	short revents;
};
#endif

using namespace mint;
using namespace std;

MINT_FUNCTION(mint_scheduler_set_events, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference events = helper.popParameter();
	SharedReference fd = helper.popParameter();

	fd->data<LibObject<pollfd>>()->impl->events = to_number(cursor, *events);
}

MINT_FUNCTION(mint_scheduler_get_revents, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference fd = helper.popParameter();

	helper.returnValue(create_number(fd->data<LibObject<pollfd>>()->impl->revents));
}

MINT_FUNCTION(mint_scheduler_poll, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference timeout = helper.popParameter();
	SharedReference fdset = helper.popParameter();

	vector<pollfd> fds;
	Array::values_type fd_array = to_array(*fdset);

	for (const SharedReference &fd : fd_array) {
		fds.push_back(*fd->data<LibObject<pollfd>>()->impl);
	}

#ifdef OS_UNIX
	int result = poll(fds.data(), fds.size(), static_cast<int>(to_number(cursor, *timeout)));
#else
	vector<WSAEVENT> handles;

	for (const SharedReference &fd : fds) {
		handles.push_back(fd.fd);
	}

	DWORD result = WSAWaitForMultipleEvents(static_cast<DWORD>(handles.size()), handles.data(), false, static_cast<DWORD>(mint_cast<Number>(cursor, *timeout)), true);

	/// \todo Convert events
#endif

	for (size_t i = 0; i < fds.size(); ++i) {
		fd_array[i]->data<LibObject<pollfd>>()->impl->revents = fds[i].revents;
	}

	helper.returnValue(create_number(result));
}
