#include "network_core.h"
#include "network_server.h"

net_server_serverClass server;

void startServer()
{
    server.init(); /** init */
    server.setup(4567); /** Port 4567 */
    if(!server.start()) { /** if start() return false */
        cout << "Server Starting error : " << net_lastError << WSAGetLastError() << endl;
        return;
    }
    cout << "Waiting For connection ..." << endl;
    int delay = 0;
    while(server.isStart) /** Run ( Space + Left_Shift to exit ) */
    {
        ++delay;
        if(delay > 60) /** Heavy network loop */ {
            server.acceptNewRequest();
            delay = 0;
        }
        if(delay % 6 == 0) /** Small network loop */ {
            server.run();
            for(int i = 0;i < server.clientList.size();i++) {
                if(server.clientList[i].recvBuff.size() > 0) {
                    string recvStr = server.getRecvDataFrom(i);
                    cout << "Recv from " << server.getIpFrom(i) << " : " << recvStr << endl;
                    // will echo to all
                    server.sendToAllClient("New message : ");
                    server.sendToAllClient(recvStr);
                }
            }
        }
        if(net_error()) /** If have error */
        {
            cout << endl << "Error recieve : " << net_lastError << endl;
            break;
        }
        if(GetAsyncKeyState(VK_SPACE) && GetAsyncKeyState(VK_LSHIFT))
        { /** Shutdown */
            if(!server.isShuttingDown) {
                cout << "Graceful shutdown mode start ( Will wait for all client to close ) " << endl;
                server.stop(); // start stop signal ( gracefully )
            }
            if(GetAsyncKeyState(VK_LCONTROL))
            {
                // force shutdown
                cout << "Force shutdown ! ";
                server.forceStop();
            }
        }
        Sleep(16);
    }
}

void startClient()
{
    SOCKET sock = net_createSocket();
    cout << "IP Address : ";
    string ip;
    cin >> ip;
    bool con = net_connect(sock,net_createAddr(ip,4567),5);
    if(con) {
        cout << "Connect success !";
        int delay = 0;
        while(true)
        {
            if(GetAsyncKeyState(13) && GetAsyncKeyState(VK_SPACE)) /** Space + Enter to type */
            {
                string toSend;
                cout << endl << "$";
                cin >> toSend; // This function will block everything
                net_send(sock,toSend);
            }
            if(++delay > 60)
            {
                delay = 0;
                string str;
                int stat = net_recv(sock,str);
                if(stat == NET_RECV_CLOSE || stat == NET_RECV_ERROR)
                {
                    // close
                    cout << "Error / Close recieve , exitting" << endl;
                    break;
                }
                else if(stat == NET_RECV_OK) {
                    cout << "Recieve : " << str << endl;
                }
            }
            Sleep(16);
        }
    }
    else cout << "Connect Failed";
    net_closeSocket(sock);
}

int main()
{
    net_init();

    int mode;
    cout << "Please select mode ( 0=Server , 1=Client ) : ";
    cin >> mode;
    if(mode == 0) { startServer(); }
    else if(mode == 1) { startClient(); }

    net_close();
    return 0;
}
