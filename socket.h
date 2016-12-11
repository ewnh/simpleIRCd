#ifndef SOCKET_H_INCLUDED
#define SOCKET_H_INCLUDED

#include <winsock2.h>

SOCKET server_setup();
SOCKET s_accept(SOCKET sock);
void s_send(SOCKET c_sock);
void s_recv(SOCKET c_sock, char* mp);

#endif // SOCKET_H_INCLUDED
