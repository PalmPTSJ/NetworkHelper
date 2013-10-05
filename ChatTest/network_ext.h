#ifndef NETWORK_EXT_H_INCLUDED
#define NETWORK_EXT_H_INCLUDED

#include "network_core.h"

#define NET_EXT_SOCK_ONLINE 0
#define NET_EXT_SOCK_CLOSING 1
#define NET_EXT_SOCK_OFFLINE 2

struct net_ext_sock
{
    net_sockHandle sockHandle;
    string recvBuff;
    string sendBuff;
    int status;
};

net_ext_sock net_ext_createSock();

#endif // NETWORK_EXT_H_INCLUDED
