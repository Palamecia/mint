#ifndef NETWORK_SCHEDULER_H
#define NETWORK_SCHEDULER_H

#include <config.h>
#include <vector>
#include <unordered_map>

#ifdef OS_WINDOWS
#include <WinSock2.h>
using handle_t = WSAEVENT;
using socklen_t = int;
#else
#ifdef OS_UNIX
#include <poll.h>
#endif
#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif
using handle_t = int;
using SOCKET = int;
#endif

struct PollFd {
	enum Event {
		read   = 0x0001,
		write  = 0x0002,
		accept = 0x0004,
		error  = 0x0008,
		close  = 0x0010
	};

	SOCKET fd;
	short events;
	short revents;
	handle_t handle;
};

class Scheduler {
public:
	class Error {
	public:
		Error(bool status);
		Error(const Error &other) noexcept;

		Error &operator =(const Error &other) noexcept;

		operator bool() const;
		int getErrno() const;

	private:
		Error(bool _status, int _errno);

		bool m_status;
		int m_errno;
	};


	static Scheduler &instance();

	SOCKET openSocket(int domain, int type, int protocol);
	void acceptSocket(SOCKET fd);
	Error closeSocket(SOCKET fd);

	bool isSocketListening(SOCKET fd) const;
	void setSocketListening(SOCKET fd, bool listening);

	bool isSocketBlocking(SOCKET fd) const;
	void setSocketBlocking(SOCKET fd, bool blocking);

	bool isSocketBlocked(SOCKET fd) const;
	void setSocketBlocked(SOCKET fd, bool blocked);

	bool poll(std::vector<PollFd> &fdset, int timeout);

private:
	Scheduler();
	~Scheduler();

	struct socket_infos {
		bool blocked : 1;
		bool blocking : 1;
		bool listening : 1;
	};
	std::unordered_map<SOCKET, socket_infos> m_sockets;
};

int errno_from_io_last_error();

#endif // NETWORK_SCHEDULER_H
