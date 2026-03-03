/**
 * Copyright (c) 2026 Gauvain CHERY.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "scheduler.h"

#include "mint/ast/cursor.h"
#include "mint/memory/builtin/array.h"
#include "mint/memory/builtin/libobject.h"
#include "mint/memory/data.h"
#include "mint/memory/memorytool.h"
#include "mint/memory/object.h"
#include "mint/memory/reference.h"
#include "mint/memory/functiontool.h"
#include "mint/memory/casttool.h"
#include "mint/system/errno.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <ranges>
#include <unordered_map>
#include <utility>
#include <vector>

#ifdef MINT_OS_WINDOWS
#include <Windows.h>
#include <minwindef.h>
#include <winerror.h>
#include <WinSock2.h>
#else
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace {

mint::WeakReference mint_scheduler_pollfd_new(mint::Cursor& cursor, const mint::Reference& socket) {
	return mint::create_c_object(cursor.ast(), new PollFd {
	                                               .fd = mint::to_integer<SOCKET>(cursor, socket),
	                                               .events = 0,
	                                               .revents = 0,
#ifdef MINT_OS_WINDOWS
	                                               .handle = WSACreateEvent(),
#endif
	                                           });
}

mint::WeakReference mint_scheduler_pollfd_delete(mint::Cursor& /*cursor*/, const mint::Reference& fd) {
#ifdef MINT_OS_WINDOWS
	WSACloseEvent(fd.data<mint::LibObject<PollFd>>().ptr->handle);
#endif
	delete fd.data<mint::LibObject<PollFd>>().ptr;
	return {};
}

mint::WeakReference mint_scheduler_set_events(mint::Cursor& cursor, const mint::Reference& fd,
    const mint::Reference& events) {
	fd.data<mint::LibObject<PollFd>>().ptr->events = mint::to_integer<short>(cursor, events);
	return {};
}

mint::WeakReference mint_scheduler_get_events(mint::Cursor& /*cursor*/, const mint::Reference& fd) {
	return mint::create_number(fd.data<mint::LibObject<PollFd>>().ptr->events);
}

mint::WeakReference mint_scheduler_get_revents(mint::Cursor& /*cursor*/, const mint::Reference& fd) {
	return mint::create_number(fd.data<mint::LibObject<PollFd>>().ptr->revents);
}

mint::WeakReference mint_scheduler_poll(mint::Cursor& cursor, const mint::Reference& handles,
    const mint::Reference& timeout) {

	auto fdset = std::vector(std::from_range,
	    std::views::transform(handles.data<mint::Array>().values, [](const auto& fd) {
		    return *fd.template data<mint::LibObject<PollFd>>().ptr;
	    }));

	const auto result = Scheduler::instance().poll(fdset, mint::to_integer<int>(cursor, timeout));

	for (const auto& [fd, poll_fd] : std::views::zip(handles.data<mint::Array>().values, fdset)) {
		fd.data<mint::LibObject<PollFd>>().ptr->revents = poll_fd.revents;
	}

	return mint::create_boolean(result);
}

}

Scheduler::Error::Error(bool status) :
    Error(status, status ? 0 : errno_from_io_last_error()) {}

Scheduler::Error::Error(bool status, int errno_value) :
    _status(status),
    _errno(errno_value) {}

Scheduler::Error::operator bool() const {
	return !_status;
}

int Scheduler::Error::get_errno() const {
	return _errno;
}

Scheduler::Scheduler() {
#ifdef MINT_OS_WINDOWS
	WSAStartup(MAKEWORD(2, 0), &_wsa_data);
#endif
}

Scheduler::~Scheduler() {
#ifdef MINT_OS_WINDOWS
	WSACleanup();
#endif
}

Scheduler& Scheduler::instance() {
	static Scheduler g_instance;
	return g_instance;
}

SOCKET Scheduler::open_socket(int domain, int type, int protocol) {

	const auto fd = ::socket(domain, type, protocol);

	if (fd != INVALID_SOCKET) {
		_sockets.emplace(fd, SocketInfo {
		                         .blocked = false,
		                         .blocking = true,
		                         .listening = false,
		                     });
	}

	return fd;
}

void Scheduler::accept_socket(SOCKET fd) {
	_sockets.emplace(fd, SocketInfo {
	                         .blocked = false,
	                         .blocking = true,
	                         .listening = false,
	                     });
}

Scheduler::Error Scheduler::close_socket(SOCKET fd) {
	_sockets.erase(fd);
#ifdef MINT_OS_UNIX
	return close(fd) == 0;
#else
	return closesocket(fd) == 0;
#endif
}

bool Scheduler::is_socket_listening(SOCKET fd) const {
	if (auto i = _sockets.find(fd); i != _sockets.end()) {
		return i->second.listening;
	}
	return false;
}

void Scheduler::set_socket_listening(SOCKET fd, bool listening) {
	if (auto i = _sockets.find(fd); i != _sockets.end()) {
		i->second.listening = listening;
	}
}

bool Scheduler::is_socket_blocking(SOCKET fd) const {
	if (auto i = _sockets.find(fd); i != _sockets.end()) {
		return i->second.blocking;
	}
	return true;
}

void Scheduler::set_socket_blocking(SOCKET fd, bool blocking) {
	if (auto i = _sockets.find(fd); i != _sockets.end()) {
		i->second.blocking = blocking;
	}
}

bool Scheduler::is_socket_blocked(SOCKET fd) const {
	if (auto i = _sockets.find(fd); i != _sockets.end()) {
		return i->second.blocked;
	}
	return false;
}

void Scheduler::set_socket_blocked(SOCKET fd, bool blocked) {
	if (auto i = _sockets.find(fd); i != _sockets.end()) {
		i->second.blocked = blocked;
	}
}

bool Scheduler::poll(std::vector<PollFd>& fdset, int timeout) {

	std::vector<native_handle_t> handles(std::from_range,
	    std::views::transform(fdset, std::bind_front(&Scheduler::to_native_handle, this)));

#ifdef MINT_OS_UNIX
	bool result = ::poll(handles.data(), handles.size(), timeout) != 0;
#else
	bool result = WSAWaitForMultipleEvents(static_cast<DWORD>(handles.size()), handles.data(), false,
	                  static_cast<DWORD>(timeout), true)
	              != WSA_WAIT_TIMEOUT;
#endif

	for (std::size_t i = 0; i < handles.size(); ++i) {
		if (revents_from_native_handle(fdset[i], handles[i])) {
			result = true;
		}
	}

	return result;
}

native_handle_t Scheduler::to_native_handle(const PollFd& desc) const {
#ifdef MINT_OS_UNIX
	pollfd handle {
	    .fd = desc.fd,
	    .events = 0,
	    .revents = 0,
	};

	if (desc.events & PollFd::read_event) {
		handle.events |= (POLLIN | POLLPRI);
	}
	if (desc.events & PollFd::write_event) {
		handle.events |= POLLOUT;
	}
	if (desc.events & PollFd::accept_event) {
		handle.events |= POLLIN;
	}
	if (desc.events & PollFd::error_event) {
		handle.events |= (POLLERR | POLLNVAL);
	}
	if (desc.events & PollFd::close_event) {
		handle.events |= POLLHUP;
#ifdef POLLRDHUP
		handle.events |= POLLRDHUP;
#endif
	}
	return handle;
#else
	long events = 0;

	if (desc.events & PollFd::read_event) {
		events |= FD_READ;
	}
	if (desc.events & PollFd::write_event) {
		events |= FD_WRITE;
	}
	if (desc.events & PollFd::accept_event) {
		events |= FD_ACCEPT;
	}
	if (desc.events & PollFd::close_event) {
		events |= FD_CLOSE;
	}

	if (is_socket_blocked(desc.fd)) {
		events |= FD_WRITE;
	}

	WSAEventSelect(desc.fd, desc.handle, events);
	return desc.handle;
#endif
}

bool Scheduler::revents_from_native_handle(PollFd& desc, [[maybe_unused]] const native_handle_t& handle) {

	bool fake_event = false;
	desc.revents = 0;

#ifdef MINT_OS_UNIX
	if ((handle.revents & (POLLIN | POLLPRI)) && !Scheduler::instance().is_socket_listening(handle.fd)) {
		desc.revents |= PollFd::read_event;
	}
	if (handle.revents & POLLOUT) {
		desc.revents |= PollFd::write_event;
	}
	if ((handle.revents & POLLIN) && Scheduler::instance().is_socket_listening(handle.fd)) {
		desc.revents |= PollFd::accept_event;
	}
	if (handle.revents & (POLLERR | POLLNVAL)) {
		desc.revents |= PollFd::error_event;
	}
	if (handle.revents & POLLHUP) {
		desc.revents |= PollFd::close_event;
	}
#ifdef POLLRDHUP
	if (handle.revents & POLLRDHUP) {
		desc.revents |= PollFd::close_event;
	}
#endif
#else
	WSANETWORKEVENTS events;
	WSAEnumNetworkEvents(desc.fd, desc.handle, &events);

	if (events.lNetworkEvents & FD_READ) {
		if ((desc.events & PollFd::error_event) && events.iErrorCode[FD_READ_BIT]) {
			desc.revents |= PollFd::error_event;
		}
		desc.revents |= PollFd::read_event;
	}
	if (events.lNetworkEvents & FD_WRITE) {
		if ((desc.events & PollFd::error_event) && events.iErrorCode[FD_WRITE_BIT]) {
			desc.revents |= PollFd::error_event;
		}
		desc.revents |= PollFd::write_event;
	}
	if (events.lNetworkEvents & FD_ACCEPT) {
		if ((desc.events & PollFd::error_event) && events.iErrorCode[FD_ACCEPT_BIT]) {
			desc.revents |= PollFd::error_event;
		}
		desc.revents |= PollFd::accept_event;
	}
	if (events.lNetworkEvents & FD_CLOSE) {
		desc.revents |= PollFd::close_event;
	}

	if (is_socket_blocked(desc.fd)) {
		set_socket_blocked(desc.fd, events.lNetworkEvents & FD_WRITE);
	}
	else if (desc.events & PollFd::write_event) {
		desc.revents |= PollFd::write_event;
		fake_event = true;
	}
#endif

	return fake_event;
}

int errno_from_io_last_error() {
#ifdef MINT_OS_WINDOWS
	static const std::unordered_map<int, int> g_errno_for = {
	    {WSAEINTR, ECANCELED},
	    {WSAEBADF, EBADF},
	    {WSAEACCES, EACCES},
	    {WSAEFAULT, EFAULT},
	    {WSAEINVAL, EINVAL},
	    {WSAEMFILE, EMFILE},
	    {WSAEWOULDBLOCK, EWOULDBLOCK},
	    {WSAEINPROGRESS, EINPROGRESS},
	    {WSAEALREADY, EALREADY},
	    {WSAENOTSOCK, ENOTSOCK},
	    {WSAEDESTADDRREQ, EDESTADDRREQ},
	    {WSAEMSGSIZE, EMSGSIZE},
	    {WSAEPROTOTYPE, EPROTOTYPE},
	    {WSAENOPROTOOPT, ENOPROTOOPT},
	    {WSAEPROTONOSUPPORT, EPROTONOSUPPORT},
#ifdef ESOCKTNOSUPPORT
	    {WSAESOCKTNOSUPPORT, ESOCKTNOSUPPORT},
#endif
	    {WSAEOPNOTSUPP, EOPNOTSUPP},
#ifdef EPFNOSUPPORT
	    {WSAEPFNOSUPPORT, EPFNOSUPPORT},
#endif
	    {WSAEAFNOSUPPORT, EAFNOSUPPORT},
	    {WSAEADDRINUSE, EADDRINUSE},
	    {WSAEADDRNOTAVAIL, EADDRNOTAVAIL},
	    {WSAENETDOWN, ENETDOWN},
	    {WSAENETUNREACH, ENETUNREACH},
	    {WSAENETRESET, ENETRESET},
	    {WSAECONNABORTED, ECONNABORTED},
	    {WSAECONNRESET, ECONNRESET},
	    {WSAENOBUFS, ENOBUFS},
	    {WSAEISCONN, EISCONN},
	    {WSAENOTCONN, ENOTCONN},
#ifdef ESHUTDOWN
	    {WSAESHUTDOWN, ESHUTDOWN},
#endif
#ifdef ETOOMANYREFS
	    {WSAETOOMANYREFS, ETOOMANYREFS},
#endif
	    {WSAETIMEDOUT, ETIMEDOUT},
	    {WSAECONNREFUSED, ECONNREFUSED},
	    {WSAELOOP, ELOOP},
	    {WSAENAMETOOLONG, ENAMETOOLONG},
#ifdef EHOSTDOWN
	    {WSAEHOSTDOWN, EHOSTDOWN},
#endif
	    {WSAEHOSTUNREACH, EHOSTUNREACH},
	    {WSAENOTEMPTY, ENOTEMPTY},
#ifdef EPROCLIM
	    {WSAEPROCLIM, EPROCLIM},
#endif
#ifdef EUSERS
	    {WSAEUSERS, EUSERS},
#endif
#ifdef EDQUOT
	    {WSAEDQUOT, EDQUOT},
#endif
#ifdef ESTALE
	    {WSAESTALE, ESTALE},
#endif
#ifdef EREMOTE
	    {WSAEREMOTE, EREMOTE},
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

MINT_RAW_FUNCTION(mint_scheduler_spawn, 1, cursor) {

	auto coroutine = cursor.stack().back();
	cursor.stack().pop_back();

	if (mint::is_instance_of(coroutine, mint::Data::Format::coroutine)) {
		coroutine.data<mint::Coroutine>().call(cursor, mint::WeakReference(coroutine));
	}
	else {
		cursor.stack().emplace_back(mint::create_none());
	}
}

MINT_RAW_FUNCTION(mint_scheduler_current_coroutine, 0, cursor) {

	cursor.exit_call();

	if (cursor.is_in_coroutine()) {
		cursor.stack().emplace_back(cursor.coroutine());
	}
	else {
		cursor.stack().emplace_back(mint::create_none());
	}
}

MINT_RAW_FUNCTION(mint_scheduler_suspend, 1, cursor) {

	auto coroutine = std::move(cursor.stack().back());
	cursor.stack().pop_back();
	cursor.exit_call();

	assert(mint::is_instance_of(coroutine, mint::Data::Format::coroutine));
	coroutine.data<mint::Coroutine>().suspend(cursor);
	cursor.stack().emplace_back(mint::create_none());
}

MINT_RAW_FUNCTION(mint_scheduler_raise, 2, cursor) {

	auto error = std::move(cursor.stack().back());
	cursor.stack().pop_back();

	auto coroutine = std::move(cursor.stack().back());
	cursor.stack().pop_back();
	// cursor.exit_call();

	assert(mint::is_instance_of(coroutine, mint::Data::Format::coroutine));
	coroutine.data<mint::Coroutine>().resume(cursor);
	cursor.raise(mint::WeakReference(error));
}

MINT_RAW_FUNCTION(mint_scheduler_resume, 1, cursor) {

	auto coroutine = std::move(cursor.stack().back());
	cursor.stack().pop_back();

	assert(mint::is_instance_of(coroutine, mint::Data::Format::coroutine));
	coroutine.data<mint::Coroutine>().resume(cursor);
}

MINT_EXPORT_FUNCTION(mint_scheduler_pollfd_new, 1);
MINT_EXPORT_FUNCTION(mint_scheduler_pollfd_delete, 1);
MINT_EXPORT_FUNCTION(mint_scheduler_set_events, 2);
MINT_EXPORT_FUNCTION(mint_scheduler_get_events, 1);
MINT_EXPORT_FUNCTION(mint_scheduler_get_revents, 1);
MINT_EXPORT_FUNCTION(mint_scheduler_poll, 2);
