#ifndef NETWORK_SERVER_H_INCLUDED
#define NETWORK_SERVER_H_INCLUDED

#include "network_ext.h"

#define NET_SERVER_STOPSUCCESS 1

class net_server_serverClass
{
public:
    net_server_serverClass();
    ~net_server_serverClass();
    void setup(int _port);
    void init();
    bool start();
    void stop();
    void forceStop();

    int acceptNewRequest();
    string getRecvDataFrom(int index);
    string getIpFrom(int index);
    int run();

    void sendToAllClient(string data);
    void sendToAllClientExcept(string data,int exceptIndex);

    bool isClientHaveData(int index);

    string getError();

    SOCKET sock;
    bool isStart;
    bool isShuttingDown;
    int port;
    string error;
    vector<net_ext_sock> clientList;

};

#endif // NETWORK_SERVER_H_INCLUDED
