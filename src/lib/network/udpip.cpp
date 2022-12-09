#include <memory/functiontool.h>
#include <memory/casttool.h>
#include <memory/builtin/string.h>

#ifdef OS_WINDOWS
#include <ws2tcpip.h>
#else
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

MINT_FUNCTION(mint_udp_ip_socket_open, 1, cursor) {

	FunctionHelper helper(cursor, 1);
	Reference &ip_version = helper.popParameter();
	WeakReference result = create_iterator();

	SOCKET socket_fd = -1;

	switch (to_integer(cursor, ip_version)) {
	case 4:
		socket_fd = Scheduler::instance().openSocket(AF_INET, SOCK_DGRAM, 0);
		break;
	case 6:
		socket_fd = Scheduler::instance().openSocket(AF_INET6, SOCK_DGRAM, 0);
		break;
	default:
		iterator_insert(result.data<Iterator>(), WeakReference::create<None>());
		iterator_insert(result.data<Iterator>(), create_number(EOPNOTSUPP));
		helper.returnValue(std::move(result));
		return;
	}

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
		iterator_insert(result.data<Iterator>(), create_number(errno_from_io_last_error()));
	}

	helper.returnValue(std::move(result));
}

MINT_FUNCTION(mint_udp_ip_socket_bind, 4, cursor) {

	FunctionHelper helper(cursor, 4);
	Reference &ip_version = helper.popParameter();
	Reference &port = helper.popParameter();
	Reference &address = helper.popParameter();
	Reference &socket = helper.popParameter();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const string address_str = to_string(address);

	unique_ptr<sockaddr> serv_addr;
	socklen_t length = sizeof(sockaddr);

	switch (to_integer(cursor, ip_version)) {
	case 4:
		length = sizeof(sockaddr_in);
		serv_addr.reset(reinterpret_cast<sockaddr *>(new sockaddr_in));
		memset(serv_addr.get(), 0, length);
		reinterpret_cast<sockaddr_in *>(serv_addr.get())->sin_family = AF_INET;
		reinterpret_cast<sockaddr_in *>(serv_addr.get())->sin_port = htons(static_cast<uint16_t>(to_integer(cursor, port)));
		switch (::inet_pton(AF_INET, address_str.c_str(), &reinterpret_cast<sockaddr_in *>(serv_addr.get())->sin_addr.s_addr)) {
		case 0:
			helper.returnValue(create_number(EINVAL));
			return;
		case 1:
			break;
		default:
			helper.returnValue(create_number(errno_from_io_last_error()));
			return;
		}
		break;
	case 6:
		length = sizeof(sockaddr_in6);
		serv_addr.reset(reinterpret_cast<sockaddr *>(new sockaddr_in6));
		memset(serv_addr.get(), 0, length);
		reinterpret_cast<sockaddr_in6 *>(serv_addr.get())->sin6_family = AF_INET6;
		reinterpret_cast<sockaddr_in6 *>(serv_addr.get())->sin6_port = htons(static_cast<uint16_t>(to_integer(cursor, port)));
		switch (::inet_pton(AF_INET6, address_str.c_str(), &reinterpret_cast<sockaddr_in6 *>(serv_addr.get())->sin6_addr.s6_addr)) {
		case 0:
			helper.returnValue(create_number(EINVAL));
			return;
		case 1:
			break;
		default:
			helper.returnValue(create_number(errno_from_io_last_error()));
			return;
		}
		break;
	default:
		helper.returnValue(create_number(EOPNOTSUPP));
		return;
	}

	if (::bind(socket_fd, serv_addr.get(), length) != 0) {
		helper.returnValue(create_number(errno_from_io_last_error()));
	}
}

MINT_FUNCTION(mint_udp_ip_socket_connect, 4, cursor) {

	FunctionHelper helper(cursor, 4);
	Reference &ip_version = helper.popParameter();
	Reference &port = helper.popParameter();
	Reference &address = helper.popParameter();
	Reference &socket = helper.popParameter();
	WeakReference result = create_iterator();

	const SOCKET socket_fd = to_integer(cursor, socket);
	const string address_str = to_string(address);
	auto IOStatus = helper.reference(symbols::Network).member(symbols::EndPoint).member(symbols::IOStatus);

	unique_ptr<sockaddr> target;
	socklen_t length = sizeof(sockaddr);

	switch (to_integer(cursor, ip_version)) {
	case 4:
		length = sizeof(sockaddr_in);
		target.reset(reinterpret_cast<sockaddr *>(new sockaddr_in));
		memset(target.get(), 0, length);
		reinterpret_cast<sockaddr_in *>(target.get())->sin_family = AF_INET;
		reinterpret_cast<sockaddr_in *>(target.get())->sin_port = htons(static_cast<uint16_t>(to_integer(cursor, port)));
		switch (::inet_pton(AF_INET, address_str.c_str(), &reinterpret_cast<sockaddr_in *>(target.get())->sin_addr.s_addr)) {
		case 0:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOError));
			iterator_insert(result.data<Iterator>(), create_number(EINVAL));
			helper.returnValue(std::move(result));
			return;
		case 1:
			break;
		default:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOError));
			iterator_insert(result.data<Iterator>(), create_number(errno_from_io_last_error()));
			helper.returnValue(std::move(result));
			return;
		}
		break;
	case 6:
		length = sizeof(sockaddr_in6);
		target.reset(reinterpret_cast<sockaddr *>(new sockaddr_in6));
		memset(target.get(), 0, length);
		reinterpret_cast<sockaddr_in6 *>(target.get())->sin6_family = AF_INET6;
		reinterpret_cast<sockaddr_in6 *>(target.get())->sin6_port = htons(static_cast<uint16_t>(to_integer(cursor, port)));
		switch (::inet_pton(AF_INET6, address_str.c_str(), &reinterpret_cast<sockaddr_in6 *>(target.get())->sin6_addr.s6_addr)) {
		case 0:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOError));
			iterator_insert(result.data<Iterator>(), create_number(EINVAL));
			helper.returnValue(std::move(result));
			return;
		case 1:
			break;
		default:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOError));
			iterator_insert(result.data<Iterator>(), create_number(errno_from_io_last_error()));
			helper.returnValue(std::move(result));
			return;
		}
		break;
	default:
		iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOError));
		iterator_insert(result.data<Iterator>(), create_number(EOPNOTSUPP));
		helper.returnValue(std::move(result));
		return;
	}

	Scheduler::instance().setSocketListening(socket_fd, false);

	if (::connect(socket_fd, target.get(), length) == 0) {
		iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOSuccess));
	}
	else {
		switch (const int error = errno_from_io_last_error()) {
		case EINPROGRESS:
		case EWOULDBLOCK:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOWouldBlock));
			break;
		default:
			iterator_insert(result.data<Iterator>(), IOStatus.member(symbols::IOError));
			iterator_insert(result.data<Iterator>(), create_number(error));
			break;
		}
	}

	helper.returnValue(std::move(result));
}

MINT_FUNCTION(mint_udp_ip_socket_send, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference buffer = std::move(helper.popParameter());
	WeakReference socket = std::move(helper.popParameter());
	WeakReference result = create_iterator();

	SOCKET socket_fd = to_integer(cursor, socket);
	vector<uint8_t> *buf = buffer.data<LibObject<vector<uint8_t>>>()->impl;
	auto IOStatus = helper.reference(symbols::Network).member(symbols::EndPoint).member(symbols::IOStatus);

#ifdef OS_WINDOWS
	int flags = 0;
#else
	int flags = MSG_CONFIRM;
#endif

	auto count = send(socket_fd, reinterpret_cast<const char *>(buf->data()), buf->size(), flags);

	switch (count) {
	case -1:
		switch (const int error = errno_from_io_last_error()) {
		case EINPROGRESS:
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

	helper.returnValue(std::move(result));
}

MINT_FUNCTION(mint_udp_ip_socket_recv, 2, cursor) {

	FunctionHelper helper(cursor, 2);
	WeakReference buffer = std::move(helper.popParameter());
	WeakReference socket = std::move(helper.popParameter());
	WeakReference result = create_iterator();

	socklen_t length = 0;
	SOCKET socket_fd = to_integer(cursor, socket);
	vector<uint8_t> *buf = buffer.data<LibObject<vector<uint8_t>>>()->impl;
	auto IOStatus = helper.reference(symbols::Network).member(symbols::EndPoint).member(symbols::IOStatus);

#ifdef OS_UNIX
	if (ioctl(socket_fd, SIOCINQ, &length) != -1) {
#else
	length = BUFSIZ; /// @todo get better value
#endif

		int flags = MSG_WAITALL;
		unique_ptr<uint8_t []> local_buffer(new uint8_t [length]);
		auto count = recv(socket_fd, reinterpret_cast<char *>(local_buffer.get()), static_cast<size_t>(length), flags);

		switch (count) {
		case -1:
			switch (const int error = errno_from_io_last_error()) {
			case EINPROGRESS:
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

	helper.returnValue(std::move(result));
}
