#include <stdio.h>

#ifdef _WIN32
#include <process.h>
#else
#include <pthread.h>
#include <string.h>
#endif

#include "socket.h"
#include "parser.h"
#include "commands.h"

struct channel* channels = NULL;

void handle_connection(struct user* hc) {
    while(1) {
        memset(hc->message, 0, 513);
        int recvstat = s_recv(hc->c_sock, hc->message);
        if(recvstat == 1) {
            break;
        }
        printf("%i: %s\n", hc->c_sock, hc->message);

        if(strlen(hc->message) == 0) {
            continue;
        }

        char* strptr = strtok(hc->message, " ");
        if(strcmp(strptr, "JOIN") == 0) {
            join_channel(&channels, hc, strtok(NULL, " "));
        }
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
