#include "network_core.h"

vector<net_sockHandle> clientList;

void startServer()
{
    SOCKET sock = net_createSocket();
    net_bindAndListen(sock,net_createAddr("",4567));
    if(net_error()) {
        cout << "Server Socket creation error : " << net_lastError << endl;
        return;
    }
    cout << "Waiting For connection ..." << endl;
    int delay = 0;
    while(!(GetAsyncKeyState(VK_SPACE) && GetAsyncKeyState(VK_LSHIFT))) /** Run ( Space + Left_Shift to exit )*/
    {
        ++delay;
        if(delay > 60) /** Heavy network loop */ {
            net_sockHandle hnd = net_accept(sock);
            if(hnd.sock != INVALID_SOCKET) {
                cout << "Got connection from : " << net_getIpFromHandle(hnd) << endl;
                clientList.push_back(hnd);
            }
            delay = 0;
        }
        if(delay % 6 == 0) /** Small network loop */ {
            for(int i = 0;i < clientList.size();i++) { /** Loop through every client */
                string str;
                int recvStat = net_recv(clientList[i].sock,str); // Recieve data
                if(recvStat == NET_RECV_OK) /** If have any data */ {
                    cout << "Recieve from " << net_getIpFromHandle(clientList[i]) << " ( " << i << " ) : " << str << endl;
                }
                else if(recvStat == NET_RECV_CLOSE)
                {
                    cout << "Connection close from " << net_getIpFromHandle(clientList[i]) << endl;
                    net_closeHandle(clientList[i]);
                    clientList.erase(clientList.begin() + i);
                    i--;
                }
                else if(recvStat == NET_RECV_ERROR)
                {
                    cout << "Recieve error from " << net_getIpFromHandle(clientList[i]) << " : " << net_getWsaError() << endl;
                    net_closeHandle(clientList[i]);
                    clientList.erase(clientList.begin() + i);
                    i--;
                }
            }
        }
        if(net_error()) /** If have error */
        {
            cout << endl << "Error recieve : " << net_lastError << endl;
            break;
        }
        Sleep(16);
    }
    for(int i = 0;i < clientList.size();i++) /** Close all socket ( handle ) */
        net_closeHandle(clientList[i]);
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
        while(true)
        {
            if(GetAsyncKeyState(VK_SPACE))
            {
                string toSend;
                cout << endl << "To Send : ";
                cin >> toSend; // This function will block everything
                net_send(sock,toSend);
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
