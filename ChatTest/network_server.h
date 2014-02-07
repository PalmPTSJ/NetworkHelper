#ifndef NETWORK_SERVER_H_INCLUDED
#define NETWORK_SERVER_H_INCLUDED

#include "network_ext.h"

#define NET_SERVER_STOPSUCCESS 1
#define NET_SERVER_SLEEP 16 // time sleep
#define NET_SERVER_ACCEPT 60 // delay count until accept
#define NET_SERVER_RUN 6 // delay count until run , must < accept
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

    void sendTo(string data,int index);
    void sendToAllClient(string data);
    void sendToAllClientExcept(string data,int exceptIndex);

    void setDebugFunc(void(*f)(string));
    void setRunFunc(void(*f)());
    void setRecvFunc(void(*f)(string,int));

    void disconnect(int index);

    void runLoop();

    bool isClientHaveData(int index);

    string getError();

    SOCKET sock;
    bool isStart;
    bool isShuttingDown;
    int port;
    string error;
    vector<net_ext_sock> clientList;
    void (*debugFunc)(string);
    void (*runFunc)();
    void (*recvFunc)(string,int);
    int delay;
};

#endif // NETWORK_SERVER_H_INCLUDED
