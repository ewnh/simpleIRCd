#include <stdio.h>
#include "socket.h"

int main()
{
    SOCKET sock = server_setup();
    SOCKET c_sock = s_accept(sock);
    s_send(c_sock);
	s_recv(c_sock);
	printf("Connection closed\n");

	closesocket(sock);
	WSACleanup();
	return 0;
}
