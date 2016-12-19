#include <stdio.h>

#ifdef _WIN32
#include <process.h>
#else
#include <pthread.h>
#include <string.h>
#endif

#include "socket.h"
#include "parser.h"

struct channel* channels = NULL;

void handle_connection(struct user* hc) {
    s_send(hc->c_sock, "Test\n");
    while(1) {
        memset(hc->message, 0, 513);
        int recvstat = s_recv(hc->c_sock, hc->message);
        if(recvstat == 1) {
            break;
        }
        printf("%i: %s\n", hc->c_sock, hc->message);

        char* strptr = strtok(hc->message, " ");
        if(strcmp(strptr, "JOIN") == 0) {
            struct channel* chn;
            char* chnname = strtok(NULL, " ");
            HASH_FIND_STR(channels, chnname, chn);

            if(chn == NULL) {
                chn = malloc(sizeof(struct channel));
                strcpy(chn->name, chnname);
                memset(chn->users, 0, sizeof(chn->users));
                chn->users[0] = hc;
                HASH_ADD_STR(channels, name, chn);
                printf("Channel %s added\n", chn->name);
                }

            else {
                for(int i = 0; i < 10; i++) {
                    if(chn->users[i] == NULL) {
                        chn->users[i] = hc;
                        break;
                    }
                }
            }
            printf("Joined channel %s\n", chnname);

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
