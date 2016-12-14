#include <stdio.h>
#include <process.h>
#include "socket.h"

struct user {
    SOCKET c_sock;
    char* message;
};

void handle_connection(struct user* hc) { //(__cdecl)
    printf("In thread");
    while(1) {
        s_recv(hc->c_sock, hc->message);
        printf("%i: %s", hc->c_sock, hc->message);
        memset(hc->message, 0, 513);
    }
    return;
}

int main()
{
    SOCKET sock = server_setup();
    char message[513] = {0};
    struct user users[4];

    for(int i = 0; 1; i++) {

        //struct user usr;
        users[i].c_sock = s_accept(sock);
        users[i].message = message;

        s_send(users[i].c_sock);
        _beginthread((void *)handle_connection, 0, &users[i]);

        //printf("Connection closed\n");

        //closesocket(socklist[i]);
    }
	WSACleanup();
	return 0;
}
