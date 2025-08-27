#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

bool initialize()
{
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

	cout << "server program" << endl;

	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET)
	{
		cerr << "socket failed: " << WSAGetLastError() << endl;
		WSACleanup();
		return 1;
	}

	BOOL exclusive = TRUE;
	setsockopt(listenSocket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
			   reinterpret_cast<const char *>(&exclusive), sizeof(exclusive));

	int port = 41030;
	sockaddr_in serverAddr{}; // zero-initialize
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0

	if (bind(listenSocket, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
	{
		cerr << "bind failed: " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		cerr << "listen failed: " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	cout << "listening on port: " << port << endl;

	SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
	if (clientSocket == INVALID_SOCKET)
	{
		cerr << "invalid client socket: " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	char buffer[4096];
	int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
	if (bytesReceived > 0)
	{
		string message(buffer, buffer + bytesReceived);
		cout << "message from client: " << message << endl;
	}
	else if (bytesReceived == 0)
	{
		cout << "client disconnected" << endl;
	}
	else
	{
		cerr << "receive failed: " << WSAGetLastError() << endl;
		closesocket(clientSocket);
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	shutdown(clientSocket, SD_SEND);
	closesocket(clientSocket);
	closesocket(listenSocket);
	WSACleanup();
	return 0;
}
