#include <stdio.h>

#ifdef _WIN32
#include <process.h>
#else
#include <pthread.h>
#endif

#include "socket.h"

struct user {
    SOCK c_sock;
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
    SOCK sock = server_setup();
    char message[513] = {0};
    struct user users[4];

    for(int i = 0; 1; i++) {

        //struct user usr;
        users[i].c_sock = s_accept(sock);
        users[i].message = &message;

        s_send(users[i].c_sock);
        #ifdef _WIN32
        _beginthread(handle_connection, 0, &users[i]);

        #else
        pthread_t conthread;
        pthread_create(&conthread, NULL, handle_connection, &users[i]);
        #endif
        //printf("Connection closed\n");

        //closesocket(socklist[i]);
    }

    #ifdef _WIN32
    WSACleanup();
	#endif

	return 0;
}
