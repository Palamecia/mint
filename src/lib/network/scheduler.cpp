#include "scheduler.h"

#include <memory/functiontool.h>
#include <memory/casttool.h>

#ifdef OS_UNIX
#include <sys/socket.h>
#include <unistd.h>
using native_handle_t = pollfd;
#else
#include <Windows.h>
using native_handle_t = HANDLE;
#endif

using namespace mint;
using namespace std;

MINT_FUNCTION(mint_scheduler_pollfd_new, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	SharedReference events = move(helper.popParameter());
	SharedReference handle = move(helper.popParameter());
	SharedReference socket = move(helper.popParameter());

	SharedReference fd = create_object(new PollFd);
	fd->data<LibObject<PollFd>>()->impl->fd = static_cast<int>(socket->data<Number>()->value);
	fd->data<LibObject<PollFd>>()->impl->events = static_cast<short>(to_number(cursor, events));
	fd->data<LibObject<PollFd>>()->impl->revents = 0;
#ifdef OS_WINDOWS
	fd->data<LibObject<PollFd>>()->impl->handle = *handle->data<LibObject<WSAEVENT>>()->impl;
#endif
	helper.returnValue(move(fd));
}

MINT_FUNCTION(mint_scheduler_pollfd_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference fd = move(helper.popParameter());
	delete fd->data<LibObject<PollFd>>()->impl;
}

MINT_FUNCTION(mint_scheduler_set_events, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference events = move(helper.popParameter());
	SharedReference fd = move(helper.popParameter());

	fd->data<LibObject<PollFd>>()->impl->events = static_cast<short>(to_number(cursor, events));
}

MINT_FUNCTION(mint_scheduler_get_revents, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference fd = move(helper.popParameter());

	helper.returnValue(create_number(fd->data<LibObject<PollFd>>()->impl->revents));
}

MINT_FUNCTION(mint_scheduler_poll, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference timeout = move(helper.popParameter());
	SharedReference handles = move(helper.popParameter());

	vector<PollFd> fdset;

	for (const SharedReference &fd : handles->data<Array>()->values) {
		fdset.push_back(*fd->data<LibObject<PollFd>>()->impl);
	}

	helper.returnValue(create_boolean(Scheduler::instance().poll(fdset, static_cast<int>(to_number(cursor, timeout)))));

	size_t i = 0;

	for (const SharedReference &fd : handles->data<Array>()->values) {
		fd->data<LibObject<PollFd>>()->impl->revents = fdset[i++].revents;
	}
}

static native_handle_t to_native_handle(const PollFd &desc) {
#ifdef OS_UNIX
	pollfd handle;

	handle.fd = desc.fd;
	handle.events = 0;
	handle.revents = 0;

	if (desc.events & PollFd::read) {
		handle.events |= (POLLIN | POLLPRI);
	}
	if (desc.events & PollFd::write) {
		handle.events |= POLLOUT;
	}
	if (desc.events & PollFd::accept) {
		handle.events |= POLLIN;
	}
	if (desc.events & PollFd::error) {
		handle.events |= (POLLERR | POLLNVAL);
	}
	if (desc.events & PollFd::close) {
		handle.events |= POLLHUP;
	}
	return handle;
#else
	long events = 0;

	if (desc.events & PollFd::read) {
		events |= FD_READ;
	}
	if (desc.events & PollFd::write) {
		events |= FD_WRITE;
	}
	if (desc.events & PollFd::accept) {
		events |= FD_ACCEPT;
	}
	if (desc.events & PollFd::close) {
		events |= FD_CLOSE;
	}

	WSAEventSelect(desc.fd, desc.handle, events);
	return desc.handle;
#endif
}

static void revents_from_native_handle(PollFd &desc, const native_handle_t &handle) {

	desc.revents = 0;

#ifdef OS_UNIX
	if ((handle.revents & (POLLIN | POLLPRI)) && !Scheduler::instance().isSocketListening(handle.fd)) {
		desc.revents |= PollFd::read;
	}
	if (handle.revents & POLLOUT) {
		desc.revents |= PollFd::write;
	}
	if ((handle.revents & POLLIN) && Scheduler::instance().isSocketListening(handle.fd)) {
		desc.revents |= PollFd::accept;
	}
	if (handle.revents & (POLLERR | POLLNVAL)) {
		desc.revents |= PollFd::error;
	}
	if (handle.revents & POLLHUP) {
		desc.revents |= PollFd::close;
	}
#else
	WSANETWORKEVENTS events;
	WSAEnumNetworkEvents(desc.fd, desc.handle, &events);

	if (events.lNetworkEvents & FD_READ) {
		if ((desc.events & PollFd::error) && events.iErrorCode[FD_READ_BIT]) {
			desc.revents |= PollFd::error;
		}
		desc.revents |= PollFd::read;
	}
	if (events.lNetworkEvents & FD_WRITE) {
		if ((desc.events & PollFd::error) && events.iErrorCode[FD_WRITE_BIT]) {
			desc.revents |= PollFd::error;
		}
		desc.revents |= PollFd::write;
	}
	if (events.lNetworkEvents & FD_ACCEPT) {
		if ((desc.events & PollFd::error) && events.iErrorCode[FD_ACCEPT_BIT]) {
			desc.revents |= PollFd::error;
		}
		desc.revents |= PollFd::accept;
	}
	if (events.lNetworkEvents & FD_CLOSE) {
		if ((desc.events & PollFd::error) && events.iErrorCode[FD_CLOSE_BIT]) {
			desc.revents |= PollFd::error;
		}
		desc.revents |= PollFd::close;
	}

	if (Scheduler::instance().isSocketBlocked(desc.fd)) {
		Scheduler::instance().setSocketBlocked(desc.fd, desc.revents & PollFd::read);
	}
	else if (desc.events & PollFd::read) {
		desc.revents |= PollFd::read;
	}
#endif
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
		m_sockets.emplace(fd, socket_infos{false, true, false});
	}

	return fd;
}

void Scheduler::closeSocket(int fd) {
	m_sockets.erase(fd);
#ifdef OS_UNIX
	close(fd);
#else
	closesocket(fd);
#endif
}

bool Scheduler::isSocketListening(int fd) const {

	auto i = m_sockets.find(fd);

	if (i != m_sockets.end()) {
		return i->second.listening;
	}

	return false;
}

void Scheduler::setSocketListening(int fd, bool listening) {

	auto i = m_sockets.find(fd);

	if (i != m_sockets.end()) {
		i->second.listening = listening;
	}
}

bool Scheduler::isSocketBlocking(int fd) const {

	auto i = m_sockets.find(fd);

	if (i != m_sockets.end()) {
		return i->second.blocking;
	}

	return true;
}

void Scheduler::setSocketBlocking(int fd, bool blocking) {

	auto i = m_sockets.find(fd);

	if (i != m_sockets.end()) {
		i->second.blocking = blocking;
	}
}

bool Scheduler::isSocketBlocked(int fd) const {

	auto i = m_sockets.find(fd);

	if (i != m_sockets.end()) {
		return i->second.blocked;
	}

	return false;
}

void Scheduler::setSocketBlocked(int fd, bool blocked) {

	auto i = m_sockets.find(fd);

	if (i != m_sockets.end()) {
		i->second.blocked = blocked;
	}
}

bool Scheduler::poll(vector<PollFd> &fdset, int timeout) {

	vector<native_handle_t> handles;

	for (const PollFd &fd : fdset) {
		handles.push_back(to_native_handle(fd));
	}

#ifdef OS_UNIX
	bool result = ::poll(handles.data(), handles.size(), timeout) != 0;
#else
	bool result = WSAWaitForMultipleEvents(static_cast<DWORD>(handles.size()), handles.data(), false, static_cast<DWORD>(timeout), true) != WSA_WAIT_TIMEOUT;
#endif

	for (size_t i = 0; i < handles.size(); ++i) {
		revents_from_native_handle(fdset[i], handles[i]);
	}

	return result;
}
