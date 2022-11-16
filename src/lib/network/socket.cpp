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

bool mint::get_socket_option(int socket, int option, int *value) {
	socklen_t len = sizeof(int);
#ifdef OS_WINDOWS
	return getsockopt(socket, SOL_SOCKET, option, reinterpret_cast<char *>(&value), &len) == 0;
#else
	return getsockopt(socket, SOL_SOCKET, option, &value, &len) == 0;
#endif
}

bool mint::set_socket_option(int socket, int option, int value) {
	return setsockopt(socket, SOL_SOCKET, option, reinterpret_cast<const char *>(&value), sizeof(value)) == 0;
}

MINT_FUNCTION(mint_socket_is_non_blocking, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference socket = move(helper.popParameter());

	bool status = false;
	int socket_fd = to_integer(cursor, socket);

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
	WeakReference enabled = move(helper.popParameter());
	WeakReference socket = move(helper.popParameter());

	bool success = false;
	int socket_fd = to_integer(cursor, socket);

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

MINT_FUNCTION(mint_socket_finalize_connexion, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference socket = move(helper.popParameter());

	int error = 1;

	if (get_socket_option(to_integer(cursor, socket), SO_ERROR, &error)) {
		helper.returnValue(create_number(error));
	}
	else {
		helper.returnValue(create_number(errno));
	}
}

MINT_FUNCTION(mint_socket_close, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference socket = move(helper.popParameter());

	int socket_fd = to_integer(cursor, socket);
#ifdef OS_WINDOWS
	shutdown(socket_fd, SD_BOTH);
#else
	shutdown(socket_fd, SHUT_RDWR);
#endif
	Scheduler::instance().closeSocket(socket_fd);
}

MINT_FUNCTION(mint_socket_get_error, 1, cursor) {
	FunctionHelper helper(cursor, 1);
	Reference &socket = helper.popParameter();
	int error = 0;
	if (get_socket_option(to_integer(cursor, socket), SO_ERROR, &error)) {
		helper.returnValue(create_number(error));
	}
	else {
		helper.returnValue(create_number(errno));
	}
}

MINT_FUNCTION(mint_socket_strerror, 1, cursor) {
	FunctionHelper helper(cursor, 1);
	Reference &error = helper.popParameter();
	helper.returnValue(create_string(strerror(to_integer(cursor, error))));
}
