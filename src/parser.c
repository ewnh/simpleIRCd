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

/*
 * public domain strtok_r() by Charlie Gordon
 *
 *   from comp.lang.c  9/14/2007
 *
 *      http://groups.google.com/group/comp.lang.c/msg/2ab1ecbb86646684
 *
 *     (Declaration that it's public domain):
 *      http://groups.google.com/group/comp.lang.c/msg/7c7b39328fefab9c
 */

//Unfortunately, MinGW does not contain an implementation of strtok_r

char* strtok_r(char *str, const char *delim, char **nextp) {
    char *ret;

    if (str == NULL) {
        str = *nextp;
    }

    str += strspn(str, delim);
    if (*str == '\0') {
        return str;
    }

    ret = str;
    str += strcspn(str, delim);
    if (*str) {
        *str++ = '\0';
    }

    *nextp = str;
    return ret;
}

void handle_connection(struct user* hc) {
    while(1) {
        memset(hc->message, 0, 513);
        int recvstat = sock_recv(hc->c_sock, hc->message);
        if(recvstat == 1) {
            break;
        }
        printf("%i: %s\n", hc->c_sock, hc->message);

        char* strptr;
        char* command = strtok_r(hc->message, " ", &strptr);
        if(strcmp(command, "JOIN") == 0) {
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
