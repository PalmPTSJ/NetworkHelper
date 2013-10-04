#include "network_core.h"

WSADATA net_wsaData;
string net_lastError = "";
char net_tempBuffer[2048] = "\0";

void net_init() {
    WSAStartup(MAKEWORD(2,0), &net_wsaData);
}

void net_close() {
    WSACleanup();
}

sockaddr_in net_createAddr(string ip,int port)
{
    sockaddr_in toRet;
    toRet.sin_family = AF_INET;
    toRet.sin_port = htons(port);
    if(ip.size() == 0)
        toRet.sin_addr.S_un.S_addr = INADDR_ANY;
    else
        toRet.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
    return toRet;
}

SOCKET net_createSocket()
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(sock == INVALID_SOCKET) {
        net_lastError = "Invalid Socket while creating";
        return INVALID_SOCKET;
    }

    u_long NB_Flag = 1;
    ioctlsocket(sock, FIONBIO, &NB_Flag);
    return sock;
}
bool net_bindAndListen(SOCKET &sock,sockaddr_in addr)
{
    if(bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        net_lastError = "Can't bound socket";
        return false;
    }

    if (listen(sock, SOMAXCONN)!=0) {
        net_lastError = "Can't listen";
        return false;
    }
}

net_sockHandle net_accept(SOCKET &sock)
{
    sockaddr_in addr;
    int addrSize = sizeof(addr);
    net_sockHandle toRet;
    toRet.sock = INVALID_SOCKET;
    SOCKET rSock = accept(sock,(sockaddr*)(&addr),&addrSize);

    if(rSock != INVALID_SOCKET)
    {
        toRet.sock = rSock;
        u_long NB_Flag = 1;
        ioctlsocket(toRet.sock, FIONBIO, &NB_Flag);
        toRet.addr = addr;
    }
    return toRet;
}

string net_getIpFromHandle(net_sockHandle& hnd)
{
    return inet_ntoa(hnd.addr.sin_addr);
}

string net_recv(SOCKET &sock)
{
    memset(net_tempBuffer,0,2048);
    int recvStat = recv(sock,net_tempBuffer,2048,0);
    if(recvStat != 0 && recvStat != SOCKET_ERROR)
    {
        string str = net_tempBuffer;
        return str;
    }
    return "";
}

bool net_error()
{
    if(net_lastError.size() > 0) return true;
    return false;
}

void net_closeSocket(SOCKET& sock)
{
    closesocket(sock);
}
void net_closeHandle(net_sockHandle& hnd)
{
    closesocket(hnd.sock);
}

bool net_connect(SOCKET &sock,sockaddr_in addr,int timeout)
{
    int retStat = connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    fd_set socket_set;
    socket_set.fd_array[0] = sock;
    socket_set.fd_count = 1;
    timeval timer;
    timer.tv_sec = timeout;

    int ret = select(sock, NULL, &socket_set, NULL, &timer);

    if(ret == 0) return false;
    return true;
}
