#ifndef NETWORK_CORE_H_INCLUDED
#define NETWORK_CORE_H_INCLUDED

#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#include <string>
#include <iostream>
#include <vector>

#define NET_DEFAULT_PORT 4657

using namespace std;

struct net_sockHandle
{
    SOCKET sock;
    sockaddr_in addr;
};

string net_getIpFromHandle(net_sockHandle& hnd);

void net_init();

sockaddr_in net_createAddr(string ip,int port);

SOCKET net_createSocket();
bool net_bindAndListen(SOCKET &sock,sockaddr_in addr);

net_sockHandle net_accept(SOCKET &sock);

bool net_connect(SOCKET &sock,sockaddr_in addr,int timeout);

void net_send(SOCKET &sock, string data);
string net_recv(SOCKET &sock);

void net_closeSocket(SOCKET& sock);
void net_closeHandle(net_sockHandle& hnd);

void net_close();

bool net_error();

extern WSADATA net_wsaData;
extern string net_lastError;
extern char net_tempBuffer[2048];
#endif // NETWORK_CORE_H_INCLUDED
