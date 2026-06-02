/**
 * Copyright (c) 2026 Gauvain CHERY.
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

#ifndef MINT_NETWORK_SOCKET_H
#define MINT_NETWORK_SOCKET_H

#include "mint/ast/abstract_syntax_tree.h"
#include "mint/ast/symbol.h"
#include "mint/config.h"
#include "mint/memory/reference.h"
#include "mint/system/async_io.h"
#include <cstddef>
#include <memory>
#include <system_error>
#include <tuple>
#include <type_traits>

#ifdef MINT_OS_WINDOWS
#include <WinSock2.h>
#include <minwinbase.h>
#include <winnt.h>
using socklen_t = int;
#else
#include <asm-generic/socket.h>
#include <bits/types/struct_timeval.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif
using SOCKET = int;
#endif

#include <unordered_map>

namespace mint_network {

namespace symbols {

static const mint::Symbol network("Network");
static const mint::Symbol socket("Socket");
static const mint::Symbol io_status("IOStatus");
static const mint::Symbol io_success("IOSuccess");
static const mint::Symbol io_would_block("IOWouldBlock");
static const mint::Symbol io_closed("IOClosed");
static const mint::Symbol io_error("IOError");

}

enum sockopt_bool : int { // NOLINT
	sockopt_false = 0,
	sockopt_true = 1
};

class SocketManager {
public:
	SocketManager(const SocketManager&) = delete;
	SocketManager(SocketManager&&) = delete;

	SocketManager& operator=(const SocketManager&) = delete;
	SocketManager& operator=(SocketManager&&) = delete;

	static SocketManager& instance();

	[[nodiscard]] SOCKET open_socket(int domain, int type, int protocol);
	void accept_socket(SOCKET socket);
	void close_socket(SOCKET socket);

	[[nodiscard]] SOCKET open_socket_from_handle(mint::handle_t handle);
	void accept_socket_from_handle(mint::handle_t handle);
	void close_socket_from_handle(mint::handle_t handle);

	[[nodiscard]] bool is_native_socket(SOCKET socket) const;

	[[nodiscard]] bool is_socket_listening(SOCKET socket) const;
	void set_socket_listening(SOCKET socket, bool listening);

	[[nodiscard]] bool is_socket_blocking(SOCKET socket) const;
	void set_socket_blocking(SOCKET socket, bool blocking);

	[[nodiscard]] bool is_socket_blocked(SOCKET socket) const;
	void set_socket_blocked(SOCKET socket, bool blocked);

private:
	SocketManager();
	~SocketManager();

	struct SocketInfo {
		bool native: 1;
		bool blocked: 1;
		bool blocking: 1;
		bool listening: 1;
	};

	std::unordered_map<SOCKET, SocketInfo> _sockets;

#ifdef MINT_OS_WINDOWS
	WSADATA _wsa_data = {};
#endif
};

mint::Reference create_socket(mint::AbstractSyntaxTree& ast, SOCKET socket);

std::tuple<sockaddr*, socklen_t> to_sockaddr(const mint::Reference& reference);

void get_socket_option(SOCKET socket, int level, int option, void* value, socklen_t len);
void set_socket_option(SOCKET socket, int level, int option, const void* value, socklen_t len);

namespace internal {

template<typename T>
struct IsUniquePtr : std::false_type {};

template<typename U, typename D>
struct IsUniquePtr<std::unique_ptr<U, D>> : std::true_type {};

template<typename T>
inline constexpr bool is_unique_ptr_v = IsUniquePtr<T>::value;

}

template<typename opt_t>
inline opt_t get_socket_option(SOCKET socket, int option) {
	if constexpr (internal::is_unique_ptr_v<opt_t>) {
		auto value = std::make_unique<typename opt_t::element_type>();
		get_socket_option(socket, SOL_SOCKET, option, value.get(), sizeof(opt_t));
		return value;
	}
	else {
		auto value = opt_t();
		get_socket_option(socket, SOL_SOCKET, option, &value, sizeof(opt_t));
		return value;
	}
}

template<typename opt_t>
inline void set_socket_option(SOCKET socket, int option, const opt_t& value) {
	set_socket_option(socket, SOL_SOCKET, option, &value, sizeof(opt_t));
}

template<>
inline void set_socket_option<std::nullptr_t>(SOCKET socket, int option, const std::nullptr_t& /*value*/) {
	set_socket_option(socket, SOL_SOCKET, option, nullptr, 0);
}

template<typename opt_t>
inline opt_t get_socket_option(SOCKET socket, int level, int option) {
	if constexpr (internal::is_unique_ptr_v<opt_t>) {
		auto value = std::make_unique<typename opt_t::element_type>();
		get_socket_option(socket, level, option, value.get(), sizeof(opt_t));
		return value;
	}
	else {
		auto value = opt_t();
		get_socket_option(socket, level, option, &value, sizeof(opt_t));
		return value;
	}
}

template<typename opt_t>
inline void set_socket_option(SOCKET socket, int level, int option, const opt_t& value) {
	set_socket_option(socket, level, option, &value, sizeof(opt_t));
}

template<>
inline void set_socket_option<std::nullptr_t>(SOCKET socket, int level, int option, const std::nullptr_t& /*value*/) {
	set_socket_option(socket, level, option, nullptr, 0);
}

void set_socket_non_blocking(SOCKET socket, bool enabled);

template<bool enabled>
class SocketBlockingModeGuard {
	SOCKET _socket;
	bool _was_blocking;
public:
	SocketBlockingModeGuard(SOCKET socket_fd) :
	    _socket(socket_fd),
	    _was_blocking(mint_network::SocketManager::instance().is_socket_blocking(_socket)) {
		set_socket_non_blocking(_socket, !enabled);
	}

	SocketBlockingModeGuard(const SocketBlockingModeGuard&) = delete;
	SocketBlockingModeGuard(SocketBlockingModeGuard&&) = delete;

	~SocketBlockingModeGuard() {
		set_socket_non_blocking(_socket, !_was_blocking);
	}

	SocketBlockingModeGuard& operator=(const SocketBlockingModeGuard&) = delete;
	SocketBlockingModeGuard& operator=(SocketBlockingModeGuard&&) = delete;
};

std::error_code last_socket_error_code();
int errno_from_socket_last_error();

}

#endif // MINT_NETWORK_SOCKET_H
