#include "network_ext.h"

net_ext_sock net_ext_createSock()
{
    net_ext_sock toRet;
    toRet.recvBuff.clear();
    toRet.sendBuff.clear();
    toRet.status = NET_EXT_SOCK_OFFLINE;
    return toRet;
}
byteArray toByteArray(string str)
{
    byteArray toRet;
    for(int i = 0;i < str.size();i++) {
        toRet.push_back((unsigned char)(str[i]));
    }
    return toRet;
}
string toString(byteArray bA)
{
    string toRet;
    for(int i = 0;i < bA.size();i++) {
        toRet.push_back(bA[i]);
    }
    return toRet;
}
