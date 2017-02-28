/** @file
 *  @brief Header file for socket.c
 */

#ifndef SOCKET_H_INCLUDED
#define SOCKET_H_INCLUDED

/** @brief Ensures compatibility over Windows and Linux
 *
 *  Linux sockets have an integer type\n
 *  Windows sockets have a specific SOCKET type
 *
 *  The value of SOCK is set based on the OS compiled on
 */
#ifdef _WIN32
    #include <winsock2.h>
    #define SOCK SOCKET
#else
    #include <sys/socket.h>
    #define SOCK int
#endif

SOCK server_setup();
void server_shutdown();
SOCK sock_accept(SOCK);
void sock_send_host(SOCK, char*, char*, char*, char*);
void sock_send(SOCK, char*, char*, char*);
int sock_recv(SOCK, char*, char*, char**);
void sock_close(SOCK);
char* strtok_r(char *, const char *, char **);

#endif // SOCKET_H_INCLUDED
