#include <stdio.h>
#include "socket.h"

int main()
{
    SOCKET sock = server_setup();
    char message[513] = {0};

    while(1) {

        SOCKET c_sock = s_accept(sock);
        s_send(c_sock);

        while(1) {
            s_recv(c_sock, message);
            printf("%s", message);
            memset(message, 0, sizeof(message));
        }
        printf("Connection closed\n");

        closesocket(c_sock);
    }
	WSACleanup();
	return 0;
}
