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

extern char server_name;
struct channel* channels = NULL;

void handle_connection(struct user* hc) {

    hc->is_cap_negotiating = 0;

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

        if(strcmp(command, "PING") == 0) {
            sock_send(hc->c_sock, "PONG", &server_name, strtok_r(NULL, " ", &strptr));
        }
        else if(strcmp(command, "CAP") == 0) {
            command = strtok_r(NULL, " ", &strptr);

            if(strcmp(command, "LS") == 0) {
                hc->is_cap_negotiating = 1;
                //We don't support any IRCv3 capabilities, so send an empty parameter
                sock_send(hc->c_sock, "CAP", "*", "LS :");
            }
            else if(strcmp(command, "END") == 0 && hc->is_cap_negotiating == 1) {
                send_registration_messages(hc->c_sock, hc->nick, hc->username);
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

            //If client doesn't want to do IRCv3 capacity negotiation
            if(hc->is_cap_negotiating == 0) {
                send_registration_messages(hc->c_sock, hc->nick, hc->username);
            }
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
