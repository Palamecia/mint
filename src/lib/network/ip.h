#ifndef MINT_NETWORK_IP_H
#define MINT_NETWORK_IP_H

#include "mint/config.h"
#include "socket.h"
#include <string>
#include <tuple>

#ifdef MINT_OS_WINDOWS
#include <WinSock2.h>
#else
#ifdef MINT_OS_LINUX
#include <linux/sockios.h>
#endif
#include <sys/socket.h>
#include <sys/types.h>
#endif

namespace mint_network {

constexpr inline int ip_version_4 = 4;
constexpr inline int ip_version_6 = 6;

std::tuple<std::string, u_short> get_ip_socket_info(const sockaddr& endpoint);

}

#endif // MINT_NETWORK_IP_H
