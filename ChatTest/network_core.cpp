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
    else {
        // resolve host name
        hostent* host = gethostbyname(ip.c_str()) ;
        //toRet.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
        if(host == NULL) {
            // error
            net_lastError = "Host name can't be resolved";
            toRet.sin_addr.S_un.S_addr = 0;
            return toRet;
        }
        toRet.sin_addr.S_un.S_addr = *((unsigned long*)host->h_addr);
    }
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

int net_recv(SOCKET &sock,string& str)
{
    memset(net_tempBuffer,0,2048);
    int recvStat = recv(sock,net_tempBuffer,2048,0);
    if(recvStat != 0 && recvStat != SOCKET_ERROR)
    {
        str = net_tempBuffer;
        return NET_RECV_OK;
    }
    if(recvStat == 0)
        return NET_RECV_CLOSE;
    if(recvStat == SOCKET_ERROR && WSAGetLastError() != 10035)
        return NET_RECV_ERROR;
    return NET_RECV_NONE;
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
    // check addr validity
    if(addr.sin_addr.S_un.S_addr == 0) return false; // incorrect ip
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

void net_send(SOCKET &sock, string data)
{
    char toSend_buff[data.size()+1];
    strcpy(toSend_buff,data.c_str());
    send(sock,toSend_buff,data.size()+1,0);
}

string net_getWsaError()
{
    int wsaErr = WSAGetLastError();
    switch(wsaErr)
    {
        case 0 : return "";
        case 10035 : return "Would Block (10035)";
        case 10054 : return "Connection Reset (10054)";
        default :
            char str[20];
            sprintf(str,"%d",wsaErr);
            return string(str);
    }
}

void net_shutdownHandle(net_sockHandle& hnd)
{
    shutdown(hnd.sock,SD_SEND);
}
