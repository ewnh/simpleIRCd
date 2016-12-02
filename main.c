#include <stdio.h>
#include <winsock2.h>

int main()
{
	WSADATA wsa;
	SOCKET sock;
	struct sockaddr_in server;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		printf("Failed to initialise WinSock: %d", WSAGetLastError());
	}
	printf("Initialised Winsock");

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		printf("Cannot create socket: %d", WSAGetLastError());
	}
	printf("Socket Created");

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(10000);

	if (bind(sock, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR) {
		printf("Bind failed: %d", WSAGetLastError());
	}
	printf("Bind complete");

	Listen(sock, 1);
	printf("Waiting");



	closesocket(sock);
	return 0;
}