#include <memory/functiontool.h>
#include <memory/memorytool.h>
#include <memory/casttool.h>

#ifdef OS_WINDOWS
#include <Windows.h>
using handle_data_t = std::remove_pointer<HANDLE>::type;
#else
#include <poll.h>
#endif

using namespace std;
using namespace mint;

MINT_FUNCTION(mint_watcher_poll, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	SharedReference timeout = move(helper.popParameter());
	Array::values_type event_set = to_array(helper.popParameter());

#ifdef OS_WINDOWS
	vector<HANDLE> fdset;

	for (SharedReference &item : event_set) {
		fdset.push_back(get_object_member(cursor, *item, "handle")->data<LibObject<handle_data_t>>()->impl);
	}

	DWORD time_ms = INFINITE;

	if (timeout->data()->format != Data::fmt_none) {
		time_ms = static_cast<int>(to_number(cursor, timeout));
	}

	DWORD status = WaitForMultipleObjectsEx(fdset.size(), fdset.data(), false, time_ms, true);

	while (status == WAIT_IO_COMPLETION) {
		status = WaitForMultipleObjectsEx(fdset.size(), fdset.data(), false, 0, true);
	}

	for (size_t i = status - WAIT_OBJECT_0 + 1; i < fdset.size(); ++i) {
		get_object_member(cursor, *event_set.at(i), "activated")->data<Boolean>()->value = (WaitForSingleObjectEx(fdset[i], 0, true) == WAIT_OBJECT_0);
	}
#else
	vector<pollfd> fdset;

	for (SharedReference &item : event_set) {
		pollfd fd;
		fd.fd = static_cast<int>(to_number(cursor, get_object_member(cursor, *item, "handle")));
		fd.events = POLLIN;
		fdset.push_back(fd);
	}

	int time_ms = -1;

	if (timeout->data()->format != Data::fmt_none) {
		time_ms = static_cast<int>(to_number(cursor, timeout));
	}

	poll(fdset.data(), fdset.size(), time_ms);

	for (size_t i = 0; i < fdset.size(); ++i) {
		get_object_member(cursor, *event_set.at(i), "activated")->data<Boolean>()->value = fdset.at(i).revents & POLLIN;
	}
#endif
}
