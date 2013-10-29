#include "network_core.h"
#include "network_server.h"

void clearScreen() {
    COORD topLeft  = { 0, 0 };
    HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO screen;
    DWORD written;

    GetConsoleScreenBufferInfo(console, &screen);
    FillConsoleOutputCharacterA(
        console, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written
    );
    FillConsoleOutputAttribute(
        console, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE,
        screen.dwSize.X * screen.dwSize.Y, topLeft, &written
    );
    SetConsoleCursorPosition(console, topLeft);
}

net_server_serverClass server;

void startServer()
{
    server.init(); /* init */
    server.setup(4567); /* Port 4567 */
    if(!server.start()) { /* if start() return false ( not success ) */
        cout << "Server Starting error : " << net_lastError << WSAGetLastError() << endl;
        return;
    }
    cout << "Waiting For connection ..." << endl;
    int delay = 0;
    while(server.isStart) /* Run ( Space + Left_Shift to exit ) */
    {
        ++delay;
        if(delay > 60) /* Heavy network loop */ {
            server.acceptNewRequest();
            delay = 0;
        }
        if(delay % 6 == 0) /* Small network loop */ {
            server.run();
            for(int i = 0;i < server.clientList.size();i++) {
                if(server.isClientHaveData(i)) { /* if this client have data */
                    string recvStr = server.getRecvDataFrom(i);
                    cout << "Recv from " << server.getIpFrom(i) << " : " << recvStr << endl;
                    server.sendToAllClientExcept(recvStr,i); /* echo to every client except itself */
                }
            }
        }
        if(net_error()) /* If have error */
        {
            cout << endl << "Error recieve : " << net_lastError << endl;
            break; /* Force exit */
        }
        if(GetAsyncKeyState(VK_SPACE) && GetAsyncKeyState(VK_LSHIFT))
        { /* Shutdown */
            if(!server.isShuttingDown) { /* If not shutting down */
                cout << "Graceful shutdown mode start ( Will wait for all client to close ) " << endl;
                server.stop(); /* Start shutting down mode ( graceful ) */
            }
            if(GetAsyncKeyState(VK_LCONTROL)) { /* Force shutdown */
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
    vector<string> chatHistory;
    string typing = "";
    if(con) {
        cout << "Connect success !";
        int delay = 0;
        while(true)
        {
            /* if any char type */
            for(int i = 'a';i <= 'z';i++) {

            }
            if(++delay > 60) /* Recieve data */
            {
                delay = 0;
                string str;
                int stat = net_recv(sock,str);
                if(stat == NET_RECV_CLOSE || stat == NET_RECV_ERROR)
                {
                    // close
                    cout << "Error / Close signal recieved , exitting" << endl;
                    break;
                }
                else if(stat == NET_RECV_OK) {
                    chatHistory.push_back(str);
                    if(chatHistory.size() > 20) chatHistory.erase(chatHistory.begin());
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
