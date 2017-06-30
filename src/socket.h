/** @file
 *  @brief Header file for socket.c
 */

#ifndef SOCKET_H_INCLUDED
#define SOCKET_H_INCLUDED

#include <stdbool.h>

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

SOCK net_setup();
void net_shutdown();
void net_accept(SOCK, SOCK*, char*);
void net_send_host(SOCK, char*, char*, char*, char*);
void net_send(SOCK, char*, char*, char*);
bool net_recv(SOCK, char*, char*, char**);
void net_close(SOCK);

#endif // SOCKET_H_INCLUDED
