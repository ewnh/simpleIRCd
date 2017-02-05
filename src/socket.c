#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#ifdef _WIN32
#include <winsock2.h>
#define SOCK SOCKET //socket type

#else
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#define SOCK int

#endif

#include "socket.h"

char server_name[256];
char startup_time[256];

void set_time() {
    time_t curtime;
    time(&curtime);
    strcpy(startup_time, ctime(&curtime));
}

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

    set_time();

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

    //If no target set (e.g. QUIT), don't attach to response
    if(strcmp(target, "") != 0) {
        strcat(response, " ");
        strcat(response, target);
    }

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

int sock_recv(SOCK c_sock, char* message, char* buffer, char** strptr){
    while(1) {
        if(**strptr == '\0') {
            *strptr = &buffer[0];
            memset(buffer, '\0', 513);

            int recvbytes = 0;
            int totalbytes = 0;

            while(1) {

                #ifdef _WIN32
                recvbytes = recv(c_sock, &buffer[totalbytes], 512, 0);
                #else
                recvbytes = read(c_sock, &buffer[totalbytes], 512);
                #endif

                //Connection closed
                if(recvbytes <= 0) {
                    return 1;
                }

                totalbytes += recvbytes;
                //Check if the last two received characters are \r\n
                if(buffer[totalbytes-2] == '\r' && buffer[totalbytes-1] == '\n') {
                    break;
                }
            }
        }

        strcpy(message, strtok_r(NULL, "\r", strptr));
        return 0;
    }
}

void sock_close(SOCK c_sock) {
    #ifdef _WIN32
    closesocket(c_sock);
    #else
    close(c_sock);
    #endif
}

/*
 * public domain strtok_r() by Charlie Gordon
 *
 *   from comp.lang.c  9/14/2007
 *
 *      http://groups.google.com/group/comp.lang.c/msg/2ab1ecbb86646684
 *
 *     (Declaration that it's public domain):
 *      http://groups.google.com/group/comp.lang.c/msg/7c7b39328fefab9c
 */

//Unfortunately, MinGW does not contain an implementation of strtok_r

char* strtok_r(char *str, const char *delim, char **nextp) {
    char *ret;

    if (str == NULL) {
        str = *nextp;
    }

    str += strspn(str, delim);
    if (*str == '\0') {
        return str;
    }

    ret = str;
    str += strcspn(str, delim);
    if (*str) {
        *str++ = '\0';
        if(*delim == '\r') {
            *str++ = '\0';
        }
    }

    *nextp = str;
    return ret;
}

void to_upper(char* str) {

    //Max length of received command is 512 bytes
    for(int i = 0; i < 512; i++) {
        if(str[i] == '\0') {
            return;
        }

        str[i] = toupper(str[i]);
    }
}
