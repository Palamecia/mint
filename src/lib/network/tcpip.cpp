#include <memory/functiontool.h>
#include <memory/casttool.h>
#include <memory/builtin/string.h>

#ifdef OS_WINDOWS
#include <WinSock2.h>
#else
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <linux/sockios.h>
#endif

#include "scheduler.h"
#include <algorithm>

using namespace std;
using namespace mint;

namespace symbols {

static const Symbol Network("Network");
static const Symbol EndPoint("EndPoint");
static const Symbol IOStatus("IOStatus");
static const Symbol io_success("io_success");
static const Symbol io_would_block("io_would_block");
static const Symbol io_closed("io_closed");
static const Symbol io_error("io_error");

}

MINT_FUNCTION(mint_tcp_ip_socket_open, 0, cursor) {

	FunctionHelper helper(cursor, 0);
	int socket_fd = Scheduler::instance().openSocket(AF_INET, SOCK_STREAM, 0);

	if (socket_fd != -1) {
		setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&sockopt_true), sizeof(sockopt_true));
		helper.returnValue(create_number(socket_fd));
	}
}

MINT_FUNCTION(mint_tcp_ip_socket_close, 1, cursor) {

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

MINT_FUNCTION(mint_tcp_ip_socket_bind, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	WeakReference port = move(helper.popParameter());
	WeakReference address = move(helper.popParameter());
	WeakReference socket = move(helper.popParameter());

	sockaddr_in serv_addr;
	int socket_fd = to_integer(cursor, socket);
	string address_str = to_string(address);

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(address_str.c_str());
	serv_addr.sin_port = htons(static_cast<uint16_t>(to_number(cursor, port)));

	helper.returnValue(create_boolean(::bind(socket_fd, reinterpret_cast<sockaddr *>(&serv_addr), sizeof(serv_addr)) == 0));
}

MINT_FUNCTION(mint_tcp_ip_socket_listen, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference backlog = move(helper.popParameter());
	WeakReference socket = move(helper.popParameter());

	int socket_fd = to_integer(cursor, socket);

	Scheduler::instance().setSocketListening(socket_fd, true);
	helper.returnValue(create_boolean(listen(socket_fd, static_cast<int>(to_integer(cursor, backlog))) == 0));
}

MINT_FUNCTION(mint_tcp_ip_socket_connect, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	WeakReference port = move(helper.popParameter());
	WeakReference address = move(helper.popParameter());
	WeakReference socket = move(helper.popParameter());

	sockaddr_in target;
	int socket_fd = to_integer(cursor, socket);
	string address_str = to_string(address);

	memset(&target, 0, sizeof(target));
	target.sin_family = AF_INET;
	target.sin_addr.s_addr = inet_addr(address_str.c_str());
	target.sin_port = htons(static_cast<uint16_t>(to_number(cursor, port)));

	Scheduler::instance().setSocketListening(socket_fd, false);
	helper.returnValue(create_boolean(connect(socket_fd, reinterpret_cast<sockaddr *>(&target), sizeof(target)) == 0));
}

MINT_FUNCTION(mint_tcp_ip_socket_accept, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference socket = move(helper.popParameter());

	WeakReference result(Reference::standard);

	sockaddr cli_addr;
	socklen_t cli_len = sizeof(cli_addr);
	int socket_fd = to_integer(cursor, socket);

	int client_fd = accept(socket_fd, &cli_addr, &cli_len);

	if (client_fd != -1) {
		switch (cli_addr.sa_family) {
		case AF_INET:
			result = create_iterator();
			iterator_insert(result.data<Iterator>(), create_number(client_fd));
			iterator_insert(result.data<Iterator>(), create_string(inet_ntoa(reinterpret_cast<sockaddr_in *>(&cli_addr)->sin_addr)));
			iterator_insert(result.data<Iterator>(), create_number(htons(reinterpret_cast<sockaddr_in *>(&cli_addr)->sin_port)));
			helper.returnValue(move(result));
			break;

		default:
			break;
		}
	}
}

MINT_FUNCTION(mint_tcp_ip_socket_send, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference buffer = move(helper.popParameter());
	WeakReference socket = move(helper.popParameter());
	WeakReference result = create_iterator();

	int socket_fd = to_integer(cursor, socket);
	vector<uint8_t> *buf = buffer.data<LibObject<vector<uint8_t>>>()->impl;
	auto IOStatus = helper.reference(symbols::Network).member(symbols::EndPoint).member(symbols::IOStatus);

#ifdef OS_WINDOWS
	int flags = 0;
#else
	int flags = MSG_NOSIGNAL;
#endif

	auto count = send(socket_fd, reinterpret_cast<const char *>(buf->data()), buf->size(), flags);

	switch (count) {
	case -1:
		switch (int error = errno_from_io_last_error()) {
		case EWOULDBLOCK:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::io_would_block));
			Scheduler::instance().setSocketBlocked(socket_fd, true);
			break;

		case EPIPE:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::io_closed));
			break;

		default:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::io_error));
			iterator_insert(result.data<Iterator>(), create_number(error));
			break;
		}
		break;
	case 0:
		iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::io_closed));
		break;
	default:
		iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::io_success));
		iterator_insert(result.data<Iterator>(), create_number(count));
		break;
	}

	helper.returnValue(move(result));
}

MINT_FUNCTION(mint_tcp_ip_socket_recv, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference buffer = move(helper.popParameter());
	WeakReference socket = move(helper.popParameter());
	WeakReference result = create_iterator();

	int length = 0;
	int socket_fd = to_integer(cursor, socket);
	vector<uint8_t> *buf = buffer.data<LibObject<vector<uint8_t>>>()->impl;
	auto IOStatus = helper.reference(symbols::Network).member(symbols::EndPoint).member(symbols::IOStatus);

#ifdef OS_UNIX
	if (ioctl(socket_fd, SIOCINQ, &length) != -1) {
#else
	length = BUFSIZ; /// @todo get better value
#endif

		unique_ptr<uint8_t []> local_buffer(new uint8_t [length]);
		auto count = recv(socket_fd, reinterpret_cast<char *>(local_buffer.get()), static_cast<size_t>(length), 0);

		switch (count) {
		case -1:
			switch (int error = errno_from_io_last_error()) {
			case EWOULDBLOCK:
				iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::io_would_block));
				Scheduler::instance().setSocketBlocked(socket_fd, true);
				break;

			case EPIPE:
				iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::io_closed));
				break;

			default:
				iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::io_error));
				iterator_insert(result.data<Iterator>(), create_number(error));
				break;
			}
			break;
		case 0:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::io_closed));
			break;
		default:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::io_success));
			copy_n(local_buffer.get(), count, back_inserter(*buf));
			break;
		}
#ifdef OS_UNIX
	}
	else {
		iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::io_error));
		iterator_insert(result.data<Iterator>(), create_number(errno));
	}
#endif

	helper.returnValue(move(result));
}

MINT_FUNCTION(mint_tcp_ip_socket_is_non_blocking, 1, cursor) {

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

MINT_FUNCTION(mint_tcp_ip_socket_set_non_blocking, 2, cursor) {

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
#else
	int value = static_cast<int>(to_boolean(cursor, enabled));

	if (ioctl(socket_fd, FIONBIO, &value) != -1) {
		success = true;
	}
#endif

	if (success) {
		Scheduler::instance().setSocketBlocking(socket_fd, static_cast<bool>(value));
	}

	helper.returnValue(create_boolean(success));
}

MINT_FUNCTION(mint_tcp_ip_socket_finalize_connexion, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference socket = move(helper.popParameter());

	socklen_t len = 0;
	int so_error = 1;
	int socket_fd = to_integer(cursor, socket);

#ifdef OS_WINDOWS
	getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&so_error), &len);
#else
	getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &so_error, &len);
#endif
	helper.returnValue(create_boolean(so_error == 0));
}
