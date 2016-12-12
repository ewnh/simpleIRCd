#include <stdio.h>
#include <process.h>
#include "socket.h"

struct h_cThread {
    SOCKET c_sock;
    char* message;
};

void* handle_connection(struct h_cThread* hc) { //(__cdecl)
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
    SOCKET socklist[2] = {};
    char message[513] = {0};
    struct h_cThread hcstruct;

    for(int i = 0; i < 10; i++) {

        socklist[i] = s_accept(sock);

        s_send(socklist[i]);
        hcstruct.c_sock = socklist[i];
        hcstruct.message = &message;
        _beginthread(handle_connection, 0, &hcstruct);

        //printf("Connection closed\n");

        //closesocket(socklist[i]);
    }
	WSACleanup();
	return 0;
}
