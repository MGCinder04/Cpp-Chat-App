#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex> 
#include <algorithm>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

// small helper to send all bytes (handles partial sends)
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

static bool initialize()
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

// Shared state
static vector<SOCKET> g_clients; 
static mutex g_clients_mtx;		 

//remove a client from the shared list
static void remove_client(SOCKET s)
{
	lock_guard<mutex> lock(g_clients_mtx);
	auto it = find(g_clients.begin(), g_clients.end(), s);
	if (it != g_clients.end())
		g_clients.erase(it);
}

// broadcast using a snapshot to avoid holding the lock during send()
static void broadcast_to_others(SOCKET from, const string &msg)
{
	vector<SOCKET> targets;
	{
		lock_guard<mutex> lock(g_clients_mtx);
		targets.reserve(g_clients.size());
		for (auto s : g_clients)
			if (s != from)
				targets.push_back(s);
	}
	for (auto s : targets)
	{
		if (!send_all(s, msg.c_str(), static_cast<int>(msg.size())))
		{
			// send error → drop this client
			cerr << "send failed to a client: " << WSAGetLastError() << endl;
			shutdown(s, SD_BOTH);
			closesocket(s);
			remove_client(s);
		}
	}
}

// per-client thread: loop recv, broadcast, clean removal on exit
static void interactWithClient(SOCKET clientSocket)
{
	char buffer[4096];

	for (;;)
	{
		int n = recv(clientSocket, buffer, sizeof(buffer), 0);
		if (n > 0)
		{
			string message(buffer, buffer + n);
			cout << "message from client: " << message << endl;
			broadcast_to_others(clientSocket, message);
		}
		else if (n == 0)
		{
			cout << "client disconnected\n";
			break;
		}
		else
		{
			int e = WSAGetLastError();
			// treat shutdown/reset as normal termination
			if (e != WSAESHUTDOWN && e != WSAECONNRESET && e != WSAENOTSOCK)
				cerr << "recv failed: " << e << endl;
			break;
		}
	}

	// graceful close for this client
	shutdown(clientSocket, SD_BOTH);
	closesocket(clientSocket);
	remove_client(clientSocket);
}

int main()
{
	if (!initialize())
		return 1;
	cout << "--------Server Program--------\n";

	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET)
	{
		cerr << "socket failed: " << WSAGetLastError() << endl;
		WSACleanup();
		return 1;
	}

	// exclusive address use to avoid TIME_WAIT issues
	BOOL exclusive = TRUE;
	setsockopt(listenSocket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
			   reinterpret_cast<const char *>(&exclusive), sizeof(exclusive));

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

	if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		cerr << "listen failed: " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	cout << "listening on port: " << port << endl;

	// Accept loop
	for (;;)
	{
		SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
		if (clientSocket == INVALID_SOCKET)
		{
			int e = WSAGetLastError();
			if (e == WSAEINTR)
				break;
			cerr << "accept failed: " << e << endl;
			continue;
		}

		{
			lock_guard<mutex> lock(g_clients_mtx);
			g_clients.push_back(clientSocket);
		}

		
		thread(interactWithClient, clientSocket).detach();
	}

	// NOTE: In this simple server we never break out of accept loop normally.
	// If you add a shutdown condition, close all clients here.

	shutdown(listenSocket, SD_BOTH);
	closesocket(listenSocket);

	// Close any remaining client sockets (defensive)
	{
		lock_guard<mutex> lock(g_clients_mtx);
		for (auto s : g_clients)
		{
			shutdown(s, SD_BOTH);
			closesocket(s);
		}
		g_clients.clear();
	}

	WSACleanup();
	return 0;
}
