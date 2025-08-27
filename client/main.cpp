#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

using namespace std;

#pragma comment(lib, "ws2_32.lib")

bool initialize()
{
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        cerr << "WSAStartup failed: " << result << endl;
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

    cout << "client program" << endl;

    SOCKET s;
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET)
    {
        cerr << "socket failed: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }
    string serverAdd = "127.0.0.1";
    int port = 41030;
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, serverAdd.c_str(), &serverAddr.sin_addr) != 1)
    {
        cerr << "setting address failed: " << WSAGetLastError() << endl;
        closesocket(s);
        WSACleanup();
        return 1;
    };

    if (connect(s, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
    {
        cerr << "connect failed: " << WSAGetLastError() << endl;
        closesocket(s);
        WSACleanup();
        return 1;
    }

    cout << "connected to server" << endl;

    // send and receive data
    string message = "Hello from client";
    int bytesSent = send(s, message.c_str(), static_cast<int>(message.size()), 0);
    if (bytesSent == SOCKET_ERROR)
    {
        cerr << "send failed: " << WSAGetLastError() << endl;
        closesocket(s);
        WSACleanup();
        return 1;
    }

    closesocket(s);
    WSACleanup();

    return 0;
}