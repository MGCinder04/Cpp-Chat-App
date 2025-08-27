#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

// helper to send all bytes (handles partial sends)
static bool send_all(SOCKET s, const char *data, int len)
{
    int sent = 0;
    while (sent < len)
    {
        int n = send(s, data + sent, len - sent, 0);
        if (n == SOCKET_ERROR)
            return false;
        sent += n;
    }
    return true;
}

bool initialize()
{
    // zero-init WSADATA and check WSAStartup result
    WSADATA wsa{};
    int r = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (r != 0)
    {
        cerr << "WSAStartup failed: " << r << endl;
        return false;
    }
    return true;
}

int main()
{
    if (!initialize())
    {
        cerr << "Initialization failed" << endl;
        return 1;
    }

    cout << "-------Client Program-------" << endl;

    // initialize socket
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET)
    {
        cerr << "socket failed: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    // server address and port
    string serverAdd = "127.0.0.1";
    int port = 41030;
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, serverAdd.c_str(), &serverAddr.sin_addr) != 1)
    {
        cerr << "inet_pton failed (invalid address or error)" << endl;
        closesocket(s); 
        WSACleanup();
        return 1;
    }


    // connect to server
    if (connect(s, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
    {
        cerr << "connect failed: " << WSAGetLastError() << endl;
        closesocket(s);
        WSACleanup();
        return 1;
    }

    cout << "connected to server" << endl;

    // send a message
    const string message = "Hello from client";
    if (!send_all(s, message.c_str(), static_cast<int>(message.size())))
    {
        cerr << "send failed: " << WSAGetLastError() << endl;
        closesocket(s);
        WSACleanup();
        return 1;
    }

    // indicate we're done sending
    shutdown(s, SD_SEND);

    // receive a response
    char buf[4096];
    int n = recv(s, buf, sizeof(buf), 0);
    if (n > 0)
    {
        cout << "server says: " << string(buf, buf + n) << endl;
    }
    else if (n == 0)
    {
        cout << "server closed connection" << endl;
    }
    else
    {
        cerr << "recv failed: " << WSAGetLastError() << endl;
        closesocket(s);
        WSACleanup();
        return 1;
    }

    closesocket(s);
    WSACleanup();
    return 0;
}
