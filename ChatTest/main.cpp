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
    while(!(GetAsyncKeyState(VK_SPACE) && GetAsyncKeyState(VK_LSHIFT)))
    {
        ++delay;
        if(delay > 60) // Heavy network loop
        {
            net_sockHandle hnd = net_accept(sock);
            if(hnd.sock != INVALID_SOCKET)
            {
                cout << "Got connection from : " << net_getIpFromHandle(hnd) << endl;
                clientList.push_back(hnd);
            }
            delay = 0;
        }
        if(delay % 6 == 0) // Small network loop
        {
            for(int i = 0;i < clientList.size();i++)
            {
                string str = net_recv(clientList[i].sock);
                if(str.size() > 0)
                {
                    cout << "Recieve from " << net_getIpFromHandle(clientList[i]) << " ( " << i << " ) : " << str << endl;
                }
            }
        }
        if(net_error())
        {
            cout << endl << "Error recieve : " << net_lastError << endl;
            break;
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
    if(con) cout << "Connect success !";
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
