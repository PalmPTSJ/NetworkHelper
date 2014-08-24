#include "network_core.h"
#include "network_server.h"
#include "network_client.h"
#include <time.h>
#include <fstream>
net_server_serverClass server;
unsigned int cvt(int a) {
    if(a>=0) return a;
    return 256+a;
}
unsigned int rotl(unsigned int value, int shift) {
    return (value << shift) | (value >> (sizeof(value)*8 - shift));
}
string bin(string str)
{
    string toRet = "";
    for(int i = 0;i < str.size();i++) {
        for(int j = 7;j >= 0;j--) {
            toRet.push_back(((str[i]>>j)&1) + '0');
        }
    }
    return toRet;
}
string bin(unsigned int i)
{
    string toRet = "";
    for(int j = 31;j >= 0;j--) {
        toRet.push_back(((i>>j) & 1) + '0');
    }
    return toRet;
}
string hex(string str)
{
    string toRet = "";
    char mp[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
    for(int i = 0;i < str.size();i++) {
        // 1 byte of str to 2 bytes
        char b1 = (str[i]>>4)&15;
        char b2 = str[i]&15;
        toRet.push_back(mp[b1]);
        toRet.push_back(mp[b2]);
    }
    return toRet;
}
string hex(byteArray data)
{
    string toRet = "";
    char mp[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
    for(int i = 0;i < data.size();i++) {
        // 1 byte of str to 2 bytes
        char b1 = (data[i]>>4)&15;
        char b2 = data[i]&15;
        toRet.push_back(mp[b1]);
        toRet.push_back(mp[b2]);
    }
    return toRet;
}
string SHA1(string str)
{
    unsigned int h0 = 0x67452301;
    unsigned int h1 = 0xEFCDAB89;
    unsigned int h2 = 0x98BADCFE;
    unsigned int h3 = 0x10325476;
    unsigned int h4 = 0xC3D2E1F0;
    // preprocessing
    unsigned long long int ml = str.size()*8;
    str.push_back(char(128));
    while(str.size()%64 != 56) str.push_back(0); // padding until %512 = 448 bits
    for(int i = 7;i >= 0;i--) {
        // add ml
        str.push_back((ml>>(8*i)) & 255);
    }
    for(int chk = 0;chk < str.size()/64;chk++)
    {
        unsigned int w[80];
        for(int i = 0;i < 16;i++) {
            unsigned int thisW = 0;
            for(int j = 0;j < 4;j++) {
                thisW<<=8;
                thisW+=cvt(str[chk*64+i*4+j]);
            }
            w[i] = thisW;
        }
        // extend 16 -> 80
        for(int i = 16;i < 80;i++) {
            w[i] = rotl(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16],1);
        }
        unsigned int a = h0;
        unsigned int b = h1;
        unsigned int c = h2;
        unsigned int d = h3;
        unsigned int e = h4;
        unsigned int f,k;
        for(int i = 0;i < 80;i++) {
            if(i < 20) {
                f = (b&c)|((~b)&d);
                k = 0x5A827999;
            }
            else if(i < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            }
            else if(i < 60) {
                f = (b&c)|(b&d)|(c&d);
                k = 0x8F1BBCDC;
            }
            else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }
            unsigned int tmp = rotl(a,5)+f+e+k+w[i];
            e = d;
            d = c;
            c = rotl(b,30);
            b = a;
            a = tmp;
        }
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }
    unsigned int hh[5];
    hh[0] = h0;
    hh[1] = h1;
    hh[2] = h2;
    hh[3] = h3;
    hh[4] = h4;
    // done , translate hh to string
    string toRet = "";
    for(int i = 0;i < 5;i++) {
        for(int j = 3;j >= 0;j--) {
            toRet.push_back((hh[i]>>(j*8)) & 255);
        }
    }
    return toRet;
}
char b64mp(int i)
{
    //cout << "[" << i << "]" << endl;
    if(i <= 25) return 'A'+i;
    else if(i <= 51) return 'a'+i-26;
    else if(i <= 61) return '0'+i-52;
    else if(i == 62) return '+';
    return '/';
}
string base64(string str)
{
    // A-Z a-z 0-9 + /
    /// split by 3
    string output = "";
    for(int i = 0;i < str.size();i+=3)
    {
        unsigned int c1,c2,c3;
        if(i+2<str.size()) {
            // full
            c1 = cvt(str[i]);
            c2 = cvt(str[i+1]);
            c3 = cvt(str[i+2]);
            output.push_back(b64mp( (c1>>2) ));
            output.push_back(b64mp( ((c1&3)<<4) + (c2>>4) ));
            output.push_back(b64mp( ((c2&15)<<2) + (c3>>6) ));
            output.push_back(b64mp( (c3&63) ));
        }
        else if(i+1<str.size()) {
            // 1 pad
            c1 = cvt(str[i]);
            c2 = cvt(str[i+1]);
            output.push_back(b64mp( (c1>>2) ));
            output.push_back(b64mp( ((c1&3)<<4) + (c2>>4) ));
            output.push_back(b64mp( (c2&15)<<2 ));
            output.push_back('=');
        }
        else {
            // 2 pad
            c1 = cvt(str[i]);
            output.push_back(b64mp( (c1>>2) ));
            output.push_back(b64mp( (c1&3)<<4 ));
            output.push_back('=');
            output.push_back('=');
        }
    }
    return output;
}
string wsHandshake(string str)
{
    /* HTTP/1.1 101 Switching Protocols
    Upgrade: websocket
    Connection: Upgrade
    Sec-WebSocket-Accept: HSmrc0sMlYUkAGmm5OPpG2HaGWk=
    Sec-WebSocket-Protocol: chat

    (Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==)
    */
    string key;
    int keyInd = str.find("Sec-WebSocket-Key: ");
    key = str.substr(keyInd+19,24);
    string GUID_str = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    key.append(GUID_str);
    string sha1 = SHA1(key);
    string accept = base64(sha1);
    string toRet = "";
    toRet.append("HTTP/1.1 101 Switching Protocols\r\n");
    toRet.append("Upgrade: websocket\r\n");
    toRet.append("Connection: Upgrade\r\n");
    toRet.append("Sec-WebSocket-Accept: "); toRet.append(accept); toRet.append("\r\n");
    toRet.append("\r\n");
    return toRet;
}
bool sp = false;
byteArray wsPing()
{
    byteArray toRet;
    toRet.push_back(137);
    toRet.push_back(0);
    toRet.push_back(0);
    return toRet;
}
string wsDecodeMsg(string wsMessage)
{
    string toRet = "";
    int meta[6];
    meta[0] = wsMessage[0]; // type
    meta[1] = wsMessage[1] & 127;
    int nowPtr = 2;
    if(meta[1] == 126) {
        // additional 2 bytes will be used for length
        nowPtr = 4;
    }
    else if(meta[1] == 127) {
        // additional 8 bytes will be used
        nowPtr = 10;
    }
    for(int i = 0;i < 4;i++) {
        meta[i+2] = wsMessage[nowPtr++];
    }
    for(int i = nowPtr;i < wsMessage.size();i++) {
        toRet.push_back(cvt(wsMessage[i]) ^ meta[((i-6)%4) + 2]);
    }
    return toRet;
}
byteArray wsEncodeMsg(string str)
{
    byteArray toRet;
    unsigned long long int sz = str.size();
    toRet.push_back(129);
    if(sz <= 125) {
        // normal len
        toRet.push_back(sz);
    }
    else if(sz <= 65535) {
        // 2+ len
        toRet.push_back(126);
        toRet.push_back(sz >> 8 &255);
        toRet.push_back(sz &255);
    }
    else {
        toRet.push_back(127);
        toRet.push_back(sz >> 56 &255);
        toRet.push_back(sz >> 48 &255);
        toRet.push_back(sz >> 40 &255);
        toRet.push_back(sz >> 32 &255);
        toRet.push_back(sz >> 24 &255);
        toRet.push_back(sz >> 16 &255);
        toRet.push_back(sz >> 8 &255);
        toRet.push_back(sz &255);
    }
    //toRet.push_back(0);toRet.push_back(0); toRet.push_back(0); toRet.push_back(0);
    toRet.insert(toRet.end(),str.begin(),str.end());
    //toRet.push_back(0);
    return toRet;
}
void run() {
    if(GetAsyncKeyState(VK_SPACE) && !sp) {
        sp = true;
        if(server.clientList.size() > 0) {
            server.sendTo(wsEncodeMsg("abc"),0);
        }
    }
    if(!GetAsyncKeyState(VK_SPACE) && sp) sp = false;
}
void recv(byteArray data,int i) {
    // parsing HTML
    string str = toString(data);
    if(str.find("GET / HTTP/1.1") != string::npos) {
        // is HTML request
        // check webSocket upgrade
        if(str.find("Upgrade: websocket") != string::npos) {
            // return webSocket upgrade response ( handshake )
            server.sendTo(wsHandshake(str),i);
            cout << "ws : Handshaked with " << i << " (" << server.getIpFrom(i) << ")" << endl;
        }
    }
    else {
        string decodeMsg = wsDecodeMsg(str);
        cout << "Message from " << i << " (" <<server.getIpFrom(i)<<") : " << decodeMsg << " [HEX] " << hex(str) << endl;
        if(decodeMsg.size() == 2) {
            // try to convert to int
            int num = cvt(decodeMsg[0])*256+cvt(decodeMsg[1]);
            if(num == 1001) {
                cout << "Get 1001 (GOAWY) code from " << i << " (" <<server.getIpFrom(i)<<")" << endl;
                server.disconnect(i);
            }
            else {
                cout << "Decoded number : " << num << endl;
            }
        }
        else {
            cout << "Recv : " << wsDecodeMsg(str) << " [HEX] " << hex(str) << endl;
            // reply back also
            server.sendTo(wsEncodeMsg("I recieved your message finally !!!"),i);
            //cout << "SENT : " << hex(wsEncodeMsg("ABCD")) << endl;
        }
    }
}
void err(string str) {
    cout << "Error : " << str << endl;
}
void debug(string str) {
    if(str.find("Run") == 0) cout << ".";
    else cout << "DBG : " << str << endl;
}
void startServer()
{
    server.init();
    server.setup(80,run,recv,err,NULL);
    server.setDebugFunc(debug);
    server.start();
    server.runLoop();
}
int main()
{
    net_init();
    startServer();
    net_close();
    return 0;
}
