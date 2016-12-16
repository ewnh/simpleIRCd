#include <stdio.h>

#ifdef _WIN32
#include <process.h>
#else
#include <pthread.h>
#endif

#include "socket.h"
#include "parser.h"

void handle_connection(struct user* hc) { //(__cdecl)
    s_send(hc->c_sock);
    while(1) {
        int recvstat = s_recv(hc->c_sock, hc->message);
        if(recvstat == 1) {
            break;
        }
        printf("%i: %s", hc->c_sock, hc->message);
        memset(hc->message, 0, 513);
    }
    printf("Connection closed\n");
    s_close(hc->c_sock);
    return;
}

void start_handle_thread(struct user* usr) {
    #ifdef _WIN32
    _beginthread((void *)handle_connection, 0, usr);
    #else
    pthread_t conthread;
    pthread_create(&conthread, NULL, (void *)handle_connection, usr);
    #endif
}
