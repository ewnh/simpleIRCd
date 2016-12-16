#ifndef SOCKET_H_INCLUDED
#define SOCKET_H_INCLUDED

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
