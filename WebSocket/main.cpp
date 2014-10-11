#define WINVER 0x0601
#include "network_core.h"
#include "network_server.h"
#include "network_client.h"
#include <time.h>
#include <fstream>
#include <string.h>
#include <tchar.h>
#include <windows.h>

#define WS_OP_TXT 129
#define WS_OP_BIN 130
net_server_serverClass server;
struct clientS {
    byteArray recvBuffer;
    byteArray waitingBuffer;
    string ip;
    wstring appname;
};
vector<clientS> clientList;
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
    for(unsigned int i = 0;i < str.size();i++) {
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
byteArray wsPing()
{
    byteArray toRet;
    toRet.push_back(137);
    toRet.push_back(0);
    toRet.push_back(0);
    return toRet;
}
string translate_s1_to_s2(string str) // ABCDE -> _A_B_C_D_E
{
    string toRetStr = "";
    for(int i = 0;i < str.size();i++) {
        toRetStr.push_back(0);
        toRetStr.push_back(str[i]);
    }
    return toRetStr;
}
string translate_ws_to_s1(wstring str) // ABCD(Unicode char) -> ABCD*
{
    string toRetStr = "";
    for(int i = 0;i < str.size();i++) {
        if(int(str[i]) > 255) toRetStr.push_back('*');
        else toRetStr.push_back(str[i]);
    }
    return toRetStr;
}
string translate_ws_to_s2(wstring str)
{
    string toRetStr = "";
    for(int i = 0;i < str.size();i++) {
        toRetStr.push_back(int(str[i])>>8);
        toRetStr.push_back(int(str[i])%256);
    }
    return toRetStr;
}
wstring translate_s2_to_ws(string str) // _A_B_C_DXX -> ABCDX
{
    return L"";
}
string translate_ws_to_url(wstring str) // ABCD -> %..%..%..%..
{   /// !!! Thai language hack only , other language will not work at all ( not even work for thai now )
    string toRetStr = "";
    for(int i = 0;i < str.size();i++) {
        if(int(str[i]) <= 255) {
            if(int(str[i]) == 32) toRetStr.push_back('+');
            else {
                toRetStr.push_back(str[i]);
            }
        }
        else {
            toRetStr.append("%E0");
        }
        //toRetStr.push_back()
        /*if(int(str[i]) > 255) toRetStr.push_back('*');
        else toRetStr.push_back(str[i]);
        cout << int(str[i]) << ",";*/
    }
    //cout << "TOURL : " << toRetStr << endl;
    return toRetStr;
}
struct packetData {
    char enc;
    string opcode;
    wstring data;
    string trimData;
};
bool wsDecodeMsg(clientS& clientData,packetData& pData)
{
    //cout << "START DECODE MSG FOR " << id << endl;
    pData.data = L"";
    pData.opcode = pData.trimData = "";
    int meta[6];
    meta[0] = clientData.recvBuffer[0]; // type & everything
    meta[1] = clientData.recvBuffer[1] & 127;
    int nowPtr = 2;
    int metaLen = 6;
    unsigned long long sz = 0;
    if(meta[1] == 126) { // additional 2 bytes will be used for length
        for(int i = 2;i < 4;i++) {sz<<=8; sz+=clientData.recvBuffer[i];}
        nowPtr = 4;
        metaLen = 8;
    }
    else if(meta[1] == 127) { // additional 8 bytes will be used for length
        for(int i = 2;i < 10;i++) {sz<<=8; sz+=clientData.recvBuffer[i];}
        nowPtr = 10;
        metaLen = 14;
    }
    else {
        sz = meta[1];
    }
    for(int i = 0;i < 4;i++) {
        meta[i+2] = clientData.recvBuffer[nowPtr++];
    }
    // split message with sz
    if((clientData.recvBuffer.size()-metaLen)<sz) {
        cout << "SIZE IS WRONG !!!" << endl;
        clientData.recvBuffer.clear();
        return false;
    }
    int charcode = 0;
    int packetFIN = meta[0]>>7;
    int packetOP = meta[0]&15;

    byteArray thisChunk; // finalData = 1 piece of message
    thisChunk.assign(clientData.recvBuffer.begin(),clientData.recvBuffer.begin()+sz+metaLen);
    clientData.recvBuffer.erase(clientData.recvBuffer.begin(),clientData.recvBuffer.begin()+sz+metaLen);

    if(packetOP&8) {
        cout << "[Control Frame]" << endl;
        if(packetOP == 8) { /// opcode for closing connection
            // reading closing code
            int closingCode = ((cvt(thisChunk[nowPtr])^meta[(nowPtr-metaLen)%4+2])<<8) + (cvt(thisChunk[nowPtr+1])^meta[(nowPtr-metaLen+1)%4+2]);
            pData.data.push_back(wchar_t(closingCode));
            pData.opcode = "CLOS";
            return true;
        }
    }
    while(nowPtr < thisChunk.size()) { // demasked data and append to recvBuffer
        charcode = cvt(thisChunk[nowPtr])^meta[(nowPtr-metaLen)%4 + 2];
        clientData.waitingBuffer.push_back(charcode);
        nowPtr++;
    }
    if(packetFIN == 0) {
        cout << "FIN = 0 ";
        if(packetOP == 0) {
            cout << "<Continuation> (OP 0)" << endl;
        }
        else {
            cout << "<Start point> with OP " << packetOP << endl;
        }
        return false;
    }
    byteArray finalData; // finalData = 1 piece of message
    finalData.assign(clientData.waitingBuffer.begin(),clientData.waitingBuffer.end());
    clientData.waitingBuffer.clear();

    // use final data as demasked data , parsing with packet spec
    nowPtr = 0;
    int encodingType = finalData[nowPtr++];
    pData.enc = char(encodingType);
    int c;
    while((c = finalData[nowPtr]) != int('|')) {
        pData.opcode.push_back(char(c));
        nowPtr++;
        if(nowPtr >= finalData.size()) {
            return false; // invalid packet spec
        }
    }
    nowPtr++; // skip |
    if(encodingType == int('1')) { // 1-byte encoding ( normal string )
        while(nowPtr < finalData.size()) {
            charcode = finalData[nowPtr];
            pData.data.push_back(wchar_t(charcode));
            pData.trimData.push_back(charcode);
            nowPtr++;
        }
    }
    else if(encodingType == int('2')) { // 2-bytes encoding ( wstring )
        while(nowPtr < finalData.size()) {
            if(nowPtr == finalData.size()-1) break; // shouldn't happen
            charcode = (finalData[nowPtr]<<8) + finalData[nowPtr+1];
            pData.data.push_back(wchar_t(charcode));
            pData.trimData.push_back(charcode>>8);
            pData.trimData.push_back(charcode%256);
            nowPtr += 2;
        }
    }
    //cout << "Decode successful , waitingBuffer : " << clientData.waitingBuffer.size() << " , recvBuffer : " << clientData.recvBuffer.size() << endl;
    return true;
}
byteArray wsEncodeMsg(string opcode,string data,int type=WS_OP_TXT,char encodingType='2')
{
    byteArray toRet;
    unsigned long long int sz = data.size()+opcode.size()+1+1; // DATA , OPCODE , | , ENC
    toRet.push_back(type);
    if(sz <= 125) {
        // normal len
        toRet.push_back(sz);
    }
    else if(sz <= 65535) {
        // 2+ len
        toRet.push_back(126);
        toRet.push_back((sz >> 8) &255);
        toRet.push_back(sz &255);
    }
    else {
        toRet.push_back(127);
        toRet.push_back((sz >> 56) &255);
        toRet.push_back((sz >> 48) &255);
        toRet.push_back((sz >> 40) &255);
        toRet.push_back((sz >> 32) &255);
        toRet.push_back((sz >> 24) &255);
        toRet.push_back((sz >> 16) &255);
        toRet.push_back((sz >> 8) &255);
        toRet.push_back(sz &255);
    }
    /// DATA area
    toRet.push_back(cvt(encodingType));
    toRet.insert(toRet.end(),opcode.begin(),opcode.end());
    toRet.push_back('|');
    toRet.insert(toRet.end(),data.begin(),data.end());
    return toRet;
}
wstring retr(wstring dir)
{
    wstring path(dir.begin(),dir.end());
    path.append(L"\\*");
    WIN32_FIND_DATAW FindData;
    HANDLE hFind;
    hFind = FindFirstFileW( path.c_str(), &FindData );
    wstring toRet = dir;
    toRet.push_back(wchar_t('|'));
    if( hFind == INVALID_HANDLE_VALUE ) {
        return L"";
    }
    do
    {
        if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            toRet.append(L">");
            toRet.append(FindData.cFileName);
        }
        else {
            toRet.append(FindData.cFileName);
        }
        toRet.push_back('|');
    }
    while( FindNextFileW(hFind, &FindData) > 0 );
    if( GetLastError() != ERROR_NO_MORE_FILES ) {
        cout << "Something went wrong during searching\n";
    }
    return toRet;
}

int cntDown = 0;
string driveList_old;
string getDriveList()
{
    string toRet = "";
    char dLetter = 'A';
    DWORD dMask = GetLogicalDrives();
    while(dMask) {
        if(dMask & 1) {
            string getType = "";
            getType.push_back(dLetter);
            getType.append(":\\");
            int res = GetDriveType(getType.c_str());
            if(res != DRIVE_UNKNOWN && res != DRIVE_NO_ROOT_DIR) {
                char str[1000];
                if(GetVolumeInformation(getType.c_str(),str,1000,NULL,NULL,NULL,NULL,0) != 0) {
                    toRet.push_back(dLetter);
                    toRet.push_back('|');
                }
            }
        }
        dLetter++;
        dMask >>= 1;
    }
    return toRet;
}

bool sp = false;
bool rs = false;
net_client_clientClass youtubeClient;
bool youtubeDataFound = false;
int youtubeStat = -1;
string youtubeRecvData = "";
void err(string str) {
    cout << "Error : " << str << endl;
}
void debug(string str) {
    cout << "DBG : " << str << endl;
}

void youtubeRecv(byteArray bt)
{
    youtubeRecvData.append(toString(bt));
    int res = youtubeRecvData.find("section-list");
    if(!youtubeDataFound && res != string::npos) {
        res = youtubeRecvData.find("yt-lockup-title",res);
        if(res != string::npos) {
            res = youtubeRecvData.find("href=",res);
            if(res != string::npos) {
                string last = youtubeRecvData.substr(res+15,11);
                if(last.size() == 11) {
                    cout << "Youtube search found [" << last << "]" << endl;
                    server.sendTo(wsEncodeMsg("YOUT",last,WS_OP_TXT,'1'),youtubeStat);
                    youtubeDataFound = true;
                    youtubeStat = -1;
                    youtubeClient.disconnect();
                }
            }
        }
    }
}
void run() {
    if(GetAsyncKeyState(VK_SPACE) && !sp) {
        sp = true;
    }
    else if(!GetAsyncKeyState(VK_SPACE) && sp) sp = false;
    if(GetAsyncKeyState(VK_RSHIFT) && !rs) {
        rs = true;
    }
    else if(!GetAsyncKeyState(VK_RSHIFT) && rs) rs = false;

    cntDown++;
    youtubeClient.run();
    if(cntDown >= 120) {
        // check new drive
        string driveList_new = getDriveList();
        if(driveList_new.compare(driveList_old) != 0) {
            server.sendToAllClient(wsEncodeMsg("DRIVE",driveList_new,WS_OP_TXT,'1'));
            cout << "Drive list changed : " << driveList_new << endl;
            driveList_old = driveList_new;
        }
        cntDown = 0;
    }
}
void recv(byteArray data,int i) {
    string str = toString(data);
    cout << "Recieved data from " << i << " (" << clientList[i].ip << ")" << endl;
    if(str.find("GET / HTTP/1.1") != string::npos) { // is HTML request
        if(str.find("Upgrade: websocket") != string::npos) { // is WebSocket handshake
            //cout << "Handshake data : " << endl << str << endl;
            cout << "  " << "Websocket handshake" << endl;
            server.sendTo(wsHandshake(str),i);
            cout << "    " << "Handshaked with " << i << " (" << server.getIpFrom(i) << ")" << endl;
        }
        else {
            // HTML request
        }
    }
    else {
        packetData pData;
        // append data to recvBuffer
        clientList[i].recvBuffer.insert(clientList[i].recvBuffer.end(),data.begin(),data.end());
        while(clientList[i].recvBuffer.size() > 0) {
            if(wsDecodeMsg(clientList[i],pData) == false) continue; // do nothing
            string opcode = pData.opcode;
            wstring decodeMsg = pData.data;
            cout << "  " << "OP [" << opcode.substr(0,15) << "] DATA [" << translate_ws_to_s1(decodeMsg.substr(0,20)) << "] SIZE [" << decodeMsg.size() << "]" << endl;
            if(opcode.compare("CLOS")==0) {
                int closingCode = int(decodeMsg[0]);
                cout << "    " << "<CLOS> " << closingCode << endl;
                server.disconnect(i);
            }
            else {
                if(opcode.compare("RETR") == 0) { /// RETR [DIR] : retrieve file list at DIR
                    cout << "    " << "<RETR>" << endl;
                    wstring toRet = retr(decodeMsg);
                    server.sendTo(wsEncodeMsg("DIRLST",translate_ws_to_s2(toRet),WS_OP_TXT,'2'),i);
                }
                else if(opcode.compare("EXEC") == 0) { /// EXEC [PATH] : Execute file
                    cout << "    " << "<EXEC>" << endl;
                    wstring fullpath = decodeMsg;
                    wstring dir = fullpath.substr(0,fullpath.find_last_of(L"/\\"));
                    wcout << L"    " << L"Executing " << dir << endl;
                    ShellExecuteW(NULL, NULL, fullpath.c_str(), NULL, dir.c_str(), SW_SHOWNORMAL);
                }
                else if(opcode.compare("RETD") == 0) { /// RETD [] : retrieve drive letter
                    cout << "    " << "<RETD>" << endl;
                    server.sendTo(wsEncodeMsg("DRIVE",driveList_old,WS_OP_TXT,'1'),i);
                }
                else if(opcode.compare("READ") == 0) { /// READ [PATH] : read file
                    cout << "    " << "<READ>" << endl;
                    wstring fullpath = decodeMsg;
                    FILE* f = _wfopen(fullpath.c_str(),L"rb");
                    if(f == NULL) return; // file not exist
                    wstring toSendH = fullpath;
                    toSendH.append(L"|");
                    string realData = translate_ws_to_s2(toSendH);
                    char c;
                    int sizeCnt = 0;
                    c=fgetc(f);
                    while(!feof(f)) {
                        realData.push_back(cvt(c));
                        sizeCnt++;
                        c=fgetc(f);
                    }
                    cout << "    " << "File size : " << sizeCnt << endl;
                    if(ferror(f) != 0) cout << "    " << "File read error code : " << ferror(f) << endl;
                    fclose(f);
                    server.sendTo(wsEncodeMsg("FILE",realData,WS_OP_BIN,'2'),i);
                }
                else if(opcode.compare("YOUT") == 0) { /// YOUT [QUERY] : Youtube Music search
                    cout << "    " << "<YOUT>" << endl;
                    wstring query = decodeMsg;
                    if(youtubeClient.connect("www.youtube.com",80,200)) {
                        string httpReq = "GET /results?search_query=";
                        httpReq.append(translate_ws_to_url(query));
                        httpReq.append(" HTTP/1.1\n");
                        httpReq.append("Host: www.youtube.com\n");
                        httpReq.append("User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64; rv:27.0) Gecko/20100101 Firefox/27.0\n\n");
                        youtubeClient.send(toByteArray(httpReq));
                        youtubeRecvData = "";
                        youtubeDataFound = false;
                        youtubeStat = i;
                        wcout << L"    " << L"Youtube query request [" << query << L"] sent" << endl;
                    }
                }
                else if(opcode.compare("SAVE") == 0) { /// SAVE [PATH | BINARYDATA] : save binary data to file
                    cout << "    " << "<SAVE>" << endl;
                    wstring param = decodeMsg;
                    int splitPos = param.find(L"|");
                    if(splitPos == string::npos) continue; // not correct packet
                    wstring filepath = param.substr(0,splitPos);
                    string binaryData = pData.trimData.substr((filepath.size()+1) * (pData.enc=='2'?2:1));
                    cout << "    " << "File will be save to " << translate_ws_to_s1(filepath) << endl;
                    cout << "    " << "File size : " << binaryData.size() << endl;
                    cout << "    " << "File begin : " << hex(binaryData.substr(0,20)) << endl;
                    FILE* f = _wfopen(filepath.c_str(),L"wb");
                    // add data to file
                    for(int i = 0;i < binaryData.size();i++) fputc(binaryData[i],f);
                    fclose(f);
                }
                else if(opcode.compare("ERROR") == 0) {
                    cout << "    " << "<ERROR>" << endl;
                }
                else if(opcode.compare("SERVERCLOSE") == 0) {
                    // server close
                    cout << "    " << "<SERVERCLOSE>" << endl;
                    server.stop();
                }
                else if(opcode.compare("REGIS") == 0) { /// REGIS [APPNAME] : Register app's name to server
                    cout << "    " << "<REGIS>" << endl;
                    wstring appname = decodeMsg;
                    cout << "    " << "Registering appname : " << translate_ws_to_s1(appname) << endl;
                    // check exists appname
                    for(int j = 0;j < clientList.size();j++) {
                        if(i != j && appname.compare(clientList[j].appname) == 0) {
                            cout << "    " << "Name already exists by " << j << endl;
                            server.sendTo(wsEncodeMsg("REGISSTAT","FAIL",WS_OP_TXT,'1'),i);
                            continue;
                        }
                    }
                    server.sendTo(wsEncodeMsg("REGISSTAT","SUCCESS",WS_OP_TXT,'1'),i);
                    clientList[i].appname = decodeMsg;
                }
                else if(opcode.compare("RELAY") == 0) { /// RELAY [APPNAME|DATA] : Relay message to other app
                    cout << "    " << "<RELAY>" << endl;
                    int splitPos = decodeMsg.find(L"|");
                    if(splitPos == string::npos) continue; // not correct packet
                    wstring appname = decodeMsg.substr(0,splitPos);
                    string data = translate_ws_to_s2(decodeMsg.substr(splitPos+1));
                    bool targFound = false;
                    for(int j = 0;j < clientList.size();j++) {
                        if(appname.compare(clientList[j].appname) == 0) {
                            cout << "    " << "Will relay to client " << j << endl;
                            server.sendTo(wsEncodeMsg("RELAY",data,WS_OP_TXT,'2'),j);
                            server.sendTo(wsEncodeMsg("RELAYSTAT","SUCCESS",WS_OP_TXT,'1'),i);
                            targFound = true;
                            break;
                        }
                    }
                    if(targFound) continue;
                    cout << "    " << "Target not found" << endl;
                    server.sendTo(wsEncodeMsg("REGISSTAT","FAIL",WS_OP_TXT,'1'),i);
                }
                else {
                    cout << "    " << "?? Unknown opcode ??" << endl;
                }
            }
        }
    }
}
bool acc(net_ext_sock connector)
{
    cout << "ClientList add " << net_getIpFromHandle(connector.sockHandle) << " as id " << clientList.size() << endl;
    clientS clientData;
    clientData.ip = net_getIpFromHandle(connector.sockHandle);
    clientList.push_back(clientData);
    return true;
}
void dis(int id)
{
    cout << "ID " << id << " disconnected" << endl;
    clientList.erase(clientList.begin() + id);
}
void startServer()
{
    server.init();
    server.setup(80,run,recv,err,acc,dis);
    server.setDebugFunc(debug);
    server.start();
    server.runLoop();
}
int main()
{
    net_init();
    SetErrorMode(SEM_NOOPENFILEERRORBOX); // for drives detection
    youtubeClient.setDebugFunc(debug);
    youtubeClient.setup(NULL,youtubeRecv,err);
    startServer();
    net_close();
    return 0;
}
