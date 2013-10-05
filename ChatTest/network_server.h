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
    bool start();
    void stop();

    int acceptNewRequest();
    int run();

    string getError();


private:
    SOCKET sock;
    bool isStart;
    bool isShuttingDown;
    int port;
    string error;
    vector<net_ext_sock> clientList;
};

#endif // NETWORK_SERVER_H_INCLUDED
