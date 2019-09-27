#include <memory/functiontool.h>
#include <memory/casttool.h>
#include <memory/builtin/string.h>

#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <linux/sockios.h>

#include "scheduler.h"
#include <algorithm>

using namespace std;
using namespace mint;

MINT_FUNCTION(mint_udp_ip_socket_open, 0, cursor) {

	FunctionHelper helper(cursor, 0);
	SharedReference socket = SharedReference::unique(Reference::create<LibObject<int>>());

	int socket_fd = Scheduler::instance().openSocket(AF_INET, SOCK_DGRAM, 0);

	if (socket_fd != -1) {
		setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &sockopt_true, sizeof(sockopt_true));
		helper.returnValue(create_number(socket_fd));
	}
}

MINT_FUNCTION(mint_udp_ip_socket_close, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference socket = helper.popParameter();

	int socket_fd = static_cast<int>(socket->data<Number>()->value);
	Scheduler::instance().closeSocket(socket_fd);
}

MINT_FUNCTION(mint_udp_ip_socket_bind, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	SharedReference port = helper.popParameter();
	SharedReference address = helper.popParameter();
	SharedReference socket = helper.popParameter();

	sockaddr_in serv_addr;
	int socket_fd = static_cast<int>(socket->data<Number>()->value);

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(to_string(address).c_str());
	serv_addr.sin_port = htons(static_cast<uint16_t>(to_number(cursor, port)));

	helper.returnValue(create_boolean(bind(socket_fd, reinterpret_cast<sockaddr *>(&serv_addr), sizeof(serv_addr)) == 0));
}

MINT_FUNCTION(mint_udp_ip_socket_connect, 3, cursor) {

	FunctionHelper helper(cursor, 3);
	SharedReference port = helper.popParameter();
	SharedReference address = helper.popParameter();
	SharedReference socket = helper.popParameter();

	sockaddr_in target;
	int socket_fd = static_cast<int>(socket->data<Number>()->value);

	memset(&target, 0, sizeof(target));
	target.sin_family = AF_INET;
	target.sin_addr.s_addr = inet_addr(to_string(address).c_str());
	target.sin_port = htons(static_cast<uint16_t>(to_number(cursor, port)));

	helper.returnValue(create_boolean(connect(socket_fd, reinterpret_cast<sockaddr *>(&target), sizeof(target)) == 0));
}

MINT_FUNCTION(mint_udp_ip_socket_send, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference buffer = helper.popParameter();
	SharedReference socket = helper.popParameter();

	int socket_fd = static_cast<int>(socket->data<Number>()->value);
	vector<uint8_t> *buf = buffer->data<LibObject<vector<uint8_t>>>()->impl;
	auto IOStatus = helper.reference("Network").member("EndPoint").member("IOStatus");

	auto count = send(socket_fd, buf->data(), buf->size(), MSG_CONFIRM);

	if (count != -1) {
		SharedReference result = SharedReference::unique(Reference::create<Iterator>());
		result->data<Iterator>()->construct();
		iterator_insert(result->data<Iterator>(), IOStatus.member("io_success"));
		iterator_insert(result->data<Iterator>(), create_number(count));
		helper.returnValue(result);
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

MINT_FUNCTION(mint_udp_ip_socket_recv, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference buffer = helper.popParameter();
	SharedReference socket = helper.popParameter();

	int length = 0;
	int socket_fd = static_cast<int>(socket->data<Number>()->value);
	vector<uint8_t> *buf = buffer->data<LibObject<vector<uint8_t>>>()->impl;
	auto IOStatus = helper.reference("Network").member("EndPoint").member("IOStatus");

	if (ioctl(socket_fd, SIOCINQ, &length) != -1) {

		unique_ptr<uint8_t []> local_buffer(new uint8_t [length]);

		auto count = recv(socket_fd, local_buffer.get(), static_cast<size_t>(length), MSG_WAITALL);
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
	}
	else {
		helper.returnValue(IOStatus.member("io_error"));
	}
}

MINT_FUNCTION(mint_udp_ip_socket_is_non_blocking, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference socket = helper.popParameter();

	int flags = 0;
	bool status = false;
	int socket_fd = static_cast<int>(socket->data<Number>()->value);

	if ((flags = fcntl(socket_fd, F_GETFL, 0)) != -1) {
		status = flags & O_NONBLOCK;
	}

	helper.returnValue(create_boolean(status));
}

MINT_FUNCTION(mint_udp_ip_socket_set_non_blocking, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	SharedReference enabled = helper.popParameter();
	SharedReference socket = helper.popParameter();

	int flags = 0;
	bool success = false;
	int socket_fd = static_cast<int>(socket->data<Number>()->value);

	if ((flags = fcntl(socket_fd, F_GETFL, 0)) != -1) {
		if (to_boolean(cursor, enabled)) {
			if (fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK) != -1) {
				success = true;
			}
		}
		else {
			if (fcntl(socket_fd, F_SETFL, flags & ~O_NONBLOCK) != -1) {
				success = true;
			}
		}
	}

	helper.returnValue(create_boolean(success));
}

MINT_FUNCTION(mint_udp_ip_socket_finalize_connexion, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	SharedReference socket = helper.popParameter();

	socklen_t len = 0;
	int so_error = 1;
	int socket_fd = static_cast<int>(socket->data<Number>()->value);

	getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &so_error, &len);
	helper.returnValue(create_boolean(so_error == 0));
}
