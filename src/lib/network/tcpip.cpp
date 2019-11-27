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

MINT_FUNCTION(mint_tcp_ip_socket_open, 0, cursor) {

	FunctionHelper helper(cursor, 0);
	SharedReference socket = SharedReference::unique(Reference::create<LibObject<int>>());

	int socket_fd = Scheduler::instance().openSocket(AF_INET, SOCK_STREAM, 0);

	if (socket_fd != -1) {
		setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&sockopt_true), sizeof(sockopt_true));
		helper.returnValue(create_number(socket_fd));
	}
}

MINT_FUNCTION(mint_tcp_ip_socket_close, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference socket = move(helper.popParameter());

	int socket_fd = static_cast<int>(socket->data<Number>()->value);
#ifdef OS_WINDOWS
	shutdown(socket_fd, SD_BOTH);
#else
	shutdown(socket_fd, SHUT_RDWR);
#endif
	Scheduler::instance().closeSocket(socket_fd);
}

MINT_FUNCTION(mint_tcp_ip_socket_bind, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	SharedReference port = move(helper.popParameter());
	SharedReference address = move(helper.popParameter());
	SharedReference socket = move(helper.popParameter());

	sockaddr_in serv_addr;
	int socket_fd = static_cast<int>(socket->data<Number>()->value);

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(to_string(address).c_str());
	serv_addr.sin_port = htons(static_cast<uint16_t>(to_number(cursor, port)));

	helper.returnValue(create_boolean(bind(socket_fd, reinterpret_cast<sockaddr *>(&serv_addr), sizeof(serv_addr)) == 0));
}

MINT_FUNCTION(mint_tcp_ip_socket_listen, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference backlog = move(helper.popParameter());
	SharedReference socket = move(helper.popParameter());

	int socket_fd = static_cast<int>(socket->data<Number>()->value);

	Scheduler::instance().setSocketListening(socket_fd, true);
	helper.returnValue(create_boolean(listen(socket_fd, static_cast<int>(to_number(cursor, backlog))) == 0));
}

MINT_FUNCTION(mint_tcp_ip_socket_connect, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	SharedReference port = move(helper.popParameter());
	SharedReference address = move(helper.popParameter());
	SharedReference socket = move(helper.popParameter());

	sockaddr_in target;
	int socket_fd = static_cast<int>(socket->data<Number>()->value);

	memset(&target, 0, sizeof(target));
	target.sin_family = AF_INET;
	target.sin_addr.s_addr = inet_addr(to_string(address).c_str());
	target.sin_port = htons(static_cast<uint16_t>(to_number(cursor, port)));

	Scheduler::instance().setSocketListening(socket_fd, false);
	helper.returnValue(create_boolean(connect(socket_fd, reinterpret_cast<sockaddr *>(&target), sizeof(target)) == 0));
}

MINT_FUNCTION(mint_tcp_ip_socket_accept, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference socket = move(helper.popParameter());

	SharedReference result;

	sockaddr cli_addr;
	socklen_t cli_len = sizeof(cli_addr);
	int socket_fd = static_cast<int>(socket->data<Number>()->value);

	int client_fd = accept(socket_fd, &cli_addr, &cli_len);

	if (client_fd != -1) {
		switch (cli_addr.sa_family) {
		case AF_INET:
			result = SharedReference::unique(Reference::create<Iterator>());
			result->data<Iterator>()->construct();
			iterator_insert(result->data<Iterator>(), create_number(client_fd));
			iterator_insert(result->data<Iterator>(), create_string(inet_ntoa(reinterpret_cast<sockaddr_in *>(&cli_addr)->sin_addr)));
			iterator_insert(result->data<Iterator>(), create_number(htons(reinterpret_cast<sockaddr_in *>(&cli_addr)->sin_port)));
			helper.returnValue(move(result));
			break;

		default:
			break;
		}
	}
}

MINT_FUNCTION(mint_tcp_ip_socket_send, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference buffer = move(helper.popParameter());
	SharedReference socket = move(helper.popParameter());

	int socket_fd = static_cast<int>(socket->data<Number>()->value);
	vector<uint8_t> *buf = buffer->data<LibObject<vector<uint8_t>>>()->impl;
	auto IOStatus = helper.reference("Network").member("EndPoint").member("IOStatus");

#ifdef OS_WINDOWS
	int flags = 0;
#else
	int flags = MSG_NOSIGNAL;
#endif

	auto count = send(socket_fd, reinterpret_cast<const char *>(buf->data()), buf->size(), flags);

	if (count != -1) {
		SharedReference result = SharedReference::unique(Reference::create<Iterator>());
		result->data<Iterator>()->construct();
		iterator_insert(result->data<Iterator>(), IOStatus.member("io_success"));
		iterator_insert(result->data<Iterator>(), create_number(count));
		helper.returnValue(move(result));
	}
	else {
		switch (errno) {
		case EWOULDBLOCK:
			Scheduler::instance().setSocketBlocked(socket_fd, true);
			helper.returnValue(IOStatus.member("io_would_block"));
			break;

		case EPIPE:
			helper.returnValue(IOStatus.member("io_closed"));
			break;

		default:
			helper.returnValue(IOStatus.member("io_error"));
			break;
		}
	}
}

MINT_FUNCTION(mint_tcp_ip_socket_recv, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference buffer = move(helper.popParameter());
	SharedReference socket = move(helper.popParameter());

	int length = 0;
	int socket_fd = static_cast<int>(socket->data<Number>()->value);
	vector<uint8_t> *buf = buffer->data<LibObject<vector<uint8_t>>>()->impl;
	auto IOStatus = helper.reference("Network").member("EndPoint").member("IOStatus");

#ifdef OS_UNIX
	if (ioctl(socket_fd, SIOCINQ, &length) != -1) {
#endif

		unique_ptr<uint8_t []> local_buffer(new uint8_t [length]);

		auto count = recv(socket_fd, reinterpret_cast<char *>(local_buffer.get()), static_cast<size_t>(length), 0);
		copy_n(local_buffer.get(), count, back_inserter(*buf));

		if (count != -1) {
			helper.returnValue(IOStatus.member("io_success"));
		}
		else {
			switch (errno) {
			case EWOULDBLOCK:
				Scheduler::instance().setSocketBlocked(socket_fd, true);
				helper.returnValue(IOStatus.member("io_would_block"));
				break;

			case EPIPE:
				helper.returnValue(IOStatus.member("io_closed"));
				break;

			default:
				helper.returnValue(IOStatus.member("io_error"));
				break;
			}
		}
#ifdef OS_UNIX
	}
	else {
		helper.returnValue(IOStatus.member("io_error"));
	}
#endif
}

MINT_FUNCTION(mint_tcp_ip_socket_is_non_blocking, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference socket = move(helper.popParameter());

	bool status = false;
	int socket_fd = static_cast<int>(socket->data<Number>()->value);

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
	SharedReference enabled = move(helper.popParameter());
	SharedReference socket = move(helper.popParameter());

	bool success = false;
	int socket_fd = static_cast<int>(socket->data<Number>()->value);

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
	SharedReference socket = move(helper.popParameter());

	socklen_t len = 0;
	int so_error = 1;
	int socket_fd = static_cast<int>(socket->data<Number>()->value);

#ifdef OS_WINDOWS
	getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, reinterpret_cast<char *>(&so_error), &len);
#else
	getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &so_error, &len);
#endif
	helper.returnValue(create_boolean(so_error == 0));
}
