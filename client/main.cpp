#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

// CHANGE: small helper to send all bytes (handles partial sends)
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
    // CHANGE: zero-init WSADATA and check WSAStartup result
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

    cout << "client program" << endl;

    // CHANGE: use IPPROTO_TCP (explicit) and check right away
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET)
    {
        cerr << "socket failed: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    // (Optional) CHANGE: disable Nagle if you want low-latency small messages
    // BOOL flag = TRUE;
    // setsockopt(s, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>(&flag), sizeof(flag));

    string serverAdd = "127.0.0.1";
    int port = 41030;

    // CHANGE: zero-initialize sockaddr_in to avoid garbage padding
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    // CHANGE: inet_pton returns 1 on success, 0 invalid, -1 error
    if (inet_pton(AF_INET, serverAdd.c_str(), &serverAddr.sin_addr) != 1)
    {
        cerr << "inet_pton failed (invalid address or error)" << endl;
        closesocket(s); // CHANGE: ensure we close the socket on all error paths
        WSACleanup();
        return 1;
    }

    if (connect(s, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
    {
        cerr << "connect failed: " << WSAGetLastError() << endl;
        closesocket(s);
        WSACleanup();
        return 1;
    }

    cout << "connected to server" << endl;

    // CHANGE: robust send (handle partial sends)
    const string message = "Hello from client";
    if (!send_all(s, message.c_str(), static_cast<int>(message.size())))
    {
        cerr << "send failed: " << WSAGetLastError() << endl;
        closesocket(s);
        WSACleanup();
        return 1;
    }

    // CHANGE: graceful half-close for send; still can recv server reply
    shutdown(s, SD_SEND);

    // CHANGE: optionally read server response (non-blocking if server closes)
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

    // CHANGE: final close/cleanup on success path too
    closesocket(s);
    WSACleanup();
    return 0;
}
