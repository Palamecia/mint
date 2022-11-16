#ifndef SOCKET_H
#define SOCKET_H

#include <config.h>

#ifdef OS_WINDOWS
#include <WinSock2.h>
#else
#include <sys/socket.h>
#endif

namespace mint {

static constexpr const int sockopt_false = 0;
static constexpr const int sockopt_true = 1;

bool get_socket_option(int socket, int option, int *value);
bool set_socket_option(int socket, int option, int value);

}

#endif // SOCKET_H
