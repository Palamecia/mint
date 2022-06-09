#ifndef NETWORK_SCHEDULER_H
#define NETWORK_SCHEDULER_H

#include <config.h>
#include <vector>
#include <map>

#ifdef OS_UNIX
#include <poll.h>
#endif

#ifdef OS_WINDOWS
#include <WinSock2.h>
using handle_t = WSAEVENT;
using socklen_t = int;
#else
using handle_t = int;
#endif

struct PollFd {
	enum Event {
		read   = 0x0001,
		write  = 0x0002,
		accept = 0x0004,
		error  = 0x0008,
		close  = 0x0010
	};

	int fd;
	short events;
	short revents;
	handle_t handle;
};

static const int sockopt_false = 0;
static const int sockopt_true = 1;

class Scheduler {
public:
	static Scheduler &instance();

	int openSocket(int domain, int type, int protocol);
	void closeSocket(int fd);

	bool isSocketListening(int fd) const;
	void setSocketListening(int fd, bool listening);

	bool isSocketBlocking(int fd) const;
	void setSocketBlocking(int fd, bool blocking);

	bool isSocketBlocked(int fd) const;
	void setSocketBlocked(int fd, bool blocked);

	bool poll(std::vector<PollFd> &fdset, int timeout);

private:
	Scheduler();
	~Scheduler();

	struct socket_infos {
		bool blocked;
		bool blocking;
		bool listening;
	};
	std::map<int, socket_infos> m_sockets;
};

int errno_from_io_last_error();

#endif // NETWORK_SCHEDULER_H
