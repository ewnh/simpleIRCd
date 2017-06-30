/** @file
 *  @brief Contains networking code.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

/** @see socket.h */
#ifdef _WIN32
#include <winsock2.h>
#define SOCK SOCKET //socket type

#else
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define SOCK int

#endif

#include "socket.h"
#include "helpers.h"

/** @brief Stores the server hostname; set in server_setup() */
char server_name[256];
/** @brief Stores the server startup time; set in server_setup() */
char startup_time[256];

/** @brief Stores the current time.
 *
 *  Called by server_setup(), used to store the server startup time.
 */
void set_time() {
    time_t curtime;
    time(&curtime);
    //Store current time in startup_time char array
    strcpy(startup_time, ctime(&curtime));
}

/** @brief Initialises the server.
 *
 *  Creates a server socket, sets the server hostname and startup time,
 *  and binds it to port 10000, allowing users to connect.
 *  @return Server socket
 */
SOCK net_setup() {

    //Initialise Winsock
    #ifdef _WIN32
    WSADATA wsa;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed to initialise WinSock: %d\n", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    #endif

    //Store the server hostname
    gethostname(server_name, sizeof(server_name));

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

    //Set socket details
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(6667);

    //Bind to port 6667
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

    printf("Server setup complete\n");

    //Set server startup time
    set_time();

    return sock;
}

/** @brief Shuts down the server.
 *
 *  Contains Windows-specific cleanup for the server.
 */
void net_shutdown() {
    #ifdef _WIN32
    WSACleanup();
    #endif
}

/** @brief Accepts a user connection.
 *
 *  Listens to server socket and accepts a new user connection.
 *  @param sock Server socket
 *  @param c_sock Pointer to the location where the new user socket should be stored
 *  @param address Pointer to char array where the user's address should be stored
 */
void net_accept(SOCK sock, SOCK* c_sock, char* address) {
    struct sockaddr_in client;

    //Wait for a user to connect
    listen(sock, 10);
    printf("Waiting for connections\n");

    int c = sizeof(struct sockaddr_in);
    //Accept connection
    *c_sock = accept(sock, (struct sockaddr *)&client, &c);

    #ifdef _WIN32
    if (*c_sock == INVALID_SOCKET) {
        printf("Accept failed: %d\n", ERROR);
        exit(EXIT_FAILURE);
    }
    #else
    if(*c_sock < 0) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }
    #endif

    //Save user's IP address in the address char array
    strcpy(address, inet_ntoa(client.sin_addr));
    printf("Connection accepted from %s\n", address);
}

/** @brief Sends an IRC response to a user.
 *
 *  Compiles the contents of the arrays given as arguments into a single message,
 *  which is then sent to the specified user.
 *
 *  IRC messages are generally of the form :hostname command target parameters
 *
 *  @see sock_send()
 *  @param c_sock User socket to send data to
 *  @param hostname Server hostname
 *  @param command Command (or numeric) sent in response
 *  @param target Target of the message (generally either * or the user's nick)
 *  @param message Message to send
 */
void net_send_host(SOCK c_sock, char* hostname, char* command, char* target, char* message) {

    //Check if the full message is too long
    if((strlen(hostname) + strlen(command) + strlen(target) + strlen(message) + 8) > 512) {
        return;
    }

    //Assemble the full message by concatenating each char array together
    char response[512] = {":"};
    strcat(response, hostname);
    strcat(response, " ");
    strcat(response, command);

    //If no target set (e.g. QUIT), don't attach target parameter to response
    if(strcmp(target, "") != 0) {
        strcat(response, " ");
        strcat(response, target);
    }

    strcat(response, " ");
    strcat(response, message);
    strcat(response, "\r\n");

    //Send the response
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

/** @brief Wrapper over sock_send_host()
 *
 *  The majority of responses have the server hostname as the prefix.
 *  sock_send() is almost exactly the same as sock_send_host(), but
 *  passes the server hostname instead of requiring a custom
 *  hostname to be set, preventing redundant code.
 *
 *  @see sock_send_host()
 *  @param c_sock User socket to send data to
 *  @param command Command (or numeric) sent in response
 *  @param target Target of the message (generally either * or the user's nick)
 *  @param message Message to send
 */
void net_send(SOCK c_sock, char* command, char* target, char* message) {
    net_send_host(c_sock, server_name, command, target, message);
}

/** @brief Receives data from a user.
 *
 *  Initially, reads data from the user socket until a full message has been received,
 *  and stores the data in the buffer char array.
 *  The first IRC message is then copied into the message char array.
 *  Any further calls to sock_recv() will check if there is any unread data left
 *  (strptr will not point at a NULL character), and, if there is, the next message
 *  will be copied. Once all data has been read, more data will be retrieved from the socket.
 *
 *  @note Calls read(), which blocks
 *  @param c_sock User socket
 *  @param message Char array that stores retrieved message
 *  @param buffer Char array in which to store received data
 *  @param strptr Pointer to the start of the next message to read; used by strtok_r()
 *  @return True if error encountered or connection closed, false otherwise
 */
bool net_recv(SOCK c_sock, char* message, char* buffer, char** strptr){
    while(1) {
        //Check if all stored data has been read
        if(**strptr == '\0') {
            //Reset buffer and strptr
            *strptr = &buffer[0];
            memset(buffer, '\0', 513);

            int recvbytes = 0;
            int totalbytes = 0;

            //Loop until a full message has been assembled
            while(1) {
                //Read from user socket
                #ifdef _WIN32
                recvbytes = recv(c_sock, &buffer[totalbytes], 512-totalbytes, 0);
                #else
                recvbytes = read(c_sock, &buffer[totalbytes], 512-totalbytes);
                #endif

                //Check if connection closed
                if(recvbytes <= 0) {
                    return true;
                }

                totalbytes += recvbytes;

                //Check if 512 bytes have been received, or if the message is empty
                if(totalbytes == 512 || buffer[0] == '\r' || buffer[0] == '\n') {
                    //Ignore invalid received data
                    totalbytes = 0;
                    continue;
                }

                //Check if the last two received characters are \r\n
                if(buffer[totalbytes-2] == '\r' && buffer[totalbytes-1] == '\n') {
                    break;
                }
            }
        }

        //Copy next command in message array
        strcpy(message, strtok_r(NULL, "\r", strptr));
        return false;
    }
}

/** @brief Close a socket
 *  @param c_sock Socket to close
 */
void net_close(SOCK c_sock) {
    #ifdef _WIN32
    closesocket(c_sock);
    #else
    close(c_sock);
    #endif
}
