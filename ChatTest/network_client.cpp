#include "network_client.h"

net_client_clientClass::net_client_clientClass()
{
    status = NET_CLIENT_OFFLINE;
    sock = INVALID_SOCKET;
}
net_client_clientClass::~net_client_clientClass()
{
    if(sock != INVALID_SOCKET) net_closeSocket(sock);
}
void net_client_clientClass::setup(void(*run)(),void(*recv)(string),void(*err)(string))
{
    if(debugFunc != NULL) (*debugFunc)("Setup");
    runFunc = run;
    recvFunc = recv;
    errFunc = err;
}
bool net_client_clientClass::connect(string ip,int port,int timeout)
{
    if(sock == INVALID_SOCKET) sock = net_createSocket();
    if(debugFunc != NULL) (*debugFunc)("Connecting");
    bool con = net_connect(sock,net_createAddr(ip,port),timeout);
    if(!con) {
        if(net_error()) { if(errFunc != NULL) (*errFunc)(net_lastError); }
        else if(errFunc != NULL) (*errFunc)("Connection Error");
        return false;
    }
    else {
        status = NET_CLIENT_ONLINE;
        return true;
    }
}
void net_client_clientClass::runLoop()
{
    if(status == NET_CLIENT_OFFLINE) {
        return;
    }
    if(debugFunc != NULL) (*debugFunc)("Run Loop started");
    while(status == NET_CLIENT_ONLINE) {
        --delay;
        if(delay < 0) {
            delay = NET_CLIENT_DELAY;
            string str;
            int stat = net_recv(sock,str);
            if(stat == NET_RECV_CLOSE || stat == NET_RECV_ERROR)
            {
                // close
                if(debugFunc != NULL) (*debugFunc)("Disconnected (CSIGN)");
                status = NET_CLIENT_OFFLINE;
                break;
            }
            else if(stat == NET_RECV_OK) {
                if(recvFunc != NULL) (*recvFunc)(str);
            }
        }
        if(net_error()) /* If have error */
        {
            if(errFunc != NULL) (*errFunc)(net_lastError);
            break; /* Force exit */
        }
        if(runFunc != NULL) (*runFunc)();
        Sleep(NET_CLIENT_SLEEP);
    }
}
void net_client_clientClass::send(string str)
{
    net_send(sock,str);
}
void net_client_clientClass::disconnect()
{
    shutdown(sock,SD_SEND);
    status = NET_CLIENT_OFFLINE;
}
void net_client_clientClass::setDebugFunc(void(*f)(string))
{
    debugFunc = f;
}
