#include <stdio.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

int main()
{
	WSADATA wsa;
	SOCKET sock, new_sock;
	struct sockaddr_in server, client;

	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		printf("Failed to initialise WinSock: %d\n", WSAGetLastError());
	}
	printf("Initialised Winsock\n");

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
		printf("Cannot create socket: %d\n", WSAGetLastError());
	}
	printf("Socket Created\n");

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(10000);

	if (bind(sock, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR) {
		printf("Bind failed: %d\n", WSAGetLastError());
	}

	printf("Bind complete\n");

	listen(sock, 1);
	printf("Waiting\n");

	int c = sizeof(struct sockaddr_in);
	if ((new_sock = accept(sock, (struct sockaddr *)&client, &c)) == INVALID_SOCKET) {
		printf("Accept failed: %d\n", WSAGetLastError());
	}
	printf("Connection accepted\n");

	char* message = "Test";
	if (send(new_sock, message, strlen(message), 0) == SOCKET_ERROR) {
		printf("Send failed: %d\n", WSAGetLastError());
	}

	char recvbuf[10];
	int recvbuflen = 512;
	int recvbytes = 1;
	do {
		recvbytes = recv(new_sock, recvbuf, recvbuflen, 0);
		if (recvbytes == 0) { printf("Connection closed"); }
		printf("Received %i bytes:\n", recvbytes);
		recvbuf[9] = 0;
		printf("%s", recvbuf);
	} while (recvbytes > 0);

	getchar();

	closesocket(sock);
	WSACleanup();
	return 0;
}