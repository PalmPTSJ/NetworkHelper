#ifndef NETWORK_CLIENT_H_INCLUDED
#define NETWORK_CLIENT_H_INCLUDED

#include "network_ext.h"

#define NET_CLIENT_ONLINE 1
#define NET_CLIENT_OFFLINE 0

#define NET_CLIENT_SLEEP 16 // time sleep
#define NET_CLIENT_DELAY 1

class net_client_clientClass
{
public:
    net_client_clientClass();
    ~net_client_clientClass();

    void setup(void(*run)(),void(*recv)(string),void(*err)(string)); // Using automatic flow
    void setDebugFunc(void(*f)(string));

    bool connect(string ip,int port,int timeout);

    void runLoop();
    void send(string str);

    void disconnect();

    void (*debugFunc)(string);
    void (*runFunc)();
    void (*recvFunc)(string);
    void (*errFunc)(string);

    int delay;
    int status;
    SOCKET sock;
};

#endif // NETWORK_CLIENT_H_INCLUDED
