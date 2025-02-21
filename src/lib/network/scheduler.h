/**
 * Copyright (c) 2025 Gauvain CHERY.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef MINT_NETWORK_SCHEDULER_H
#define MINT_NETWORK_SCHEDULER_H

#include <mint/config.h>
#include <unordered_map>
#include <cstdint>
#include <vector>

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
	enum Event : std::uint8_t {
		READ_EVENT = 0x01,
		WRITE_EVENT = 0x02,
		ACCEPT_EVENT = 0x04,
		ERROR_EVENT = 0x08,
		CLOSE_EVENT = 0x10
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
		Error(const Error &other) noexcept = default;
		Error(Error &&) = delete;
		~Error() = default;

		Error &operator=(const Error &other) noexcept = default;
		Error &operator=(Error &&) = delete;

		operator bool() const;
		[[nodiscard]] int get_errno() const;

	private:
		Error(bool status, int errno_value);

		bool m_status;
		int m_errno;
	};

	Scheduler(const Scheduler &) = delete;
	Scheduler(Scheduler &&) = delete;

	Scheduler &operator=(const Scheduler &) = delete;
	Scheduler &operator=(Scheduler &&) = delete;

	static Scheduler &instance();

	[[nodiscard]] SOCKET open_socket(int domain, int type, int protocol);
	void accept_socket(SOCKET fd);
	Error close_socket(SOCKET fd);

	[[nodiscard]] bool is_socket_listening(SOCKET fd) const;
	void set_socket_listening(SOCKET fd, bool listening);

	[[nodiscard]] bool is_socket_blocking(SOCKET fd) const;
	void set_socket_blocking(SOCKET fd, bool blocking);

	[[nodiscard]] bool is_socket_blocked(SOCKET fd) const;
	void set_socket_blocked(SOCKET fd, bool blocked);

	bool poll(std::vector<PollFd> &fdset, int timeout);

private:
	Scheduler();
	~Scheduler();

	struct SocketInfo {
		bool blocked: 1;
		bool blocking: 1;
		bool listening: 1;
	};

	std::unordered_map<SOCKET, SocketInfo> m_sockets;
};

int errno_from_io_last_error();

#endif // MINT_NETWORK_SCHEDULER_H
