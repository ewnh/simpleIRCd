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
SOCK s_accept(SOCK sock);
void s_send(SOCK c_sock, char* message);
int s_recv(SOCK c_sock, char* mp);

#endif // SOCKET_H_INCLUDED
