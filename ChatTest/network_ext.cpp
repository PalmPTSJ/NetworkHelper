#include "network_ext.h"

net_ext_sock net_ext_createSock()
{
    net_ext_sock toRet;
    toRet.recvBuff = toRet.sendBuff = "";
    toRet.status = NET_EXT_SOCK_OFFLINE;
}
