#include <memory/functiontool.h>
#include <memory/memorytool.h>
#include <memory/casttool.h>

#ifdef OS_WINDOWS

#else
#include <poll.h>
#endif

using namespace std;
using namespace mint;

MINT_FUNCTION(mint_watcher_poll, 2, cursor) {

	FunctionHelper helper(cursor, 2);

	SharedReference timeout = helper.popParameter();
	Array::values_type event_set = to_array(helper.popParameter());

#ifdef OS_WINDOWS

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
