#include "network_server.h"
net_server_serverClass::net_server_serverClass()
{
    /** Constructor */
    sock = INVALID_SOCKET;
}
void net_server_serverClass::init()
{
    sock = net_createSocket();
    port = NET_DEFAULT_PORT;
    isStart = false;
    isShuttingDown = false;
    if(debugFunc != NULL) (*debugFunc)("Server init");
}
net_server_serverClass::~net_server_serverClass()
{
    /** Destructor */
    if(isStart) {
        // force shutdown
        for(int i = 0;i < clientList.size();i++) /** force close everything */
            net_closeHandle(clientList[i].sockHandle);
        net_closeSocket(sock);
    }
}
void net_server_serverClass::setup(int _port,void(*run)(),void(*recv)(string,int),void(*err)(string))
{
    /** Set up data before start */
    if(sock == INVALID_SOCKET) init();
    port = _port;
    runFunc = run;
    recvFunc = recv;
    errFunc = err;
}
void net_server_serverClass::setup(int _port)
{
    setup(_port,NULL,NULL,NULL);
}

bool net_server_serverClass::start()
{
    net_bindAndListen(sock,net_createAddr("",port)); /** Bind to port , listen to any ip */
    if(net_error()) return false;
    isStart = true;
    return true;
}

void net_server_serverClass::disconnect(int index)
{
    if(index < 0 || index >= clientList.size()) return;
    if(clientList[index].sendBuff.size() > 0)  /// send any pending data
        net_send(clientList[index].sockHandle.sock,clientList[index].sendBuff);
    clientList[index].status = NET_EXT_SOCK_CLOSING;
    net_shutdownHandle(clientList[index].sockHandle);
}
void net_server_serverClass::stop()
{
    if(isShuttingDown) return;
    if(debugFunc != NULL) (*debugFunc)("Shutting down");
    for(int i = 0;i < clientList.size();i++) { /** Initialize graceful shutdown ( will wait until the other side close ) */
        clientList[i].status = NET_EXT_SOCK_CLOSING;
        net_shutdownHandle(clientList[i].sockHandle);
    }
    isShuttingDown = true;
}
void net_server_serverClass::forceStop()
{
    if(debugFunc != NULL) (*debugFunc)("Force close");
    for(int i = 0;i < clientList.size();i++) /** force close everything */
        net_closeHandle(clientList[i].sockHandle);
    net_closeSocket(sock);
    isStart = false;
}
int net_server_serverClass::run()
{
    for(int i = 0;i < clientList.size();i++) {
        // send pending data
        if(clientList[i].sendBuff.size() > 0) { /// send waiting data
            net_send(clientList[i].sockHandle.sock,clientList[i].sendBuff);
            clientList[i].sendBuff = "";
        }
        // recv
        string recvStr;
        int ret = net_recv(clientList[i].sockHandle.sock,recvStr);
        if(ret == NET_RECV_CLOSE && clientList[i].status == NET_EXT_SOCK_CLOSING) { /// shutdown handling
            char cc[100]; sprintf(cc,"%s [%d] disconnected (SHUTD)",net_getIpFromHandle(clientList[i].sockHandle).c_str(),i);
            if(debugFunc != NULL) (*debugFunc)(string(cc));
            net_closeHandle(clientList[i].sockHandle);
            clientList.erase(clientList.begin() + i);
            --i;
        }
        else if(ret == NET_RECV_OK) { /// recieve data
            clientList[i].recvBuff.append(recvStr);
        }
        else if(ret == NET_RECV_CLOSE) { /// other side disconnect
            char cc[100]; sprintf(cc,"%s [%d] disconnected (CSIGN)",net_getIpFromHandle(clientList[i].sockHandle).c_str(),i);
            if(debugFunc != NULL) (*debugFunc)(string(cc));
            net_shutdownHandle(clientList[i].sockHandle);
            net_closeHandle(clientList[i].sockHandle);
            clientList.erase(clientList.begin() + i);
            --i;
        }
    }
    if(isShuttingDown && clientList.size() == 0) { /// graceful shutdown success
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
        char cc[100]; sprintf(cc,"%s connected",net_getIpFromHandle(hnd).c_str());
        if(debugFunc != NULL) (*debugFunc)(string(cc));
        clientList.push_back(client);
        return 1;
    }
    return 0;
}

bool net_server_serverClass::isClientHaveData(int index)
{
    return (clientList[index].recvBuff.size()>0)?true:false;
}

string net_server_serverClass::getRecvDataFrom(int index)
{
    if(index < 0 || index >= clientList.size()) return "";
    string toRet = clientList[index].recvBuff;
    clientList[index].recvBuff.clear();
    return toRet;
}

string net_server_serverClass::getIpFrom(int index)
{
    if(index < 0 || index >= clientList.size()) return "";
    return net_getIpFromHandle(clientList[index].sockHandle);
}
void net_server_serverClass::sendTo(string data,int index)
{
    if(index < 0 || index >= clientList.size()) return;
    clientList[index].sendBuff.append(data);
}
void net_server_serverClass::sendToAllClient(string data)
{
    for(int i = 0;i < clientList.size();i++)
        clientList[i].sendBuff.append(data);
}
void net_server_serverClass::sendToAllClientExcept(string data,int exceptIndex)
{
    for(int i = 0;i < clientList.size();i++)
        if(i != exceptIndex)
            clientList[i].sendBuff.append(data);
}

void net_server_serverClass::setDebugFunc(void(*f)(string))
{
    debugFunc = f;
}

void net_server_serverClass::runLoop()
{
    if(!isStart) {
        // try to start
        if(!start()) {
            if(errFunc != NULL) (*errFunc)(net_lastError);
            return;
        }
    }
    if(debugFunc != NULL) (*debugFunc)("Run Loop started");
    while(isStart) {
        --delay;
        if(delay < 0) {
            delay = NET_SERVER_ACCEPT;
            acceptNewRequest();
        }
        if(delay % NET_SERVER_RUN == 0) {
            run();
            for(int i = 0;i < clientList.size();i++) {
                if(isClientHaveData(i)) { /* if this client have data */
                    string recvStr = getRecvDataFrom(i);
                    if(recvFunc != NULL) (*recvFunc)(recvStr,i);
                }
            }
        }
        if(net_error()) /* If have error */
        {
            if(errFunc != NULL) (*errFunc)(net_lastError);
            break; /* Force exit */
        }
        if(runFunc != NULL) (*runFunc)();
        Sleep(NET_SERVER_SLEEP);
    }
}
