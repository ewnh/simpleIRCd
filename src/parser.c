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
    char message[513];
    char recvbuffer[513];
    memset(recvbuffer, '\0', 513);
    char* buffptr = &recvbuffer[0];

    while(1) {
        memset(message, 0, 513);
        int recvstat = sock_recv(hc->c_sock, message, recvbuffer, &buffptr);
        if(recvstat == 1) {
            break;
        }
        printf("%i: %s\n", hc->c_sock, message);

        char* strptr;
        char* command = strtok_r(message, " ", &strptr);

        if(strcmp(command, "CAP") == 0) {
            command = strtok_r(NULL, " ", &strptr);

            if(strcmp(command, "LS") == 0) {
                //We don't support any IRCv3 capabilities, so send an empty parameter
                sock_send(hc->c_sock, "CAP", "*", "LS :");
            }
        }
        else if(strcmp(command, "NICK") == 0) {
            strcpy(hc->nick, strtok_r(NULL, " ", &strptr));
        }
        else if(strcmp(command, "JOIN") == 0) {
            join_channel(&channels, hc, strtok_r(NULL, " ", &strptr));
        }
    }

    printf("Connection closed\n");
    sock_close(hc->c_sock);
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
