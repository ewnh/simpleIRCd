/** @file
 *  @brief Contains the main server loop and multi-threading code.
 */

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
/** @brief Channel hashtable, using uthash */
struct channel* channels = NULL;

/** @brief Main server loop.
 *
 *  Handles all parts of a user connection - receives messages
 *  and executes the appropriate commands.
 *  @param hc User struct
 */
void handle_connection(struct user* hc) {
    //Initialise variables required to receive messages
    char message[513];
    char recvbuffer[513];
    memset(recvbuffer, '\0', 513);
    char* buffptr = &recvbuffer[0];

    while(1) {
        //Clear message array
        memset(message, 0, 513);
        //Receive data from the user
        int recvstat = sock_recv(hc->c_sock, message, recvbuffer, &buffptr);
        //Close connection if necessary
        if(recvstat == 1) {
            break;
        }
        printf("%i: %s\n", hc->c_sock, message);

        char* strptr;
        //Get the command from the received message
        char* command = strtok_r(message, " ", &strptr);

        //Execute the appropriate code if the command is valid
        if(strcmp(command, "PRIVMSG") == 0) {
            char* target = strtok_r(NULL, " ", &strptr);
            char* message = strtok_r(NULL, " ", &strptr);
            send_privmsg(&channels, target, hc->nick, message);
        }
        else if(strcmp(command, "PING") == 0) {
            //PONG response, used by client to verify server is still responding
            sock_send(hc->c_sock, "PONG", &server_name, strtok_r(NULL, " ", &strptr));
        }
        else if(strcmp(command, "CAP") == 0) {
            //Split again to get next part of command
            command = strtok_r(NULL, " ", &strptr);

            //Display IRCv3 Capacities
            if(strcmp(command, "LS") == 0) {
                //But we don't support any, so send an empty parameter
                sock_send(hc->c_sock, "CAP", "*", "LS :");
            }
        }
        else if(strcmp(command, "NICK") == 0) {
            //Set user nick as specified
            strcpy(hc->nick, strtok_r(NULL, " ", &strptr));
        }
        else if(strcmp(command, "USER") == 0) {
            //Store next word as the user's username
            strcpy(hc->username, strtok_r(NULL, " ", &strptr));

            //Clear user's mode array
            memset(hc->modes, '\0', 7);
            //If strtok_r doesn't return an int, or returns 0, ignore
            if(atoi(strtok_r(NULL, " ", &strptr)) == 8) {
                //Set user as invisible
                hc->modes[0] = 'i';
            }

            //Ignore next parameter; not used
            strtok_r(NULL, " ", &strptr);

            //Store next word as the user's real name
            char* realnm = strtok_r(NULL, " ", &strptr);
            strcpy(hc->realname, realnm++);

            send_registration_messages(hc->c_sock, hc->nick, hc->username);
        }
        else if(strcmp(command, "JOIN") == 0) {
            join_channel(&channels, hc, strtok_r(NULL, " ", &strptr));
        }
    }

    //Close connection and free memory allocated to user struct
    printf("Connection closed\n");
    sock_close(hc->c_sock);
    free(hc);
    return;
}

/** @brief Start a new thread running handle_connection()
 *
 *  Called by main(); used to remove platform-dependant code from main.c,
 *  making it look cleaner.
 *  @param usr User struct
 */
void start_handle_thread(struct user* usr) {
    #ifdef _WIN32
    _beginthread((void *)handle_connection, 0, usr);
    #else
    pthread_t conthread;
    pthread_create(&conthread, NULL, (void *)handle_connection, usr);
    #endif
}
