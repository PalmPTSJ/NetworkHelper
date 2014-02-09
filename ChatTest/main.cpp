#include "network_core.h"
#include "network_server.h"
#include "network_client.h"
#include <time.h>
#include <fstream>
net_server_serverClass server;

void debug(string str) {
    cout << "Debug : " << str << endl;
}
void error(string err) {
    cout << "Error : " << err << endl;
}

void chat_reply(string str,int i) {
    cout << "Recv from " << server.getIpFrom(i) << " : " << str << endl;
    server.sendToAllClientExcept(str,i); /* echo to every client except itself */
}
void chat_run() {
    if(GetAsyncKeyState(VK_SPACE) && GetAsyncKeyState(VK_LSHIFT)) {
        server.stop();
        if(GetAsyncKeyState(VK_LCONTROL))
            server.forceStop();
    }
}
void startServer()
{
    server.setDebugFunc(debug);
    server.setup(4567,chat_run,chat_reply,error); /* Port 4567 */
    server.runLoop();
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

string body,line;
void http_reply(string str,int i) {
    if(str.find("GET / HTTP/1.1") != string::npos) {
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
    }
    server.disconnect(i);
}
void http_run() {
    // catch shutdown button
    if(GetAsyncKeyState(VK_SPACE) && GetAsyncKeyState(VK_LSHIFT)) {
        server.stop();
        if(GetAsyncKeyState(VK_LCONTROL))
            server.forceStop();
    }
}
void startHTTPserv()
{
    // load
    ifstream mf("test.html");
    while (getline(mf,line) )
    {
      body.append(line);
      body.push_back('\n');
    }
    mf.close();

    server.setDebugFunc(debug);

    server.setup(80,http_run,http_reply,error);
    server.runLoop();
}
net_client_clientClass client;
void chttp_run() {

}
void chttp_reply(string str) {
    int pos,len;
    string toFind = "<div class=\"wx-temperature\"><span itemprop=\"temperature-fahrenheit\">";
    if((pos=str.find(toFind)) != string::npos) {
        string data = str.substr(pos+toFind.size(),100);
        if((len = data.find("</span>")) == string::npos) {
            cout << "Temp Parse error , no closing tag found";
            client.disconnect();
            return;
        }
        string weather = data.substr(0,len);
        float temp;
        char cc[100];
        sprintf(cc,"%s",weather.c_str());
        sscanf(cc,"%f",&temp);
        cout << "Now : " << temp <<" F = " << (temp-32)*5/9 << " C (Auto-fetched from www.weather.com)"<< endl;
        client.disconnect(); // disconnect
    }
}
void startHTTPclient()
{
    string host = "www.weather.com";
    // ok connect !!
    client.setDebugFunc(debug);
    client.setup(chttp_run,chttp_reply,error);
    if(client.connect(host,80,5)) {
        // send http request
        string httpReq = "GET /weather/today/THPS0251:1:TH HTTP/1.1\n";
        httpReq.append("Host: www.weather.com\n");
        httpReq.append("User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; rv:27.0) Gecko/20100101 Firefox/27.0\n\n");
        client.send(httpReq);
        client.runLoop();
    }
}
int main()
{
    net_init();

    /*int mode;
    cout << "Please select mode ( 0=Server , 1=Client ) : ";
    cin >> mode;
    if(mode == 0) { startServer(); }
    else if(mode == 1) { startClient(); }*/
    /*cout << "Welcome to HTTP server test !\n";
    startHTTPserv();*/
    /*cout << "Welcome to DNS solver : ";
    string str;
    cin >> str;
    cout << "IP : " << inet_ntoa(net_createAddr(str,80).sin_addr);*/
    cout << "Welcome to HTTP client\n";
    startHTTPclient();

    net_close();
    cout << "Net closed , ";
    system("pause");
    return 0;
}
