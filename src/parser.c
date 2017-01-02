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

extern char* server_name; //defined in socket.c
extern char* startup_time;
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
            else if(strcmp(command, "END") == 0) {
                char tempbuffer[128];
                sprintf(tempbuffer, "Welcome to the Internet Relay Network %s!%s", hc->nick, hc->username);
                sock_send(hc->c_sock, "001", hc->nick, tempbuffer);
                sprintf(tempbuffer, "Your host is %s, running simpleIRCd", &server_name);
                sock_send(hc->c_sock, "002", hc->nick, tempbuffer);
                sprintf(tempbuffer, "This server was started %s", &startup_time);
                sock_send(hc->c_sock, "003", hc->nick, tempbuffer);
                sprintf(tempbuffer, "%s simpleIRCd TODO", &server_name);
                sock_send(hc->c_sock, "004", hc->nick, tempbuffer);
            }
        }
        else if(strcmp(command, "NICK") == 0) {
            strcpy(hc->nick, strtok_r(NULL, " ", &strptr));

        }
        else if(strcmp(command, "USER") == 0) {
            strcpy(hc->username, strtok_r(NULL, " ", &strptr));

            memset(hc->modes, '\0', 7);
            //If strtok_r doesn't return an int, or returns 0, ignore
            if(atoi(strtok_r(NULL, " ", &strptr)) == 8) {
                //Set user as invisible
                hc->modes[0] = 'i';
            }

            //Ignore next parameter; not used
            strtok_r(NULL, " ", &strptr);

            char* realnm = strtok_r(NULL, " ", &strptr);
            strcpy(hc->realname, realnm++);

        }
        else if(strcmp(command, "TEST") == 0) {
            printf("USER: %s\nMODES: %s\nREALNAME: %s\n", hc->username, hc->modes, hc->realname);
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
