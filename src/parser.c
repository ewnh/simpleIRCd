#include <stdio.h>

#ifdef _WIN32
#include <process.h>
#else
#include <pthread.h>
#include <string.h>
#endif

#include "socket.h"
#include "commands.h"
#include "helpers.h"
#include "defines.h"

struct channel* channels = NULL;
struct user* users = NULL;

void handle_connection(struct user* usr) {

    memset(usr->channels, 0, sizeof(usr->channels));
    usr->nick[0] = '\0';

    char message[513];
    char recvbuffer[513];
    memset(recvbuffer, '\0', 513);
    char* buffptr = &recvbuffer[0];

    while(1) {
        memset(message, 0, 513);
        int recvstat = sock_recv(usr->c_sock, message, recvbuffer, &buffptr);
        if(recvstat == 1) {
            break;
        }
        printf("%i: %s\n", usr->c_sock, message);

        char* strptr;
        char* command = strtok_r(message, " ", &strptr);
        to_upper(command);

        if(strcmp(command, "PRIVMSG") == 0) {
            send_privmsg(usr, strptr);
        }
        else if(strcmp(command, "PING") == 0) {
            sock_send(usr->c_sock, "PONG", &server_name[0], strtok_r(NULL, " ", &strptr));
        }
        else if(strcmp(command, "CAP") == 0) {
            command = strtok_r(NULL, " ", &strptr);

            //Display IRCv3 Capacities
            if(strcmp(command, "LS") == 0) {
                //But we don't support any, so send an empty parameter
                sock_send(usr->c_sock, "CAP", "*", "LS :");
            }
        }
        else if(strcmp(command, "NICK") == 0) {
            set_nick(usr, strtok_r(NULL, " ", &strptr));
        }
        else if(strcmp(command, "USER") == 0) {
            strcpy(usr->username, strtok_r(NULL, " ", &strptr));

            //Ignore next two parameters; not used
            strtok_r(NULL, " ", &strptr);
            strtok_r(NULL, " ", &strptr);

            char* realnm = strtok_r(NULL, " ", &strptr);
            strcpy(usr->realname, ++realnm);

            send_registration_messages(usr->c_sock, usr->nick, usr->username, usr->address);
        }
        else if(strcmp(command, "JOIN") == 0) {
            join_channel(usr, strptr);
        }
        else if(strcmp(command, "WHOIS") == 0) {
            whois_user(usr->c_sock, usr->nick, strtok_r(NULL, " ", &strptr));
        }
        else if(strcmp(command, "TOPIC") == 0) {
            set_topic(usr, strptr);
        }
        else if(strcmp(command, "WHO") == 0) {
            who_request(usr, strtok_r(NULL, " ", &strptr));
        }
        else if(strcmp(command, "NAMES") == 0) {
            name_reply(usr, strtok_r(NULL, " ", &strptr));
        }
        else if(strcmp(command, "PART") == 0) {
            user_part(usr, strptr);
        }
        else if(strcmp(command, "QUIT") == 0) {
            user_quit(usr, strtok_r(NULL, " ", &strptr));
            break;
        }
        else if(strcmp(command, "LIST") == 0) {
            list_channels(usr);
        }
        else if(strcmp(command, "MODE") == 0) {
            set_mode(usr, strptr);
        }
        else if(strcmp(command, "KICK") == 0) {
            kick_user(usr, strptr);
        }
        else {
            send_error(NULL, usr, 421, command);
        }
    }

    printf("Connection closed\n");
    sock_close(usr->c_sock);
    free(usr);
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
