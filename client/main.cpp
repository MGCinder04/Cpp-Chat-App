#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
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
    WSADATA wsa{}; // FIX: zero-init
    int r = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (r != 0)
    {
        cerr << "WSAStartup failed: " << r << endl;
        return false;
    }
    return true;
}

void sendMessage(SOCKET s)
{
    cout << "Enter your chat name: " << endl;
    string name;
    getline(cin, name);

    cout << "Type /quit to exit" << endl;
    string message;
    while (true)
    {
        cout << "> ";
        if (!getline(cin, message))
            break;
        if (message == "/quit")
            break;

        string fullMessage = name + ": " + message;

        if (!send_all(s, fullMessage.c_str(), static_cast<int>(fullMessage.size())))
        {
            cerr << "send failed: " << WSAGetLastError() << endl;
            break;
        }
    }
}

void receiveMessage(SOCKET s)
{
    char buf[4096];
    for (;;)
    {
        int n = recv(s, buf, sizeof(buf), 0);
        if (n > 0)
        {
            cout << "\nserver says: " << string(buf, buf + n) << "\n> " << flush;
        }
        else if (n == 0)
        {
            cout << "\nserver closed connection\n";
            break;
        }
        else
        {
            int e = WSAGetLastError();
            if (e == WSAESHUTDOWN || e == WSAECONNRESET || e == WSAENOTSOCK)
                break;

            cerr << "\nrecv failed: " << e << "\n";
            break;
        }
    }
}

int main()
{
    if (!initialize())
    {
        cerr << "Initialization failed" << endl;
        return 1;
    }

    cout << "-------Client Program-------" << endl;

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET)
    {
        cerr << "socket failed: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

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

    if (connect(s, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
    {
        cerr << "connect failed: " << WSAGetLastError() << endl;
        closesocket(s);
        WSACleanup();
        return 1;
    }

    cout << "connected to server" << endl;

    thread sendThread(sendMessage, s);
    thread recvThread(receiveMessage, s);

    // wait for the sender to finish (/quit)
    sendThread.join();

    // graceful shutdown: we’re done sending; tell server (FIN)
    shutdown(s, SD_SEND);

    // If the server doesn’t send anything and keeps the socket open,
    // recvThread could block. To guarantee exit, you can uncomment next line:
    // shutdown(s, SD_BOTH);  // force recv to unblock

    // wait for receiver to finish
    recvThread.join();

    closesocket(s);
    WSACleanup();
    return 0;
}
