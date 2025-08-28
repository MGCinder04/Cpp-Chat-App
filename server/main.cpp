#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>
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

void interactWithClient(SOCKET clientSocket, vector<SOCKET> &clients)
{
	// send and receive data from client
	char buffer[4096];
	while (true)
	{
		int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
		if (bytesReceived > 0)
		{
			string message(buffer, buffer + bytesReceived);
			cout << "message from client: " << message << endl;
			
			// broadcast message to all clients
			for (SOCKET s : clients)
			{
				if (s != clientSocket)
				{
					int sent = send(s, message.c_str(), static_cast<int>(message.size()), 0);
					if (sent == SOCKET_ERROR)
					{
						cerr << "send failed: " << WSAGetLastError() << endl;
					}
				}
			}
		}
		else if (bytesReceived == 0)
		{
			cout << "client disconnected" << endl;
			break;
		}
		else
		{
			cerr << "receive failed: " << WSAGetLastError() << endl;
			break;
		}
		auto it = find(clients.begin(), clients.end(), clientSocket);
		if (it != clients.end())
			clients.erase(it);
	}
	closesocket(clientSocket);
}

int main()
{
	if (!initialize())
	{
		cerr << "Initialization failed" << endl;
		return 1;
	}

	cout << "--------Server Program--------" << endl;

	// listen socket
	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET)
	{
		cerr << "socket failed: " << WSAGetLastError() << endl;
		WSACleanup();
		return 1;
	}

	// avoid "address already in use" error on bind
	BOOL exclusive = TRUE;
	setsockopt(listenSocket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
			   reinterpret_cast<const char *>(&exclusive), sizeof(exclusive));

	// bind and listen
	// listen on all interfaces
	int port = 41030;
	sockaddr_in serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listenSocket, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR)
	{
		cerr << "bind failed: " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	// start listening
	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		cerr << "listen failed: " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	cout << "listening on port: " << port << endl;

	vector<SOCKET> clients;
	SOCKET clientSocket;
	while (true)
	{
		// accept a client connection
		clientSocket = accept(listenSocket, nullptr, nullptr);
		if (clientSocket == INVALID_SOCKET)
		{
			cerr << "invalid client socket: " << WSAGetLastError() << endl;
			closesocket(listenSocket);
			WSACleanup();
			return 1;
		}
		clients.push_back(clientSocket);
		thread t1(interactWithClient, clientSocket, std::ref(clients));
	};

	cout << "client connected" << endl;

	shutdown(clientSocket, SD_SEND);
	closesocket(clientSocket);
	closesocket(listenSocket);
	WSACleanup();
	return 0;
}
