#define WINVER 0x0601
#include "network_core.h"
#include "network_server.h"
#include "network_client.h"
#include <time.h>
#include <fstream>
#include <string.h>
#include <tchar.h>
#include <windows.h>
#include <Psapi.h>
#include <string>
#include <cstdlib>
#include <wchar.h>
#include <sstream>
#include <map>
#include <algorithm>

#define WS_OP_TXT 129 // WebSocket Opcode "Text" data
#define WS_OP_BIN 130 // WebSocket Opcode "Binary" data

#define TIMESTAMP TRUE // Use timestamp in debugging
#define LOG_CONN TRUE // Connection ( Websocket handshake , id assignment )
#define LOG_SERVDBG TRUE // Server ( Connect , Accept , Disconnect )
#define LOG_PARSE FALSE // Parse ( WebSocket packet parsing )
#define LOG_PACKET FALSE // Packet ( Arrival , Info )
#define LOG_OS TRUE // OS ( OS command )
#define LOG_HTTP FALSE // HTTP ( HTTP request )

/// DEFINITION
net_server_serverClass server;
struct clientS {
    byteArray recvBuffer;
    byteArray waitingBuffer;
    string ip;
    wstring appname;
    int socket_idRunner;
    map<int,net_client_clientClass> socketList;
};
struct packetData {
    char enc;
    string opcode;
    string id;
    wstring data;
    string trimData;
};
vector<clientS> clientList;
map<string,vector<int> > clientFlag;
string workingDir;

map<string,string> contentTypeMap;

vector<INPUT> inputQueue;
/*  Flag list
FLAG_DRIVECHANGE = Notified ( DRIVELST ) when server's physical drive list changed
FLAG_AUTOPONG = Send a PONG message every 15 ticks ( about 0.5 sec )
*/
/// STATIC VAR , ASSIGNMENT
void initContentTypeMap()
{
    // Text
    contentTypeMap[".html"] = "text/html";
    contentTypeMap[".js"] = "application/javascript";
    contentTypeMap[".css"] = "text/css";
    contentTypeMap[".txt"] = "text/plain";
    // Image
    contentTypeMap[".bmp"] = "image/bmp";
    contentTypeMap[".gif"] = "image/gif";
    contentTypeMap[".jpg"] = "image/jpeg";
    contentTypeMap[".jpeg"] = "image/jpeg";
    contentTypeMap[".png"] = "image/png";
    // Audio
    contentTypeMap[".mp3"] = "audio/mpeg";
    contentTypeMap[".ogg"] = "audio/ogg";
    contentTypeMap[".wav"] = "audio/vnd.wave";
    // Video
    contentTypeMap[".mp4"] = "video/mp4";
    contentTypeMap[".avi"] = "video/avi";
    contentTypeMap[".wmv"] = "video/x-ms-wmv";
}
/// LOGGING FUNCTION
int timestamp;
string reqTimestamp() {
    if(TIMESTAMP) {
        string str = "{";
        char timestampStr[100];
        sprintf(timestampStr,"%06d",timestamp);
        str.append(timestampStr);
        str.append("}");
        return str;
    }
    else {
        return "";
    }
}
string logHeader(string module,int leadingSpace = 0) {
    stringstream ss;
    for(int i = 0;i < leadingSpace;i++) ss << " ";
    ss << reqTimestamp() << " " << "[" << module << "]" << " ";
    return ss.str();
}
void error(string str) {
    cout << logHeader("ERROR") << str << endl;
}
void debug(string str) {
    cout << logHeader("SERVDBG") << str << endl;
}
bool isLogEnable(string module)
{
    if(module.compare("CONN")==0 && !LOG_CONN) return false;
    else if(module.compare("PARSE")==0 && !LOG_PARSE) return false;
    else if(module.compare("PACKET")==0 && !LOG_PACKET) return false;
    else if(module.compare("SERVDBG")==0 && !LOG_SERVDBG) return false;
    else if(module.compare("OS")==0 && !LOG_OS) return false;
    return true;
}
/// FUNCTION
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
string translate_ws_to_s2(wstring str) // ABCDX -> _A_B_C_DXX
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
    /// NOT TEST
    wstring toRet;
    for(int i = 0;i < str.size();i+=2) {
        if(i == str.size()-1) toRet.push_back(wchar_t(str[i]));
        else toRet.push_back(wchar_t(str[i]<<8+str[i+1]));
    }
    return toRet;
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
    }
    return toRetStr;
}
/// PROGRAM FUNCTION
string wsHandshake(string str)
{
    /* HTTP/1.1 101 Switching Protocols
    Upgrade: websocket
    Connection: Upgrade
    Sec-WebSocket-Accept: HSmrc0sMlYUkAGmm5OPpG2HaGWk=

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
byteArray wsPingMsg()
{
    byteArray toRet;
    toRet.push_back(137);
    toRet.push_back(0);
    toRet.push_back(0);
    return toRet;
}
bool wsDecodeMsg(clientS& clientData,packetData& pData)
{
    pData.data = L"";
    pData.opcode = pData.trimData = pData.id = "";
    int meta[6];
    meta[0] = clientData.recvBuffer[0]; // type & everything
    meta[1] = clientData.recvBuffer[1] & 127; // length + masked
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
        if(isLogEnable("PARSE")) cout << logHeader("PARSE") << "BufferSize - metaLen < sz , data might be incompleted" << endl;
        return false;
    }
    int charcode = 0;
    int packetFIN = meta[0]>>7;
    int packetOP = meta[0]&15;

    byteArray thisChunk; // finalData = 1 piece of message
    thisChunk.assign(clientData.recvBuffer.begin(),clientData.recvBuffer.begin()+sz+metaLen);
    clientData.recvBuffer.erase(clientData.recvBuffer.begin(),clientData.recvBuffer.begin()+sz+metaLen);

    if(packetOP&8) {
        if(isLogEnable("PARSE")) cout << logHeader("PARSE") << "[Control Frame]" << endl;
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
        if(isLogEnable("PARSE")) cout << logHeader("PARSE") << "FIN = 0 ";
        if(packetOP == 0) {
            if(isLogEnable("PARSE")) cout << logHeader("PARSE") << "<Continuation> (OP 0)" << endl;
        }
        else {
            if(isLogEnable("PARSE")) cout << logHeader("PARSE") << "<Start point> with OP " << packetOP << endl;
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
            if(isLogEnable("PARSE")) cout << logHeader("PARSE") << "invalid PalmOS packet , OPCODE not fin" << endl;
            return false; // invalid packet spec
        }
    }
    nowPtr++; // skip |
    while((c = finalData[nowPtr]) != int('|')) {
        pData.id.push_back(char(c));
        nowPtr++;
        if(nowPtr >= finalData.size()) {
            if(isLogEnable("PARSE")) cout << logHeader("PARSE") << "invalid PalmOS packet , ID not fin" << endl;
            return false; // invalid packet spec
        }
    }
    nowPtr++; // skip second |
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
    return true;
}
byteArray wsEncodeMsg(string opcode,string data,string id="0",int type=WS_OP_TXT,char encodingType='2')
{
    byteArray toRet;
    unsigned long long int sz = data.size()+opcode.size()+1+1+id.size()+1; // ENC OPCODE | ID | DATA
    toRet.push_back(type);
    if(sz <= 125) { // normal len
        toRet.push_back(sz);
    }
    else if(sz <= 65535) { // 2 bytes len
        toRet.push_back(126);
        toRet.push_back((sz >> 8) &255);
        toRet.push_back(sz &255);
    }
    else { // 8 bytes len
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
    // DATA area
    toRet.push_back(cvt(encodingType));
    toRet.insert(toRet.end(),opcode.begin(),opcode.end());
    toRet.push_back('|');
    toRet.insert(toRet.end(),id.begin(),id.end());
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

string reqClientInfo(int id) {
    stringstream ss;
    ss << " [[" << id << " " << clientList[id].ip;
    if(clientList[id].appname.size() > 0) ss << " " << translate_ws_to_s1(clientList[id].appname);
    ss << "]] ";
    return ss.str();
}
BOOL isGoodWindow(HWND hwnd)
{
    HWND hwndTry, hwndWalk = NULL;
    if(!IsWindowVisible(hwnd)) return FALSE;
    hwndTry = GetAncestor(hwnd, GA_ROOTOWNER);
    while(hwndTry != hwndWalk) {
        hwndWalk = hwndTry;
        hwndTry = GetLastActivePopup(hwndWalk);
        if(IsWindowVisible(hwndTry)) break;
    }
    if(hwndWalk != hwnd) return FALSE;
    if(GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) return FALSE;
    return TRUE;
}
wstring progListBuff;
BOOL CALLBACK EnumWindowsCB(HWND hwnd, LPARAM lParam) {
    char class_name[80];
	char title[80];
	GetClassName(hwnd,class_name, sizeof(class_name));
	GetWindowText(hwnd,title,sizeof(title));
	if(!isGoodWindow(hwnd)) return true;
	cout <<title << "," << class_name << endl;
	// retr process id
	DWORD procID;
    GetWindowThreadProcessId(hwnd,&procID);
    //cout << "    PID : " << procID << endl;
    HANDLE procH = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,FALSE,procID);
    if(procH) {
        wchar_t buff[10000];
        if(GetModuleFileNameExW(procH, NULL, buff, 10000))
        {
            wstring str = buff;
            if(progListBuff.size() > 0) progListBuff.append(L"|");
            progListBuff.append(str);
            // try to get file ver
            /*DWORD fileVerLen = GetFileVersionInfoSize(buff,NULL);
            void* data = NULL;
            GetFileVersionInfo(buff,0,fileVerLen,data);*/
        }
        else
        {
            cout << "    Error : " << GetLastError() << endl;
        }
        CloseHandle(procH);
    }
    else {
        cout << "    WTF Error : " << GetLastError() << endl;
    }
    string titl = title;
    if(titl.find("Code::Blocks") != string::npos) {
        /*ShowWindow(hwnd,SW_RESTORE);
        SetActiveWindow(hwnd);
        SetForegroundWindow(hwnd); // test set focus to code::blocks
        INPUT    Input={0};*/
        // left down
        // try send 3 spacebar
    }
    return true;
}
// testareahere012345678901234567890123456789
/// MAIN LOOP FUNCTION
void run() {
    timestamp++;
    if(GetAsyncKeyState(VK_SPACE) && !sp) {
        sp = true;
    }
    else if(!GetAsyncKeyState(VK_SPACE) && sp) sp = false;
    if(GetAsyncKeyState(VK_RSHIFT) && !rs) {
        rs = true;
    }
    else if(!GetAsyncKeyState(VK_RSHIFT) && rs) rs = false;

    cntDown++;
    if(cntDown % 15 == 0) {
        vector<int> *idListToSend = &clientFlag["FLAG_AUTOPONG"];
        for(int i = 0;i < idListToSend->size();i++) {
            server.sendTo(wsEncodeMsg("PONG","","0",WS_OP_TXT,'1'),idListToSend->at(i));
        }
        // recv
        for(int i = 0;i < clientList.size();i++) {
            map<int,net_client_clientClass>::iterator it = clientList[i].socketList.begin();
            while(it != clientList[i].socketList.end()) {
                // hack deep into client
                //cout << "TRY RECVING" << endl;
                byteArray recvD;
                int stat = (it->second).recvData(recvD);
                if(stat == 1) {
                    stringstream ss;
                    ss << "RESP|" << (it->first) << "|" << toString(recvD);
                    //cout << "Data recieved size : " << recvD.size() << endl;
                    server.sendTo(wsEncodeMsg("SOCKETSTAT",ss.str(),"0",WS_OP_TXT,'1'),i);
                }
                else if(stat == 2) {
                    // disconnect
                    stringstream ss;
                    ss << "CLOSE|" << (it->first);
                    server.sendTo(wsEncodeMsg("SOCKETSTAT",ss.str(),"0",WS_OP_TXT,'1'),i);
                }
                it++;
            }
        }
    }
    if(cntDown >= 120) {
        // check new drive
        string driveList_new = getDriveList();
        if(driveList_new.compare(driveList_old) != 0) {
            vector<int> *idListToSend = &clientFlag["FLAG_DRIVECHANGE"];
            for(int i = 0;i < idListToSend->size();i++) {
                server.sendTo(wsEncodeMsg("DRIVE",driveList_new,"0",WS_OP_TXT,'1'),idListToSend->at(i));
            }
            if(isLogEnable("OS")) cout << logHeader("OS") << "Drive list changed : " << driveList_new << endl;
            driveList_old = driveList_new;
        }
        cntDown = 0;
    }
    if(inputQueue.size() > 0) {
        ::SendInput(inputQueue.size(),&inputQueue[0],sizeof(INPUT));
        inputQueue.clear();
    }
}
void recv(byteArray data,int i) {
    string str = toString(data);
    if(str.find("GET /") == 0 && str.find("HTTP/1.1") != string::npos) {
        if(str.find("Upgrade: websocket") != string::npos) { // WebSocket handshake
            server.sendTo(wsHandshake(str),i);
            if(isLogEnable("CONN")) cout << logHeader("CONN") << "Websocket connected with " << reqClientInfo(i) << endl;
        }
        else { // HTTP request
            string pageRequest = str.substr(5,str.find("HTTP/1.1")-6);
            if(isLogEnable("HTTP")) cout << logHeader("HTTP") << reqClientInfo(i) << "HTTP req '" << pageRequest << "'" << endl;
            if(pageRequest.size() == 0) pageRequest = "index.html"; // default page
            string fullpath = workingDir;
            fullpath.append("\\");
            fullpath.append(pageRequest);
            string response;
            if(pageRequest.find_last_of('.') == string::npos) {
                cout << "Invalid resource indication" << endl;
                response = "HTTP/1.1 404 Not Found\r\n";
                response.append("\r\n");
            }
            else {
                string extension = pageRequest.substr(pageRequest.find_last_of('.'));
                string contentType = "";

                FILE* f = fopen(fullpath.c_str(),"rb");
                if(f == NULL) {
                    response = "HTTP/1.1 404 Not Found\r\n";
                    response.append("\r\n");
                    if(isLogEnable("HTTP")) cout << logHeader("HTTP") << reqClientInfo(i) << " 404 " << endl;
                }
                else {
                    response = "HTTP/1.1 200 OK\r\n";
                    response.append("Content-Type: ");
                    if(contentTypeMap.count(extension) == 0) response.append("application/octet-stream"); // byte array of unknown thing
                    else response.append(contentTypeMap[extension]);
                    response.append("\r\n\r\n");
                    char c;
                    c=fgetc(f);
                    while(!feof(f)) {
                        response.push_back(cvt(c));
                        c=fgetc(f);
                    }
                    if(ferror(f) != 0) {
                        if(isLogEnable("HTTP")) cout << logHeader("HTTP") << reqClientInfo(i) << "HTTP response read error : " << ferror(f) << endl;
                        response = "HTTP/1.1 404 Not Found\r\n";
                    }
                    else {
                        if(isLogEnable("HTTP")) cout << logHeader("HTTP") << reqClientInfo(i) << "200 OK" << endl;
                    }
                    fclose(f);
                }
            }
            server.sendTo(response,i);
            server.disconnect(i);
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
            if(isLogEnable("PACKET")) cout << reqTimestamp() << logHeader("PACKET") << reqClientInfo(i) << " OP[" << opcode.substr(0,15) << "] PID[" << pData.id << "] DATA[" << translate_ws_to_s1(decodeMsg.substr(0,20)) << "] SIZE[" << decodeMsg.size() << "]" << endl;
            if(opcode.compare("CLOS")==0) {
                int closingCode = int(decodeMsg[0]);
                if(isLogEnable("PACKET")) cout << logHeader("PACKET",2) << "Close packet " << reqClientInfo(i) << " code " << closingCode << endl;
                server.disconnect(i);
            }
            else {
                if(opcode.compare("RETR") == 0) { /// RETR [DIR] : retrieve file list at DIR
                    if(isLogEnable("OS")) cout << logHeader("OS",2) << reqClientInfo(i) << "RETR" << endl;
                    wstring toRet = retr(decodeMsg);
                    server.sendTo(wsEncodeMsg("DIRLST",translate_ws_to_s2(toRet),pData.id,WS_OP_TXT,'2'),i);
                }
                else if(opcode.compare("EXEC") == 0) { /// EXEC [PATH] : Execute file
                    wstring fullpath = decodeMsg;
                    wstring dir = fullpath.substr(0,fullpath.find_last_of(L"/\\"));
                    if(isLogEnable("OS")) cout << logHeader("OS",2) << reqClientInfo(i) << "EXEC" << endl;
                    ShellExecuteW(NULL, NULL, fullpath.c_str(), NULL, dir.c_str(), SW_SHOWNORMAL);
                }
                else if(opcode.compare("RETD") == 0) { /// RETD [] : retrieve drive letter
                    if(isLogEnable("OS")) cout << logHeader("OS",2) << reqClientInfo(i) << "RETD" << endl;
                    server.sendTo(wsEncodeMsg("DRIVE",driveList_old,pData.id,WS_OP_TXT,'1'),i);
                }
                else if(opcode.compare("READ") == 0) { /// READ [PATH] : read file
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
                    //cout << "    " << "File size : " << sizeCnt << endl;
                    if(ferror(f) != 0) {
                        //cout << "    " << "File read error code : " << ferror(f) << endl;
                        if(isLogEnable("OS")) cout << logHeader("OS",2) << reqClientInfo(i) << "READ " << translate_ws_to_s1(fullpath) << " , Error code " << ferror(f) << endl;
                    }
                    else {
                        if(isLogEnable("OS")) cout << logHeader("OS",2) << reqClientInfo(i) << "READ " << translate_ws_to_s1(fullpath) << " , size " << sizeCnt << endl;
                    }
                    fclose(f);
                    server.sendTo(wsEncodeMsg("FILE",realData,pData.id,WS_OP_BIN,'2'),i);
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
                else if(opcode.compare("SERVERCLOSE") == 0) {
                    // server close
                    if(isLogEnable("OS")) cout << logHeader("OS",2) << "SERVERCLOSE" << endl;
                    server.stop();
                }
                else if(opcode.compare("REGIS") == 0) { /// REGIS [APPNAME] : Register app's name to server
                    if(isLogEnable("OS")) cout << logHeader("OS",2) << reqClientInfo(i) << "REGIS" << endl;
                    wstring appname = decodeMsg;
                    if(isLogEnable("OS")) cout << logHeader("OS",4) << "Name : " << translate_ws_to_s1(appname) << endl;
                    // check exists appname
                    for(int j = 0;j < clientList.size();j++) {
                        if(i != j && appname.compare(clientList[j].appname) == 0) {
                            if(isLogEnable("OS")) cout << logHeader("OS",4) << "Name exists with " << reqClientInfo(j) << endl;
                            server.sendTo(wsEncodeMsg("REGISSTAT","FAIL",pData.id,WS_OP_TXT,'1'),i);
                            continue;
                        }
                    }
                    if(isLogEnable("OS")) cout << logHeader("OS",4) << "Register successful" << endl;
                    server.sendTo(wsEncodeMsg("REGISSTAT","SUCCESS",pData.id,WS_OP_TXT,'1'),i);
                    clientList[i].appname = decodeMsg;
                }
                else if(opcode.compare("RELAY") == 0) { /// RELAY [APPNAME|DATA] : Relay message to other app
                    if(isLogEnable("OS")) cout << logHeader("OS",2) << reqClientInfo(i) << "RELAY" << endl;
                    int splitPos = decodeMsg.find(L"|");
                    if(splitPos == string::npos) continue; // not correct packet
                    wstring appname = decodeMsg.substr(0,splitPos);
                    string data = translate_ws_to_s2(decodeMsg.substr(splitPos+1));
                    bool targFound = false;
                    for(int j = 0;j < clientList.size();j++) {
                        if(appname.compare(clientList[j].appname) == 0) {
                            if(isLogEnable("OS")) cout << logHeader("OS",4) << reqClientInfo(i) << "Will relay to " << reqClientInfo(j) << endl;
                            server.sendTo(wsEncodeMsg("RELAY",data,"0",WS_OP_TXT,'2'),j);
                            server.sendTo(wsEncodeMsg("RELAYSTAT","SUCCESS",pData.id,WS_OP_TXT,'1'),i);
                            targFound = true;
                            break;
                        }
                    }
                    if(targFound) continue;
                    if(isLogEnable("OS")) cout << logHeader("OS",4) << reqClientInfo(i) << "Target not found " << endl;
                    server.sendTo(wsEncodeMsg("RELAYSTAT","FAIL",pData.id,WS_OP_TXT,'1'),i);
                }
                else if(opcode.compare("INPUT") == 0) { /// INPUT [TYPE|Additional args ...] : Simulate input events
                    //if(INPUT_LOG) cout << "    " << "<INPUT>" << endl;
                    vector<wstring> args;
                    args.push_back(L"");
                    for(int i = 0;i < decodeMsg.size();i++) {
                        if(decodeMsg[i] == wchar_t('|')) args.push_back(L"");
                        else args[args.size()-1].push_back(decodeMsg[i]);
                    }
                    if(args[0].compare(L"MOUSEMOVE") == 0) { /// INPUT -> MOUSEMOVE|PIXEL X|PIXEL Y : Mouse move event
                        if(args.size() < 3) {cout << "    " << "!!! ARG SIZE NOT CORRECT" << endl; continue; }
                        INPUT Input = {0};
                        Input.type = INPUT_MOUSE;
                        Input.mi.dx = (LONG)_wtoi(args[1].c_str());
                        Input.mi.dy = (LONG)_wtoi(args[2].c_str());
                        //Input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
                        Input.mi.dwFlags = MOUSEEVENTF_MOVE;
                        //SendInput(1, &Input, sizeof(INPUT));
                        inputQueue.push_back(Input);
                        if(isLogEnable("OS")) cout << logHeader("OS") << reqClientInfo(i) << "INPUT MOUSEMOVE " << Input.mi.dx << "," << Input.mi.dy << endl;
                    }
                    else if(args[0].compare(L"MOUSESETPOS") == 0) { /// INPUT -> MOUSESETPOS|X|Y : Set cursor pos to X Y ( leave blank for unchange )
                        if(args.size() < 3) {cout << "    " << "!!! ARG SIZE NOT CORRECT" << endl; continue; }
                        POINT cursorCoord;
                        GetCursorPos(&cursorCoord);
                        if(args[1].size() != 0) {
                            cursorCoord.x = _wtoi(args[1].c_str());
                        }
                        else if(args[2].size() != 0) {
                            cursorCoord.y = _wtoi(args[2].c_str());
                        }
                        SetCursorPos(cursorCoord.x,cursorCoord.y);
                        if(isLogEnable("OS")) cout << logHeader("OS") << reqClientInfo(i) << "INPUT MOUSESET " << cursorCoord.x << "," << cursorCoord.y << endl;
                    }
                    else if(args[0].compare(L"MOUSECLICK") == 0) { /// INPUT -> MOUSECLICK|(L,R)+(_,U,D) : Mouse click event
                        if(args.size() < 2) {cout << "    " << "!!! ARG SIZE NOT CORRECT" << endl; continue; }
                        INPUT    Input={0};
                        Input.type      = INPUT_MOUSE;
                        if(args[1].compare(L"LD") == 0) Input.mi.dwFlags  = MOUSEEVENTF_LEFTDOWN;
                        else if(args[1].compare(L"LU") == 0) Input.mi.dwFlags  = MOUSEEVENTF_LEFTUP;
                        else if(args[1].compare(L"L") == 0) Input.mi.dwFlags  = MOUSEEVENTF_LEFTDOWN;
                        else if(args[1].compare(L"RD") == 0) Input.mi.dwFlags  = MOUSEEVENTF_RIGHTDOWN;
                        else if(args[1].compare(L"RU") == 0) Input.mi.dwFlags  = MOUSEEVENTF_RIGHTUP;
                        else if(args[1].compare(L"R") == 0) Input.mi.dwFlags  = MOUSEEVENTF_RIGHTDOWN;
                        else Input.mi.dwFlags  = MOUSEEVENTF_LEFTDOWN; // default
                        inputQueue.push_back(Input);

                        // immediate up (for click)
                        if(args[1].size() == 1) {
                            //::ZeroMemory(&Input,sizeof(INPUT));
                            if(args[1].compare(L"L") == 0)Input.mi.dwFlags  = MOUSEEVENTF_LEFTUP;
                            else if(args[1].compare(L"R") == 0)Input.mi.dwFlags  = MOUSEEVENTF_RIGHTUP;
                            else Input.mi.dwFlags  = MOUSEEVENTF_LEFTUP; // default
                            //::SendInput(1,&Input,sizeof(INPUT));
                            inputQueue.push_back(Input);
                        }
                        if(isLogEnable("OS")) cout << logHeader("OS") << reqClientInfo(i) << "INPUT MOUSECLICK " << endl;
                        //if(INPUT_LOG) cout << "    " << "Mouse clicked" << endl;
                    }
                    else if(args[0].compare(L"KEYBOARD") == 0) { /// INPUT -> KEYBOARD|KEYCODE(3-digit dec)+(U,D,P)+(_,E) : Keyboard event
                        if(args.size() < 2 || args[1].size() < 4) {cout << "    " << "!!! ARG SIZE NOT CORRECT" << endl; continue; }
                        int keycode = (args[1][0]-'0')*100+(args[1][1]-'0')*10+(args[1][2]-'0');
                        if(isLogEnable("OS")) cout << logHeader("OS") << reqClientInfo(i) << " KEYBOARD " << keycode << " , " << char(args[1][3]) << " , " << char((args[1].size()>4)?args[1][4]:'-') << endl;
                        INPUT    Input={0};
                        // left down
                        Input.type      = INPUT_KEYBOARD;
                        Input.ki.wVk = 0;
                        Input.ki.wScan = keycode;
                        //cout << MapVirtualKey(VK_LEFT, 0) << " &&& " << MapVirtualKey(VK_RIGHT, 0) << endl;
                        Input.ki.time = 0;
                        Input.ki.dwExtraInfo = 0;
                        if(args[1][3] == 'U') {
                            Input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
                            if(args[1].size() > 4 && args[1][4] == 'E') Input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
                        }
                        else {
                            Input.ki.dwFlags = KEYEVENTF_SCANCODE;
                            if(args[1].size() > 4 && args[1][4] == 'E') Input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
                        }
                        //::SendInput(1,&Input,sizeof(INPUT));
                        inputQueue.push_back(Input);
                        if(args[1][3] == 'P') {
                            Input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
                            if(args[1].size() > 4 && args[1][4] == 'E') Input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
                            //::SendInput(1,&Input,sizeof(INPUT));
                            inputQueue.push_back(Input);
                        }
                    }
                    else {
                        cout << "    " << "!!! Input type doesn't exist" << endl;
                    }
                }
                else if(opcode.compare("PING") == 0) {
                    if(isLogEnable("OS")) cout << logHeader("OS") << reqClientInfo(i) << "Ping !!" << endl;
                    server.sendTo(wsEncodeMsg("PONG","",pData.id,WS_OP_TXT,'1'),i);
                }
                else if(opcode.compare("REGFLAG") == 0) {
                    vector<string> args;
                    args.push_back("");
                    string translatedMsg = translate_ws_to_s1(decodeMsg);
                    for(int i = 0;i < translatedMsg.size();i++) {
                        if(translatedMsg[i] == '|') args.push_back("");
                        else args[args.size()-1].push_back(translatedMsg[i]);
                    }
                    if(isLogEnable("OS")) cout << logHeader("OS") << reqClientInfo(i) << "REGFLAG ";
                    for(int x = 0;x < args.size();x++) {
                        cout << args[x] << " ";
                        // if this flag doesn't exist , add it
                        if(clientFlag.count(args[x]) == 0) {
                            vector<int> emp;
                            clientFlag[args[x]] = emp;
                        }
                        if(std::find(clientFlag[args[x]].begin(), clientFlag[args[x]].end(), i) != clientFlag[args[x]].end()) continue; // already registered
                        clientFlag[args[x]].push_back(i);
                    }
                    cout << endl;
                }
                else if(opcode.compare("WINDOWLIST") == 0) {
                    if(isLogEnable("OS")) cout << logHeader("OS") << reqClientInfo(i) << "WINDOWLIST" << endl;
                    // Request every running programs
                    progListBuff = L""; // clear toRet buffer
                    EnumWindows(EnumWindowsCB, NULL);
                    server.sendTo(wsEncodeMsg("WINDOW",translate_ws_to_s2(progListBuff),pData.id,WS_OP_TXT,'2'),i);
                }
                else if(opcode.compare("SOCKET") == 0) {
                    //if(isLogEnable("OS")) cout << logHeader("OS",2) << reqClientInfo(i) << "SOCKET COMMAND" << endl;
                    // SOCKET
                    vector<string> args;
                    args.push_back("");
                    for(int i = 0;i < decodeMsg.size();i++) {
                        if(decodeMsg[i] == wchar_t('|')) args.push_back("");
                        else args[args.size()-1].push_back(char(decodeMsg[i]));
                    }
                    if(args[0].compare("CREATE") == 0) {
                        // create new socket , give an id
                        net_client_clientClass cl;
                        cl.setup(NULL,NULL,error);
                        //cl.setDebugFunc(debug);
                        clientList[i].socketList.insert(pair<int,net_client_clientClass>(clientList[i].socket_idRunner,cl));
                        stringstream toRet;
                        toRet << "OK" << clientList[i].socket_idRunner++;
                        server.sendTo(wsEncodeMsg("SOCKETSTAT",toRet.str(),pData.id,WS_OP_TXT,'1'),i);
                        if(isLogEnable("OS")) cout << logHeader("OS",4) << "CREATE ... OK , ID : " << clientList[i].socket_idRunner << endl;
                    }
                    else if(args[0].compare("CONNECT") == 0) { // SOCKET|CONNECT|[ID]|[HOST]|[PORT]
                        if(args.size() < 4) continue;
                        int id = (LONG)atoi(args[1].c_str());
                        int port = (LONG)atoi(args[3].c_str());
                        if(isLogEnable("OS")) cout << logHeader("OS",4) << "@"<<id<<" CONNECT " << args[2] << ":" << port << " ... ";
                        bool stat = clientList[i].socketList[id].connect(args[2],port,1); // allow 1 second freeze
                        if(stat) {
                            if(isLogEnable("OS")) cout << "OK" << endl;
                            server.sendTo(wsEncodeMsg("SOCKETSTAT","CONNECTED",pData.id,WS_OP_TXT,'1'),i);
                        }
                        else {
                            if(isLogEnable("OS")) cout << "ERROR" << endl;
                            server.sendTo(wsEncodeMsg("SOCKETSTAT","ERROR",pData.id,WS_OP_TXT,'1'),i);
                        }
                    }
                    else if(args[0].compare("SEND") == 0) { // SOCKET|SEND|[ID]|[DATA]
                        if(args.size() < 3) continue;
                        int id = (LONG)atoi(args[1].c_str());
                        clientList[i].socketList[id].send(args[2]);
                        if(isLogEnable("OS")) cout << logHeader("OS",4) << "@"<<id<<" SEND" << endl;
                        server.sendTo(wsEncodeMsg("SOCKETSTAT","SENT",pData.id,WS_OP_TXT,'1'),i);
                    }
                    else if(args[0].compare("CREATECONNECTSEND") == 0) { // SOCKET|CREATECONNECTSEND|[HOST]|[PORT]|[DATA]
                        if(args.size() < 4) continue;
                        net_client_clientClass cl;
                        cl.setup(NULL,NULL,error);
                        cl.setDebugFunc(debug);
                        clientList[i].socketList.insert(pair<int,net_client_clientClass>(clientList[i].socket_idRunner,cl));
                        net_client_clientClass* sock = &(clientList[i].socketList[clientList[i].socket_idRunner]);
                        int port = (LONG)atoi(args[2].c_str());
                        if(!sock->connect(args[1],port,1)) {
                            server.sendTo(wsEncodeMsg("SOCKETSTAT","NOCONNECT",pData.id,WS_OP_TXT,'1'),i);
                            continue;
                        }
                        sock->send(args[3]);
                        stringstream ss;
                        ss << "CCS" << clientList[i].socket_idRunner++;
                        if(isLogEnable("OS")) cout << logHeader("OS",4) <<"CREATECONNECTSEND" << endl;
                        server.sendTo(wsEncodeMsg("SOCKETSTAT",ss.str(),pData.id,WS_OP_TXT,'1'),i);
                    }
                    else if(args[0].compare("CLOSE") == 0) { // SOCKET|CLOSE|[ID]
                        int id = (LONG)atoi(args[1].c_str());
                        if(isLogEnable("OS")) cout << logHeader("OS",4) << "@" << id << " CLOSE" << endl;
                        clientList[i].socketList[id].disconnect();
                    }
                }
                else {
                    if(isLogEnable("PACKET")) cout << logHeader("PACKET") << "Invalid OS packet opcode : " << opcode << endl;
                }
            }
        }
    }
}
bool acc(net_ext_sock connector)
{
    if(isLogEnable("CONN")) cout << logHeader("CONN") << "Add " << net_getIpFromHandle(connector.sockHandle) << " as id " << clientList.size() << endl;
    clientS clientData;
    clientData.ip = net_getIpFromHandle(connector.sockHandle);
    clientData.socket_idRunner = 1;
    clientList.push_back(clientData);
    return true; // accept the connection
}
void dis(int id)
{
    if(isLogEnable("CONN")) cout << logHeader("CONN") << "Disconnect " << reqClientInfo(id) << endl;
    // remove flag associate to this , re-id everything
    map<string,vector<int> >::iterator it = clientFlag.begin();
    while(it != clientFlag.end()) {
        vector<int> *targ = &(it->second);
        for(int i = 0;i < targ->size();i++) {
            if(targ->at(i) > id) targ->at(i)--; // re-id
            else if(targ->at(i) == id) {
                targ->erase(targ->begin()+i); // remove
                i--;
            }
        }
        if(targ->size() == 0) {
            clientFlag.erase(it++);
        }
        else {
            it++;
        }
    }
    // close socket
    map<int,net_client_clientClass>::iterator itt = clientList[id].socketList.begin();
    while(itt != clientList[id].socketList.end()) {
        cout << "Force shutdown " << endl;
        (itt->second).forceShutdown();
        itt++;
    }
    clientList.erase(clientList.begin() + id);
}
void startServer()
{
    server.init();
    server.setup(80,run,recv,error,acc,dis);
    server.setDebugFunc(debug);
    server.start();
    server.runLoop();
}
int main(int argc,char** argv)
{
    net_init();
    SetErrorMode(SEM_NOOPENFILEERRORBOX); // for drives detection
    timestamp = 0;
    if(argc >= 1) {
        workingDir = argv[0];
        workingDir = workingDir.substr(0,workingDir.find_last_of('\\'));
        cout << "Working dir : " << workingDir << endl;
    }
    initContentTypeMap();
    startServer();
    net_close();
    return 0;
}
