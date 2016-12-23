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
void server_shutdown();
SOCK s_accept(SOCK);
void s_send_host(SOCK, char*, char*, char*, char*);
void s_send(SOCK, char*, char*, char*);
int s_recv(SOCK, char*);
void s_close(SOCK);

#endif // SOCKET_H_INCLUDED
