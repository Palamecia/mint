#ifndef NETWORK_SCHEDULER_H
#define NETWORK_SCHEDULER_H

#include <config.h>
#include <vector>
#include <map>

#ifdef OS_UNIX
#include <poll.h>
#endif

#ifdef OS_WINDOWS
enum PollEvent {
	POLLIN   = 0x0001,
	POLLPRI  = 0x0002,
	POLLOUT  = 0x0004,
	POLLERR  = 0x0008,
	POLLHUP  = 0x0010,
	POLLNVAL = 0x0020
};

struct pollfd {
	int fd;
	short events;
	short revents;
	WSAEVENT handle;
};
#endif

static const int sockopt_false = 0;
static const int sockopt_true = 1;

class Scheduler {
public:
	static Scheduler &instance();

	int openSocket(int domain, int type, int protocol);
	void closeSocket(int fd);

	bool isSocketBlocket(int fd) const;
	void setSocketBlocked(int fd, bool blocked);

	int poll(std::vector<pollfd> &fdset, int timeout);

private:
	Scheduler();
	~Scheduler();

	std::map<int, bool> m_sockets;
};

#endif // NETWORK_SCHEDULER_H
