#ifndef MINT_NETWORK_IP_H
#define MINT_NETWORK_IP_H

#include <mint/config.h>
#include <string>

#ifdef OS_WINDOWS
#include <ws2tcpip.h>
#else
#include <linux/sockios.h>
#endif

namespace mint {

int get_ip_socket_info(const sockaddr *socket, socklen_t socketlen, std::string *sock_addr, u_short *sock_port);

}

#endif // MINT_NETWORK_IP_H
