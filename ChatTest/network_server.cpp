#include "network_server.h"

net_server_serverClass::net_server_serverClass()
{
    /** Constructor */
    sock = net_createSocket();
    port = NET_DEFAULT_PORT;
    isStart = false;
    isShuttingDown = false;
}
net_server_serverClass::~net_server_serverClass()
{
    /** Destructor */
    if(isStart) {
        // force shutdown
        net_closeSocket(sock);
    }
}
void net_server_serverClass::setup(int _port)
{
    /** Set up data before start */
    port = _port;
}
bool net_server_serverClass::start()
{
    net_bindAndListen(sock,net_createAddr("",port)); /** Bind to port , listen to any ip */
    if(net_error())
        return false;
    isStart = true;
    return true;
}
void net_server_serverClass::stop()
{
    for(int i = 0;i < clientList.size();i++) {
        /** Initialize graceful shutdown */
        clientList[i].status = NET_EXT_SOCK_CLOSING;
        net_shutdownHandle(clientList[i].sockHandle);
    }
    isShuttingDown = true;
}

int net_server_serverClass::run()
{
    for(int i = 0;i < clientList.size();i++) {
        // send pending data
        if(clientList[i].sendBuff.size() > 0) {
            net_send(clientList[i].sockHandle.sock,clientList[i].sendBuff);
        }
        // recv
        string recvStr;
        int ret = net_recv(clientList[i].sockHandle.sock,recvStr);
        if(ret == NET_RECV_CLOSE && clientList[i].status == NET_EXT_SOCK_CLOSING)
        {
            // shutdown successful
            net_closeHandle(clientList[i].sockHandle);
            clientList.erase(clientList.begin() + i);
            --i;
        }
        else if(ret == NET_RECV_OK) {
            clientList[i].recvBuff.append(recvStr);
        }
    }
    if(isShuttingDown && clientList.size() == 0)
    {
        // graceful shutdown success
        net_closeSocket(sock);
        isStart = false;
        isShuttingDown = false;
        return NET_SERVER_STOPSUCCESS;
    }
    return 0;
}

int net_server_serverClass::acceptNewRequest()
{
    if(!isStart) return -1;
    net_sockHandle hnd = net_accept(sock);
    if(hnd.sock != INVALID_SOCKET) {
        /** Connection recieved */
        net_ext_sock client = net_ext_createSock();
        client.sockHandle = hnd;
        client.status = NET_EXT_SOCK_ONLINE;
        cout << "Got connection from : " << net_getIpFromHandle(hnd) << endl;
        clientList.push_back(client);
        return 1;
    }
    return 0;
}
