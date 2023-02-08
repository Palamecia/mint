/**
 * Copyright (c) 2024 Gauvain CHERY.
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

#include <mint/config.h>
#include <mint/ast/symbol.h>

#ifdef OS_WINDOWS
#include <WinSock2.h>
#else
#include <sys/socket.h>
#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif
using SOCKET = int;
#endif

namespace mint {

namespace symbols {

static const Symbol Network("Network");
static const Symbol EndPoint("EndPoint");
static const Symbol IOStatus("IOStatus");
static const Symbol IOSuccess("IOSuccess");
static const Symbol IOWouldBlock("IOWouldBlock");
static const Symbol IOClosed("IOClosed");
static const Symbol IOError("IOError");

}

enum sockopt_bool : int {
	sockopt_false = 0,
	sockopt_true = 1
};

bool get_socket_option(SOCKET socket, int option, int *value);
bool set_socket_option(SOCKET socket, int option, int value);
bool get_socket_option(SOCKET socket, int option, sockopt_bool *value);
bool set_socket_option(SOCKET socket, int option, sockopt_bool value);
bool get_socket_option(SOCKET socket, int option, linger *value);
bool set_socket_option(SOCKET socket, int option, const linger *value);
bool get_socket_option(SOCKET socket, int option, timeval *value);
bool set_socket_option(SOCKET socket, int option, const timeval *value);

bool get_socket_option(SOCKET socket, int level, int option, int *value);
bool set_socket_option(SOCKET socket, int level, int option, int value);
bool get_socket_option(SOCKET socket, int level, int option, u_char *value);
bool set_socket_option(SOCKET socket, int level, int option, u_char value);
bool get_socket_option(SOCKET socket, int level, int option, sockopt_bool *value);
bool set_socket_option(SOCKET socket, int level, int option, sockopt_bool value);
bool get_socket_option(SOCKET socket, int level, int option, void *value, socklen_t len);
bool set_socket_option(SOCKET socket, int level, int option, const void *value, socklen_t len);

template<typename opt_t>
bool get_socket_option(SOCKET socket, int level, int option, opt_t *value) {
	return get_socket_option(socket, level, option, value, sizeof(opt_t));
}

template<typename opt_t>
bool set_socket_option(SOCKET socket, int level, int option, const opt_t *value) {
	return set_socket_option(socket, level, option, value, sizeof(opt_t));
}

}

#endif // MINT_NETWORK_SOCKET_H
