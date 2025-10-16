#ifndef MINT_NETWORK_IP_H
#define MINT_NETWORK_IP_H

#include "mint/config.h"
#include <string>

#ifdef MINT_OS_WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#ifdef MINT_OS_LINUX
#include <linux/sockios.h>
#endif
#include <sys/socket.h>
#include <sys/types.h>
#endif

namespace mint {

constexpr inline int ip_version_4 = 4;
constexpr inline int ip_version_6 = 6;

int get_ip_socket_info(const sockaddr* socket, socklen_t socketlen, std::string* sock_addr, u_short* sock_port);

}

#endif // MINT_NETWORK_IP_H
