#include <stdio.h>
#include <process.h>
#include "socket.h"

struct user {
    SOCKET c_sock;
    char* message;
};

void* handle_connection(struct user* hc) { //(__cdecl)
    printf("In thread");
    while(1) {
        s_recv(hc->c_sock, hc->message);
        printf("%s", hc->message);
        memset(hc->message, 0, sizeof(hc->message));
    }
    return;
}

int main()
{
    SOCKET sock = server_setup();
    char message[513] = {0};
    struct user[4] users;

    for(int i = 0; i < 10; i++) {

        struct user usr;
        usr.c_sock = s_accept(sock);
        usr.message = &message;

        s_send(socklist[i]);
        _beginthread(handle_connection, 0, &usr);

        //printf("Connection closed\n");

        //closesocket(socklist[i]);
    }
	WSACleanup();
	return 0;
}
