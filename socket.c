#include <stdio.h>

#ifdef _WIN32
#include <winsock2.h>
#define SOCK SOCKET //socket type

#else
#include <sys/socket.h>
#include <netinet/in.h>
#define SOCK int

#endif

#pragma comment(lib, "ws2_32.lib")

SOCK server_setup() {

	#ifdef _WIN32
    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed to initialise WinSock: %d\n", WSAGetLastError());
    }
    printf("Initialised Winsock\n");
	#endif

    SOCK sock;
    struct sockaddr_in server;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    #ifdef _WIN32
    if (sock == INVALID_SOCKET) {
        printf("Cannot create socket: %d\n", WSAGetLastError());
    }
	#else
    if(sock < 0) {
        printf("Cannot create socket");
    }
	#endif

	printf("Socket Created\n");

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(10000);

	int err = bind(sock, (struct sockaddr *)&server, sizeof(server));
	#ifdef _WIN32
    if (err == SOCKET_ERROR) {
        printf("Bind failed: %d\n", WSAGetLastError());
    }
	#else
    if(err < 0) {
        printf("Bind failed");
    }
	#endif

	printf("Bind complete\n");
	return sock;
}

SOCK s_accept(SOCK sock) {
    struct sockaddr_in client;


	listen(sock, 1);
	printf("Waiting\n");

	int c = sizeof(struct sockaddr_in);
    SOCK c_sock = accept(sock, (struct sockaddr *)&client, &c);

    #ifdef _WIN32
    if (c_sock == INVALID_SOCKET) {
        printf("Accept failed: %d\n", ERROR);
    }
	#else
    if(c_sock < 0) {
        printf("Accept failed");
    }
	#endif
	printf("Connection accepted\n");

	return c_sock;
}

void s_send(SOCK c_sock) {
    char* message = "Test\n";

    #ifdef _WIN32
    if (send(c_sock, message, strlen(message), 0) == SOCKET_ERROR) {
        printf("Send failed: %d\n", WSAGetLastError());
    }
	#else
    if(write(c_sock, message, strlen(message)) < 0) {
        printf("Send failed\n");
    }
	#endif
}

void s_recv(SOCK c_sock, char* message) {
    int recvbytes;
    int totalrecv = 0;
    do {
    	#ifdef _WIN32
        recvbytes = recv(c_sock, &message[totalrecv], 512, 0);
    	#else
        recvbytes = read(c_sock, &message[totalrecv], 512);
       	#endif

        totalrecv += recvbytes;

        for(int i = 0; i < 513; i++) {
            if(message[i] == '\r' && message[i+1] == '\n') {
                return;
            }
        }
    } while (recvbytes > 0);
}
