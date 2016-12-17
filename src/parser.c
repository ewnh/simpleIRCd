#include <stdio.h>

#ifdef _WIN32
#include <process.h>
#else
#include <pthread.h>
#include <string.h>
#endif

#include "socket.h"
#include "parser.h"

void handle_connection(struct user* hc) {
    s_send(hc->c_sock, "Test\n");
    while(1) {
        memset(hc->message, 0, 513);
        int recvstat = s_recv(hc->c_sock, hc->message);
        if(recvstat == 1) {
            break;
        }
        printf("%i: %s\n", hc->c_sock, hc->message);
    }
    printf("Connection closed\n");
    s_close(hc->c_sock);
    free(hc);
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
