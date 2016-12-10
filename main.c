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

	char* message = "Test\n";
	if (send(new_sock, message, strlen(message), 0) == SOCKET_ERROR) {
		printf("Send failed: %d\n", WSAGetLastError());
	}

	char messagebuf[513];
	int recvbytes = 0;
	do {
		recvbytes = recv(new_sock, messagebuf, 512, 0);
		printf("Received %i bytes: ", recvbytes);
		for(int i = 0; i < 513; i++) {
            if(messagebuf[i] == '\r' && messagebuf[i+1] == '\n') {
                printf("CRLF\n");
                messagebuf[i+2] = '\0';
                break;
            }
		}
        for(int i = 0; i < recvbytes; i++) {
            printf("%c", messagebuf[i]);
        }
        printf("\n");
	} while (recvbytes > 0);

	printf("Connection closed\n");

	closesocket(sock);
	WSACleanup();
	return 0;
}
