#include "scheduler.h"

#include <memory/functiontool.h>
#include <memory/casttool.h>

#include <sys/socket.h>
#include <unistd.h>

using namespace mint;
using namespace std;

MINT_FUNCTION(mint_scheduler_pollfd_new, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	SharedReference events = helper.popParameter();
	SharedReference handle = helper.popParameter();
	SharedReference socket = helper.popParameter();

	SharedReference fd = create_object(new pollfd);
	fd->data<LibObject<pollfd>>()->impl->fd = static_cast<int>(socket->data<Number>()->value);
	fd->data<LibObject<pollfd>>()->impl->events = static_cast<short>(to_number(cursor, *events));
	fd->data<LibObject<pollfd>>()->impl->revents = 0;
#ifdef OS_WINDOWS
	fd->data<LibObject<pollfd>>()->impl->handle = *handle->data<LibObject<WSAEVENT>>()->impl;
#endif
	helper.returnValue(fd);
}

MINT_FUNCTION(mint_scheduler_pollfd_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference fd = helper.popParameter();
	delete fd->data<LibObject<pollfd>>()->impl;
}

MINT_FUNCTION(mint_scheduler_set_events, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference events = helper.popParameter();
	SharedReference fd = helper.popParameter();

	fd->data<LibObject<pollfd>>()->impl->events = static_cast<short>(to_number(cursor, *events));
}

MINT_FUNCTION(mint_scheduler_get_revents, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference fd = helper.popParameter();

	helper.returnValue(create_number(fd->data<LibObject<pollfd>>()->impl->revents));
}

MINT_FUNCTION(mint_scheduler_poll, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference timeout = helper.popParameter();
	SharedReference handles = helper.popParameter();

	vector<pollfd> fdset;

	for (const SharedReference &fd : handles->data<Array>()->values) {
		fdset.push_back(*fd->data<LibObject<pollfd>>()->impl);
	}

	helper.returnValue(create_number(Scheduler::instance().poll(fdset, static_cast<int>(to_number(cursor, *timeout)))));

	size_t i = 0;

	for (const SharedReference &fd : handles->data<Array>()->values) {
		fd->data<LibObject<pollfd>>()->impl->revents = fdset[i++].revents;
	}
}

Scheduler::Scheduler() {

}

Scheduler::~Scheduler() {

}

Scheduler &Scheduler::instance() {
	static Scheduler g_instance;
	return g_instance;
}

int Scheduler::openSocket(int domain, int type, int protocol) {

	int fd = ::socket(domain, type, protocol);

	if (fd != -1) {
		m_sockets.emplace(fd, false);
	}

	return fd;
}

void Scheduler::closeSocket(int fd) {
	m_sockets.erase(fd);
	close(fd);
}

bool Scheduler::isSocketBlocket(int fd) const {

	auto i = m_sockets.find(fd);

	if (i != m_sockets.end()) {
		return i->second;
	}

	return false;
}

void Scheduler::setSocketBlocked(int fd, bool blocked) {

	auto i = m_sockets.find(fd);

	if (i != m_sockets.end()) {
		i->second = blocked;
	}
}

int Scheduler::poll(vector<pollfd> &fdset, int timeout) {

#ifdef OS_UNIX
	int result = ::poll(fdset.data(), fdset.size(), timeout);
#else
	vector<WSAEVENT> handles;

	for (const pollfd &fd : fdset) {
		handles.push_back(fd.handle);
	}

	DWORD result = WSAWaitForMultipleEvents(static_cast<DWORD>(handles.size()), handles.data(), false, static_cast<DWORD>(timeout), true);

	/// \todo Convert events
#endif

	for (pollfd &fd : fdset) {
		if (isSocketBlocket(fd.fd)) {
			setSocketBlocked(fd.fd, fd.revents & POLLIN);
		}
		else if (fd.events & POLLIN) {
			fd.revents |= POLLIN;
		}
	}

	return result;
}
