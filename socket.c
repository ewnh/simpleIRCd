/** @file
 *  @brief Contains network code
 */

#include <stdio.h>
#include <stdlib.h>

/** @see socket.h */
#ifdef _WIN32
#include <winsock2.h>
#define SOCK SOCKET //socket type

#else
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define SOCK int

#endif

/** @brief Initialises the server.
 *
 *  Creates a server socket and binds it to port 10000, allowing users to connect.
 *  @return Server socket
 */
SOCK server_setup() {

    //Initialise Winsock
	#ifdef _WIN32
    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed to initialise WinSock: %d\n", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    printf("Initialised Winsock\n");
	#endif

    //Create a socket
    SOCK sock;
    struct sockaddr_in server;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    //Check for errors
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

    //Set socket details
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(10000);

    //Bind to port 10000
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

/** @brief Accepts a user connection.
 *
 *  Listens to server socket and accepts a new user connection.
 *  @param sock Server socket
 *  @return Socket containing user connection
 */
SOCK s_accept(SOCK sock) {
    struct sockaddr_in client;

    //Wait for a user to connect
	listen(sock, 1);
	printf("Waiting\n");

	int c = sizeof(struct sockaddr_in);
	//Accept connection
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

/** @brief Sends data to a user.
 *  @param c_sock User socket
 *  @param message Data to send
 */
void s_send(SOCK c_sock, char* message) {
    #ifdef _WIN32
    if (send(c_sock, message, strlen(message), 0) == SOCKET_ERROR) {
        printf("Send failed: %d\n", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
	#else
    if(write(c_sock, message, strlen(message)) < 0) {
        perror("Send failed\n");
        exit(EXIT_FAILURE);
    }
	#endif
}

/** @brief Receives data from a user.
 *  @note Calls read(), which blocks
 *  @param c_sock User socket
 *  @param message Char array in which to store received data
 *  @return Connection status - 1 if connection closed, 0 if not
 */
int s_recv(SOCK c_sock, char* message) {
    int recvbytes;
    int totalrecv = 0;
    do {

        //Read from user socket
    	#ifdef _WIN32
        recvbytes = recv(c_sock, &message[totalrecv], 512-totalrecv, 0);
    	#else
        recvbytes = read(c_sock, &message[totalrecv], 512-totalrecv);
       	#endif

       	//Check if connection closed
       	if(recvbytes == 0) {
            return 1;
       	}

        totalrecv += recvbytes;

        //If we've already received 512 characters, add a line break (to print nicely)
        //and return the filled buffer
        if(totalrecv == 512) {
            message[511] = '\n';
            return 0;
        }

        //If the last two characters are \r\n, we know we have a valid IRC message
        for(int i = 0; i < totalrecv; i++) {
            if(message[i] == '\r' && message[i+1] == '\n') {
                return 0;
            }
        }
    //Keep reading if we have not got a full message
    } while (recvbytes > 0);
}
