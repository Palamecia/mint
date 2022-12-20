#ifndef SOCKET_H
#define SOCKET_H

#include <config.h>
#include <ast/symbol.h>

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

static constexpr const int sockopt_false = 0;
static constexpr const int sockopt_true = 1;

namespace symbols {

static const Symbol Network("Network");
static const Symbol EndPoint("EndPoint");
static const Symbol IOStatus("IOStatus");
static const Symbol IOSuccess("IOSuccess");
static const Symbol IOWouldBlock("IOWouldBlock");
static const Symbol IOClosed("IOClosed");
static const Symbol IOError("IOError");

}

bool get_socket_option(SOCKET socket, int option, int *value);
bool set_socket_option(SOCKET socket, int option, int value);

}

#endif // SOCKET_H
