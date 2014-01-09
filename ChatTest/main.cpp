#include "network_core.h"
#include "network_server.h"
#include <time.h>
#include <fstream>
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
            if(GetAsyncKeyState(VK_SPACE) && GetAsyncKeyState('T')) {
                string toSend;
                char buff[1000];
                fgets(buff,1000,stdin);
                toSend = buff;
                net_send(sock,toSend);
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
string htmlField(string fieldName,int d)
{
    char toRet[10000];
    sprintf(toRet,"%s: %d\n",fieldName.c_str(),d);
    string toR = toRet;
    return toR;
}
void startHTTPserv()
{
    // load
    string body,line;
    ifstream mf("test.html");
    while (getline(mf,line) )
    {
      body.append(line);
      body.push_back('\n');
    }
    mf.close();

    server.init();
    server.setup(80);
    server.start();
    int delay = 0;
    while(server.isStart) {
        --delay;
        if(delay < 0) {
            delay = 60;
            server.acceptNewRequest();
        }
        if(delay % 6 == 0) {
            server.run();
            for(int i = 0;i < server.clientList.size();i++) {
                if(server.isClientHaveData(i)) { /* if this client have data */
                    string recvStr = server.getRecvDataFrom(i);
                    // if HTTP request
                    if(recvStr.find("GET / HTTP/1.1") != string::npos) {
                        cout << "Got a http request from : #" << i << "\n";
                        // send back HTTP response and close connection
                        string response = "HTTP/1.1 200 OK\r\n";
                        response.append("\r\n");
                        response.append(body);
                        // send
                        server.sendTo(response,i);
                        response.append("\r\n");
                        // close connection
                        cout << "Sent HTTP response to : #" << i << " , Disconnecting ...\n";
                        server.disconnect(i);
                    }
                }
            }
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

int main()
{
    net_init();

    int mode;
    /*cout << "Please select mode ( 0=Server , 1=Client ) : ";
    cin >> mode;
    if(mode == 0) { startServer(); }
    else if(mode == 1) { startClient(); }*/
    cout << "Welcome to HTTP server test !\n";
    startHTTPserv();

    net_close();
    system("pause");
    return 0;
}
