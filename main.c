#include <stdio.h>

#ifdef _WIN32
#include <process.h>
#else
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#endif

#include "socket.h"

struct user {
    SOCK c_sock;
    char* message;
};

void handle_connection(struct user* hc) { //(__cdecl)
    while(1) {
        int recvstat = s_recv(hc->c_sock, hc->message);
        if(recvstat == 1) {
            break;
        }
        printf("%i: %s", hc->c_sock, hc->message);
        memset(hc->message, 0, 513);
    }
    printf("Connection closed\n");
    #ifdef _WIN32
    closesocket(hc->c_sock);
	#else
    close(hc->c_sock);
    #endif
    return;
}

int main()
{
    SOCK sock = server_setup();
    char message[513] = {0};
    struct user users[512];

    for(int i = 0; 1; i++) {

        users[i].c_sock = s_accept(sock);
        users[i].message = message;

        s_send(users[i].c_sock, "Test\n");

        #ifdef _WIN32
        _beginthread((void *)handle_connection, 0, &users[i]);
        #else
        pthread_t conthread;
        pthread_create(&conthread, NULL, (void *)handle_connection, &users[i]);
        #endif
    }

    #ifdef _WIN32
    WSACleanup();
	#endif

	return 0;
}
