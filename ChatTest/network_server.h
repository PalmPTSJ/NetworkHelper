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

    void init();

    void setup(int _port); // Using manual flow
    void setup(int _port,void(*run)(),void(*recv)(string,int),void(*err)(string)); // Using automatic flow

    bool start();

    int acceptNewRequest(); // Check for new request

    int run(); // Manual flow recieving
    void runLoop(); // Automatic flow recieving & accept & error

    void disconnect(int index);

    void stop();
    void forceStop();

    string getRecvDataFrom(int index);
     bool isClientHaveData(int index);
    string getIpFrom(int index);

    void sendTo(string data,int index);
    void sendToAllClient(string data);
    void sendToAllClientExcept(string data,int exceptIndex);

    void setDebugFunc(void(*f)(string));

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
    void (*errFunc)(string);
    int delay;
};

#endif // NETWORK_SERVER_H_INCLUDED
