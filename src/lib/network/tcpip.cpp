#include <memory/functiontool.h>
#include <memory/casttool.h>
#include <memory/builtin/string.h>

#ifdef OS_WINDOWS
#include <ws2tcpip.h>
#else
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <linux/sockios.h>
#endif

#include "scheduler.h"
#include "socket.h"
#include <algorithm>

using namespace std;
using namespace mint;

namespace symbols {

static const Symbol Network("Network");
static const Symbol EndPoint("EndPoint");
static const Symbol IOStatus("IOStatus");
static const Symbol IOSuccess("IOSuccess");
static const Symbol IOWouldBlock("IOWouldBlock");
static const Symbol IOClosed("IOClosed");
static const Symbol IOError("IOError");

}

MINT_FUNCTION(mint_tcp_ip_socket_open, 0, cursor) {

	FunctionHelper helper(cursor, 0);
	int socket_fd = Scheduler::instance().openSocket(AF_INET, SOCK_STREAM, 0);
	WeakReference result = create_iterator();

	if (socket_fd != -1) {
		iterator_insert(result.data<Iterator>(), create_number(socket_fd));
		if (set_socket_option(socket_fd, SO_REUSEADDR, sockopt_true)) {
			iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
		}
		else {
			iterator_insert(result.data<Iterator>(), create_number(errno));
		}
	}
	else {
		iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
#ifdef OS_WINDOWS
		iterator_insert(result.data<Iterator>(), create_number(errno_from_io_last_error()));
#else
		iterator_insert(result.data<Iterator>(), create_number(errno));
#endif
	}

	helper.returnValue(move(result));
}

MINT_FUNCTION(mint_tcp_ip_socket_bind, 4, cursor) {

	FunctionHelper helper(cursor, 4);
	WeakReference ip_version = move(helper.popParameter());
	WeakReference port = move(helper.popParameter());
	WeakReference address = move(helper.popParameter());
	WeakReference socket = move(helper.popParameter());

	sockaddr_in serv_addr;
	int socket_fd = to_integer(cursor, socket);
	string address_str = to_string(address);

	memset(&serv_addr, 0, sizeof(serv_addr));

	switch (to_integer(cursor, ip_version)) {
	case 4:
	{
		struct in_addr dst;
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = inet_pton(AF_INET, address_str.c_str(), &dst);
		serv_addr.sin_port = htons(static_cast<uint16_t>(to_integer(cursor, port)));
	}
		break;
	case 6:
	{
		struct in6_addr dst;
		serv_addr.sin_family = AF_INET6;
		serv_addr.sin_addr.s_addr = inet_pton(AF_INET6, address_str.c_str(), &dst);
		serv_addr.sin_port = htons(static_cast<uint16_t>(to_integer(cursor, port)));
	}
		break;
	default:
		return;
	}

	if (::bind(socket_fd, reinterpret_cast<sockaddr *>(&serv_addr), sizeof(serv_addr)) != 0) {
#ifdef OS_WINDOWS
		helper.returnValue(create_number(errno_from_io_last_error()));
#else
		helper.returnValue(create_number(errno));
#endif
	}
}

MINT_FUNCTION(mint_tcp_ip_socket_listen, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference backlog = move(helper.popParameter());
	WeakReference socket = move(helper.popParameter());

	int socket_fd = to_integer(cursor, socket);

	Scheduler::instance().setSocketListening(socket_fd, true);
	if (::listen(socket_fd, static_cast<int>(to_integer(cursor, backlog))) != 0) {
#ifdef OS_WINDOWS
		helper.returnValue(create_number(errno_from_io_last_error()));
#else
		helper.returnValue(create_number(errno));
#endif
	}
}

MINT_FUNCTION(mint_tcp_ip_socket_connect, 4, cursor) {

	FunctionHelper helper(cursor, 4);
	WeakReference ip_version = move(helper.popParameter());
	WeakReference port = move(helper.popParameter());
	WeakReference address = move(helper.popParameter());
	WeakReference socket = move(helper.popParameter());

	sockaddr_in target;
	int socket_fd = to_integer(cursor, socket);
	string address_str = to_string(address);

	memset(&target, 0, sizeof(target));

	switch (to_integer(cursor, ip_version)) {
	case 4:
	{
		struct in_addr dst;
		target.sin_family = AF_INET;
		target.sin_addr.s_addr = inet_pton(AF_INET, address_str.c_str(), &dst);
		target.sin_port = htons(static_cast<uint16_t>(to_integer(cursor, port)));
	}
		break;
	case 6:
	{
		struct in6_addr dst;
		target.sin_family = AF_INET6;
		target.sin_addr.s_addr = inet_pton(AF_INET6, address_str.c_str(), &dst);
		target.sin_port = htons(static_cast<uint16_t>(to_integer(cursor, port)));
	}
		break;
	default:
		return;
	}

	Scheduler::instance().setSocketListening(socket_fd, false);

	if (connect(socket_fd, reinterpret_cast<sockaddr *>(&target), sizeof(target)) != 0) {
#ifdef OS_WINDOWS
		helper.returnValue(create_number(errno_from_io_last_error()));
#else
		helper.returnValue(create_number(errno));
#endif
	}
}

MINT_FUNCTION(mint_tcp_ip_socket_accept, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	WeakReference socket = move(helper.popParameter());
	WeakReference result = create_iterator();

	sockaddr cli_addr;
	socklen_t cli_len = sizeof(cli_addr);
	int socket_fd = to_integer(cursor, socket);

	int client_fd = accept(socket_fd, &cli_addr, &cli_len);

	if (client_fd != -1) {
		switch (cli_addr.sa_family) {
		case AF_INET:
		{
			char buffer[INET_ADDRSTRLEN];
			sockaddr_in *client = reinterpret_cast<sockaddr_in *>(&cli_addr);
			if (const char *address = inet_ntop(cli_addr.sa_family, &client->sin_addr, buffer, sizeof(buffer))) {
				iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
				iterator_insert(result.data<Iterator>(), create_number(client_fd));
				iterator_insert(result.data<Iterator>(), create_string(address));
				iterator_insert(result.data<Iterator>(), create_number(htons(client->sin_port)));
			}
		}
			break;
		case AF_INET6:
		{
			char buffer[INET6_ADDRSTRLEN];
			sockaddr_in *client = reinterpret_cast<sockaddr_in *>(&cli_addr);
			if (const char *address = inet_ntop(cli_addr.sa_family, &client->sin_addr, buffer, sizeof(buffer))) {
				iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
				iterator_insert(result.data<Iterator>(), create_number(client_fd));
				iterator_insert(result.data<Iterator>(), create_string(address));
				iterator_insert(result.data<Iterator>(), create_number(htons(client->sin_port)));
			}
		}
			break;
		default:
			iterator_insert(result.data<Iterator>(), create_number(EOPNOTSUPP));
			break;
		}
	}
	else {
#ifdef OS_WINDOWS
		helper.returnValue(create_number(errno_from_io_last_error()));
#else
		helper.returnValue(create_number(errno));
#endif
	}

	helper.returnValue(move(result));
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
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOWouldBlock));
			Scheduler::instance().setSocketBlocked(socket_fd, true);
			break;

		case EPIPE:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOClosed));
			break;

		default:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOError));
			iterator_insert(result.data<Iterator>(), create_number(error));
			break;
		}
		break;
	case 0:
		iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOClosed));
		break;
	default:
		iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOSuccess));
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
				iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOWouldBlock));
				Scheduler::instance().setSocketBlocked(socket_fd, true);
				break;

			case EPIPE:
				iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOClosed));
				break;

			default:
				iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOError));
				iterator_insert(result.data<Iterator>(), create_number(error));
				break;
			}
			break;
		case 0:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOClosed));
			break;
		default:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOSuccess));
			copy_n(local_buffer.get(), count, back_inserter(*buf));
			break;
		}
#ifdef OS_UNIX
	}
	else {
		iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOError));
		iterator_insert(result.data<Iterator>(), create_number(errno));
	}
#endif

	helper.returnValue(move(result));
}
