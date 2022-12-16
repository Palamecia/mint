#include <memory/functiontool.h>
#include <memory/casttool.h>
#include "scheduler.h"
#include "socket.h"

#ifdef OS_WINDOWS
#include <ws2tcpip.h>
#else
#include <fcntl.h>
#include <sys/ioctl.h>
#endif

using namespace mint;
using namespace std;

bool mint::get_socket_option(SOCKET socket, int option, int *value) {
	socklen_t len = sizeof(int);
#ifdef OS_WINDOWS
	return getsockopt(socket, SOL_SOCKET, option, reinterpret_cast<char *>(value), &len) == 0;
#else
	return getsockopt(socket, SOL_SOCKET, option, value, &len) == 0;
#endif
}

bool mint::set_socket_option(SOCKET socket, int option, int value) {
	return setsockopt(socket, SOL_SOCKET, option, reinterpret_cast<const char *>(&value), sizeof(value)) == 0;
}

MINT_FUNCTION(mint_socket_is_non_blocking, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &socket = helper.popParameter();

	bool status = false;
	const SOCKET socket_fd = to_integer(cursor, socket);

#ifdef OS_WINDOWS
	status = Scheduler::instance().isSocketBlocking(socket_fd);
#else
	int flags = 0;

	if ((flags = fcntl(socket_fd, F_GETFL, 0)) != -1) {
		status = flags & O_NONBLOCK;
	}
#endif

	helper.returnValue(create_boolean(status));
}

MINT_FUNCTION(mint_socket_set_non_blocking, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	Reference &enabled = helper.popParameter();
	Reference &socket = helper.popParameter();

	bool success = false;
	const SOCKET socket_fd = to_integer(cursor, socket);

#ifdef OS_WINDOWS
	u_long value = static_cast<u_long>(to_boolean(cursor, enabled));

	if (ioctlsocket(socket_fd, FIONBIO, &value) != SOCKET_ERROR) {
		success = true;
	}
	else {
		helper.returnValue(create_number(errno_from_io_last_error()));
	}
#else
	int value = static_cast<int>(to_boolean(cursor, enabled));

	if (ioctl(socket_fd, FIONBIO, &value) != -1) {
		success = true;
	}
	else {
		helper.returnValue(create_number(errno));
	}
#endif

	if (success) {
		Scheduler::instance().setSocketBlocking(socket_fd, value != 0);
	}
}

MINT_FUNCTION(mint_socket_finalize_connection, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &socket = helper.popParameter();
	WeakReference result = create_iterator();

	int error = EINVAL;
	const SOCKET socket_fd = to_integer(cursor, socket);
	auto IOStatus = helper.reference(symbols::Network).member(symbols::EndPoint).member(symbols::IOStatus);

	if (!get_socket_option(socket_fd, SO_ERROR, &error)) {
		error = errno_from_io_last_error();
	}

	switch (error) {
	case 0:
		iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOSuccess));
		break;
	case EALREADY:
	case EINPROGRESS:
	case EWOULDBLOCK:
		iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOWouldBlock));
		Scheduler::instance().setSocketBlocked(socket_fd, true);
		break;
	default:
		iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOError));
		iterator_insert(result.data<Iterator>(), create_number(error));
		break;
	}

	helper.returnValue(std::move(result));
}

MINT_FUNCTION(mint_socket_shutdown, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &socket = helper.popParameter();
	WeakReference result = create_iterator();

#ifdef OS_WINDOWS
	const int how = SD_BOTH;
#else
	const int how = SHUT_RDWR;
#endif
	const SOCKET socket_fd = to_integer(cursor, socket);
	auto IOStatus = helper.reference(symbols::Network).member(symbols::EndPoint).member(symbols::IOStatus);

	if (::shutdown(socket_fd, how) == 0) {
		iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOSuccess));
	}
	else {
		switch (int error = errno_from_io_last_error()) {
		case EINPROGRESS:
		case EWOULDBLOCK:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOWouldBlock));
			break;
		case ENOTCONN:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOClosed));
			break;
		default:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOError));
			iterator_insert(result.data<Iterator>(), create_number(error));
			break;
		}
	}

	helper.returnValue(std::move(result));
}

MINT_FUNCTION(mint_socket_close, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &socket = helper.popParameter();

	const SOCKET socket_fd = to_integer(cursor, socket);

	if (Scheduler::Error error = Scheduler::instance().closeSocket(socket_fd)) {
		helper.returnValue(create_number(error.getErrno()));
	}
}

MINT_FUNCTION(mint_socket_get_error, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &socket = helper.popParameter();

	int error = 0;

	if (get_socket_option(to_integer(cursor, socket), SO_ERROR, &error)) {
		helper.returnValue(create_number(error));
	}
	else {
		helper.returnValue(create_number(errno_from_io_last_error()));
	}
}

MINT_FUNCTION(mint_socket_strerror, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &error = helper.popParameter();

	helper.returnValue(create_string(strerror(to_integer(cursor, error))));
}
