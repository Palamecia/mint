#include "scheduler.h"

#include <memory/functiontool.h>
#include <memory/casttool.h>
#include <system/errno.h>

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

MINT_FUNCTION(mint_scheduler_pollfd_new, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &socket = helper.popParameter();

	WeakReference fd = create_object(new PollFd);
	fd.data<LibObject<PollFd>>()->impl->fd = to_integer(cursor, socket);
	fd.data<LibObject<PollFd>>()->impl->events = 0;
	fd.data<LibObject<PollFd>>()->impl->revents = 0;
#ifdef OS_WINDOWS
	fd.data<LibObject<PollFd>>()->impl->handle = WSACreateEvent();
#endif

	helper.returnValue(std::move(fd));
}

MINT_FUNCTION(mint_scheduler_pollfd_delete, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &fd = helper.popParameter();

#ifdef OS_WINDOWS
	WSACloseEvent(fd.data<LibObject<PollFd>>()->impl->handle);
#endif
	delete fd.data<LibObject<PollFd>>()->impl;
}

MINT_FUNCTION(mint_scheduler_set_events, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &events = helper.popParameter();
	Reference &fd = helper.popParameter();

	fd.data<LibObject<PollFd>>()->impl->events = static_cast<short>(to_number(cursor, events));
}

MINT_FUNCTION(mint_scheduler_get_events, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &fd = helper.popParameter();

	helper.returnValue(create_number(fd.data<LibObject<PollFd>>()->impl->events));
}

MINT_FUNCTION(mint_scheduler_get_revents, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &fd = helper.popParameter();

	helper.returnValue(create_number(fd.data<LibObject<PollFd>>()->impl->revents));
}

MINT_FUNCTION(mint_scheduler_poll, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &timeout = helper.popParameter();
	WeakReference handles = std::move(helper.popParameter());

	vector<PollFd> fdset;

	for (const Array::values_type::value_type &fd : handles.data<Array>()->values) {
		fdset.push_back(*fd.data<LibObject<PollFd>>()->impl);
	}

	helper.returnValue(create_boolean(Scheduler::instance().poll(fdset, static_cast<int>(to_integer(cursor, timeout)))));

	size_t i = 0;

	for (const Array::values_type::value_type &fd : handles.data<Array>()->values) {
		fd.data<LibObject<PollFd>>()->impl->revents = fdset[i++].revents;
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
#ifdef POLLRDHUP
		handle.events |= POLLRDHUP;
#endif
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
#ifdef POLLRDHUP
	if (handle.revents & POLLRDHUP) {
		desc.revents |= PollFd::close;
	}
#endif
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

Scheduler::Error::Error(bool status) :
	Error(status, status ? 0 : errno_from_io_last_error()) {

}

Scheduler::Error::Error(const Error &other) noexcept :
	Error(other.m_status, other.m_errno) {

}

Scheduler::Error::Error(bool _status, int _errno) :
	m_status(_status),
	m_errno(_errno) {

}

Scheduler::Error &Scheduler::Error::operator =(const Error &other) noexcept {
	m_status = other.m_status;
	m_errno = other.m_errno;
	return *this;
}

Scheduler::Error::operator bool() const {
	return !m_status;
}

int Scheduler::Error::getErrno() const {
	return m_errno;
}

Scheduler::Scheduler() {
#ifdef OS_WINDOWS
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 0), &wsaData);
#endif
}

Scheduler::~Scheduler() {
#ifdef OS_WINDOWS
	WSACleanup();
#endif
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

void Scheduler::acceptSocket(SOCKET fd) {
	m_sockets.emplace(fd, socket_infos{false, true, false});
}

Scheduler::Error Scheduler::closeSocket(SOCKET fd) {
	m_sockets.erase(fd);
#ifdef OS_UNIX
	return close(fd) == 0;
#else
	return closesocket(fd) == 0;
#endif
}

bool Scheduler::isSocketListening(SOCKET fd) const {

	auto i = m_sockets.find(fd);

	if (i != m_sockets.end()) {
		return i->second.listening;
	}

	return false;
}

void Scheduler::setSocketListening(SOCKET fd, bool listening) {

	auto i = m_sockets.find(fd);

	if (i != m_sockets.end()) {
		i->second.listening = listening;
	}
}

bool Scheduler::isSocketBlocking(SOCKET fd) const {

	auto i = m_sockets.find(fd);

	if (i != m_sockets.end()) {
		return i->second.blocking;
	}

	return true;
}

void Scheduler::setSocketBlocking(SOCKET fd, bool blocking) {

	auto i = m_sockets.find(fd);

	if (i != m_sockets.end()) {
		i->second.blocking = blocking;
	}
}

bool Scheduler::isSocketBlocked(SOCKET fd) const {

	auto i = m_sockets.find(fd);

	if (i != m_sockets.end()) {
		return i->second.blocked;
	}

	return false;
}

void Scheduler::setSocketBlocked(SOCKET fd, bool blocked) {

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

int errno_from_io_last_error() {
#ifdef OS_WINDOWS
	static const std::unordered_map<int, int> g_errno_for = {
		{ WSAEINTR, ECANCELED },
		{ WSAEBADF, EBADF },
		{ WSAEACCES, EACCES },
		{ WSAEFAULT, EFAULT },
		{ WSAEINVAL, EINVAL },
		{ WSAEMFILE, EMFILE },
		{ WSAEWOULDBLOCK, EWOULDBLOCK },
		{ WSAEINPROGRESS, EINPROGRESS },
		{ WSAEALREADY, EALREADY },
		{ WSAENOTSOCK, ENOTSOCK },
		{ WSAEDESTADDRREQ, EDESTADDRREQ },
		{ WSAEMSGSIZE, EMSGSIZE },
		{ WSAEPROTOTYPE, EPROTOTYPE },
		{ WSAENOPROTOOPT, ENOPROTOOPT },
		{ WSAEPROTONOSUPPORT, EPROTONOSUPPORT },
	#ifdef ESOCKTNOSUPPORT
		{ WSAESOCKTNOSUPPORT, ESOCKTNOSUPPORT },
	#endif
		{ WSAEOPNOTSUPP, EOPNOTSUPP },
	#ifdef EPFNOSUPPORT
		{ WSAEPFNOSUPPORT, EPFNOSUPPORT },
	#endif
		{ WSAEAFNOSUPPORT, EAFNOSUPPORT },
		{ WSAEADDRINUSE, EADDRINUSE },
		{ WSAEADDRNOTAVAIL, EADDRNOTAVAIL },
		{ WSAENETDOWN, ENETDOWN },
		{ WSAENETUNREACH, ENETUNREACH },
		{ WSAENETRESET, ENETRESET },
		{ WSAECONNABORTED, ECONNABORTED },
		{ WSAECONNRESET, ECONNRESET },
		{ WSAENOBUFS, ENOBUFS },
		{ WSAEISCONN, EISCONN },
		{ WSAENOTCONN, ENOTCONN },
	#ifdef ESHUTDOWN
		{ WSAESHUTDOWN, ESHUTDOWN },
	#endif
	#ifdef ETOOMANYREFS
		{ WSAETOOMANYREFS, ETOOMANYREFS },
	#endif
		{ WSAETIMEDOUT, ETIMEDOUT },
		{ WSAECONNREFUSED, ECONNREFUSED },
		{ WSAELOOP, ELOOP },
		{ WSAENAMETOOLONG, ENAMETOOLONG },
	#ifdef EHOSTDOWN
		{ WSAEHOSTDOWN, EHOSTDOWN },
	#endif
		{ WSAEHOSTUNREACH, EHOSTUNREACH },
		{ WSAENOTEMPTY, ENOTEMPTY },
	#ifdef EPROCLIM
		{ WSAEPROCLIM, EPROCLIM },
	#endif
	#ifdef EUSERS
		{ WSAEUSERS, EUSERS },
	#endif
	#ifdef EDQUOT
		{ WSAEDQUOT, EDQUOT },
	#endif
	#ifdef ESTALE
		{ WSAESTALE, ESTALE },
	#endif
	#ifdef EREMOTE
		{ WSAEREMOTE, EREMOTE },
	#endif
		/** @todo
		{ WSASYSNOTREADY, },
		{ WSAVERNOTSUPPORTED, },
		{ WSANOTINITIALISED, },
		{ WSAEDISCON, },
		{ WSAENOMORE, },
		{ WSAECANCELLED, },
		{ WSAEINVALIDPROCTABLE, },
		{ WSAEINVALIDPROVIDER, },
		{ WSAEPROVIDERFAILEDINIT, },
		{ WSASYSCALLFAILURE, },
		{ WSASERVICE_NOT_FOUND, },
		{ WSATYPE_NOT_FOUND, },
		{ WSA_E_NO_MORE, },
		{ WSA_E_CANCELLED, },
		{ WSAEREFUSED, },
		{ WSAHOST_NOT_FOUND, },
		{ WSATRY_AGAIN, },
		{ WSANO_RECOVERY, },
		{ WSANO_DATA, },
		{ WSA_QOS_RECEIVERS, },
		{ WSA_QOS_SENDERS, },
		{ WSA_QOS_NO_SENDERS, },
		{ WSA_QOS_NO_RECEIVERS, },
		{ WSA_QOS_REQUEST_CONFIRMED, },
		{ WSA_QOS_ADMISSION_FAILURE, },
		{ WSA_QOS_POLICY_FAILURE, },
		{ WSA_QOS_BAD_STYLE, },
		{ WSA_QOS_BAD_OBJECT, },
		{ WSA_QOS_TRAFFIC_CTRL_ERROR, },
		{ WSA_QOS_GENERIC_ERROR, },
		{ WSA_QOS_ESERVICETYPE, },
		{ WSA_QOS_EFLOWSPEC, },
		{ WSA_QOS_EPROVSPECBUF, },
		{ WSA_QOS_EFILTERSTYLE, },
		{ WSA_QOS_EFILTERTYPE, },
		{ WSA_QOS_EFILTERCOUNT, },
		{ WSA_QOS_EOBJLENGTH, },
		{ WSA_QOS_EFLOWCOUNT, },
		{ WSA_QOS_EUNKOWNPSOBJ, },
		{ WSA_QOS_EPOLICYOBJ, },
		{ WSA_QOS_EFLOWDESC, },
		{ WSA_QOS_EPSFLOWSPEC, },
		{ WSA_QOS_EPSFILTERSPEC, },
		{ WSA_QOS_ESDMODEOBJ, },
		{ WSA_QOS_ESHAPERATEOBJ, },
		{ WSA_QOS_RESERVED_PETYPE, },
		{ WSA_SECURE_HOST_NOT_FOUND, },
		{ WSA_IPSEC_NAME_POLICY_ERROR, }
		*/
	};

	auto i = g_errno_for.find(WSAGetLastError());
	return (i != g_errno_for.end()) ? i->second : EINVAL;
#else
	return errno;
#endif
}
