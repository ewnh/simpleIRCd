#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <winsock2.h>
#define SOCK SOCKET //socket type

#else
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#define SOCK int

#endif

char server_name[256];

SOCK server_setup() {

	#ifdef _WIN32
    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed to initialise WinSock: %d\n", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    printf("Initialised Winsock\n");
	#endif

    gethostname(server_name, sizeof(server_name));

    SOCK sock;
    struct sockaddr_in server;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    #ifdef _WIN32
    if (sock == INVALID_SOCKET) {
        printf("Cannot create socket: %d\n", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
	#else
    if(sock < 0) {
        perror("Cannot create socket");
        exit(EXIT_FAILURE);
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
        exit(EXIT_FAILURE);
    }
	#else
    if(err < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
	#endif

	printf("Bind complete\n");
	return sock;
}

void server_shutdown() {
    #ifdef _WIN32
    WSACleanup();
	#endif
}

SOCK sock_accept(SOCK sock) {
    struct sockaddr_in client;

	listen(sock, 1);
	printf("Waiting\n");

	int c = sizeof(struct sockaddr_in);
    SOCK c_sock = accept(sock, (struct sockaddr *)&client, &c);

    #ifdef _WIN32
    if (c_sock == INVALID_SOCKET) {
        printf("Accept failed: %d\n", ERROR);
        exit(EXIT_FAILURE);
    }
	#else
    if(c_sock < 0) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }
	#endif
	printf("Connection accepted\n");

	return c_sock;
}

void sock_send_host(SOCK c_sock, char* hostname, char* command, char* target, char* message) {

    if((strlen(hostname) + strlen(command) + strlen(target) + strlen(message) + 8) > 512) {
        printf("Message too long to send\n");
        printf("Command: %s\n", command);
        printf("Target: %s\n", target);
        printf("Message: %s\n", message);
    }

    //Server messages should be of the form :hostname command target parameters
    char response[512] = {":"};
    strcat(response, hostname);
    strcat(response, " ");
    strcat(response, command);
    strcat(response, " ");
    strcat(response, target);
    strcat(response, " ");
    strcat(response, message);
    strcat(response, "\r\n");

    #ifdef _WIN32
    if (send(c_sock, response, strlen(response), 0) == SOCKET_ERROR) {
        printf("Send failed: %d\n", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
	#else
    if(write(c_sock, response, strlen(response)) < 0) {
        perror("Send failed\n");
        exit(EXIT_FAILURE);
    }
	#endif
}

//Wrapper over s_send_host - sends the server hostname instead of a user's hostname
//For use when sending server messages
void sock_send(SOCK c_sock, char* command, char* target, char* message) {
    sock_send_host(c_sock, server_name, command, target, message);
}

int sock_recv(SOCK c_sock, char* message) {
    int recvbytes;
    int totalrecv = 0;
    do {

    	#ifdef _WIN32
        recvbytes = recv(c_sock, &message[totalrecv], 512-totalrecv, 0);
    	#else
        recvbytes = read(c_sock, &message[totalrecv], 512-totalrecv);
       	#endif
       	if(recvbytes == 0) {
            return 1;
       	}

        totalrecv += recvbytes;

        for(int i = 0; i < 512; i++) {
            if(message[i] == '\r' && message[i+1] == '\n') {
                message[i] = '\0';
                return 0;
            }
        }
    } while (recvbytes > 0);
}

void sock_close(SOCK c_sock) {
    #ifdef _WIN32
    closesocket(c_sock);
    #else
    close(c_sock);
    #endif
}
