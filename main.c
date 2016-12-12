#include <stdio.h>
#include <process.h>
#include "socket.h"

int handle_connection(SOCKET c_sock, char* message) {
    while(1) {
            s_recv(c_sock, message);
            printf("%s", message);
            memset(message, 0, sizeof(message));
    }
    return 0;
}

int main()
{
    SOCKET sock = server_setup();
    SOCKET socklist[2] =
    char message[513] = {0};

    while(1) {

        SOCKET c_sock = s_accept(sock);
        s_send(c_sock);

        _beginthread(handle_connection(c_sock, message), 0, NULL);

        printf("Connection closed\n");

        closesocket(c_sock);
    }
	WSACleanup();
	return 0;
}
