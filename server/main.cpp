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
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(41030);

	// convert ipaddress (0.0.0.0) and put it inside sin_family from text to binary
	if (InetPton(AF_INET, _T("0.0.0.0"), &serverAddr.sin_addr) != 1)
	{
		cout << "setting address failed: " << WSAGetLastError() << endl;
		closesocket(lsitenSocket);
		WSACleanup();
		return 1;
	};

	WSACleanup();

	return 0;
}