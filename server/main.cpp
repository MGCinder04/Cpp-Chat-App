#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <tchar.h>

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
		return 1;

	cout << "server program" << endl;

	SOCKET lsitenSocket = socket(AF_INET, SOCK_STREAM, 0);

	if (lsitenSocket == INVALID_SOCKET)
	{
		cerr << "socket failed: " << WSAGetLastError() << endl;
		WSACleanup();
		return 1;
	}

	// create address structure
	int port = 41030;
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);

	// convert ipaddress (0.0.0.0) and put it inside sin_family from text to binary
	if (InetPton(AF_INET, _T("0.0.0.0"), &serverAddr.sin_addr) != 1)
	{
		cerr << "setting address failed: " << WSAGetLastError() << endl;
		closesocket(lsitenSocket);
		WSACleanup();
		return 1;
	};

	// bind the ip address and port to a socket
	if (bind(lsitenSocket, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
	{
		cerr << "bind failed: " << WSAGetLastError() << endl;
		closesocket(lsitenSocket);
		WSACleanup();
		return 1;
	}

	// put the socket in listening mode
	if (listen(lsitenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		cerr << "listen failed: " << WSAGetLastError() << endl;
		closesocket(lsitenSocket);
		WSACleanup();
		return 1;
	}

	cout << "listening on port: " << port << endl;

	// accept a client socket
	SOCKET clientSocket = accept(lsitenSocket, nullptr, nullptr);
	if (clientSocket == INVALID_SOCKET)
	{
		cerr << "invalid client socket: " << WSAGetLastError() << endl;
		closesocket(lsitenSocket);
		WSACleanup();
		return 1;
	}

	char buffer[4096];
	int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

	string message;
	if (bytesReceived > 0)
	{
		message = string(buffer, 0, bytesReceived);
		cout << "message from client: " << message << endl;
	}
	else if (bytesReceived == 0)
	{
		cout << "client disconnected" << endl;
	}
	else
	{
		cerr << "recieve failed: " << WSAGetLastError() << endl;
		return 1;
	}

	closesocket(clientSocket);
	closesocket(lsitenSocket);

	WSACleanup();

	return 0;
}