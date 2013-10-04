#include "network_core.h"

vector<net_sockHandle> clientList;

int main()
{
    net_init();
    SOCKET sock = net_createSocket();
    net_bindAndListen(sock,net_createAddr(4567));
    if(net_error()) {
        cout << "Server Socket creation error : " << net_lastError << endl;
        net_close();
        return 1;
    }
    cout << "Waiting ... ";
    int delay = 0;
    while(!(GetAsyncKeyState(VK_SPACE) && GetAsyncKeyState(VK_LSHIFT)))
    {
        ++delay;
        if(delay > 60) // Heavy network loop
        {
            net_sockHandle hnd = net_accept(sock);
            if(hnd.sock != INVALID_SOCKET)
            {
                cout << "\r                                \r" << "Got connection from : " << net_getIpFromHandle(hnd) << endl;
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
    net_close();
    return 0;
}
