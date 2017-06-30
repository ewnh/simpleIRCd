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
#include "commands.h"
#include "helpers.h"
#include "defines.h"

/** @brief Channel hashtable, using uthash */
struct channel* channels = NULL;
/** @brief User hashtable, using uthash */
struct user* users = NULL;

/** @brief Main server loop.
 *
 *  Handles all parts of a user connection - receives messages
 *  and executes the appropriate commands.
 *  @param usr User struct
 */
void handle_connection(struct user* usr) {
    //Set user variables
    memset(usr->channels, 0, sizeof(usr->channels));
    usr->is_registered = false;
    usr->has_sent_quit = false;

    //Initialise variables required to receive messages
    char message[513];
    char recvbuffer[513];
    memset(recvbuffer, '\0', 513);
    char* buffptr = &recvbuffer[0];

    while(1) {
        //Clear message array
        memset(message, 0, 513);
        //Receive data from the user
        int recvstat = net_recv(usr->c_sock, message, recvbuffer, &buffptr);
        //Close connection if necessary
        if(recvstat == 1) {
            break;
        }
        printf("%i: %s\n", usr->c_sock, message);

        char* strptr;
        //Get the command from the received message
        char* command = strtok_r(message, " ", &strptr);
        //Convert command to uppercase
        to_upper(command);

        //Until the user has registered, they only have access to NICK and USER
        if(strcmp(command, "NICK") == 0) {
            set_nick(usr, strtok_r(NULL, " ", &strptr));
            //Skip the rest of the loop so we don't get to the error at the bottom
            continue;
        }
        else if(strcmp(command, "USER") == 0) {
            //Store next word as the user's username
            strcpy(usr->username, strtok_r(NULL, " ", &strptr));

            //Ignore next two parameters; not used
            strtok_r(NULL, " ", &strptr);
            strtok_r(NULL, " ", &strptr);

            //Store next word as the user's real name
            char* realnm = strtok_r(NULL, " ", &strptr);
            strcpy(usr->realname, ++realnm);

            //User has now verified, so set the boolean
            usr->is_registered = true;

            send_registration_messages(usr->c_sock, usr->nick, usr->username, usr->address);
            //Same as above
            continue;
        }

        //If the user hasn't registered (i.e. sent valid NICK and USER commands), don't let
        //them use any other commands - skip the rest of the loop
        if(!usr->is_registered) {
            continue;
        }

        //Otherwise, execute the appropriate code if the command is valid
        //See https://tools.ietf.org/html/rfc2812 for a explanation of each command
        if(strcmp(command, "PRIVMSG") == 0) {
            send_privmsg(usr, strptr);
        }
        else if(strcmp(command, "PING") == 0) {
            //PONG response, used by client to verify server is still responding
            net_send(usr->c_sock, "PONG", &server_name[0], strtok_r(NULL, " ", &strptr));
        }
        else if(strcmp(command, "CAP") == 0) {
            //Split again to get next part of command
            command = strtok_r(NULL, " ", &strptr);

            //Display IRCv3 Capacities
            if(strcmp(command, "LS") == 0) {
                //But we don't support any, so send an empty parameter
                net_send(usr->c_sock, "CAP", "*", "LS :");
            }
        }
        else if(strcmp(command, "JOIN") == 0) {
            join_channel(usr, strptr);
        }
        else if(strcmp(command, "WHOIS") == 0) {
            whois_user(usr, strtok_r(NULL, " ", &strptr));
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
            user_quit(usr, strptr);
            //Stop executing the loop and go to cleanup
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
            //ERR_UNKNOWNCOMMAND
            send_error(NULL, usr, 421, command);
        }
    }

    printf("Connection closed\n");
    //If the user hasn't already sent a QUIT message, send one automatically
    if(!usr->has_sent_quit) {
        user_quit(usr, "Connection closed by user");
    }
    //Remove the user from the users hashtable
    HASH_DEL(users, usr);
    //Close connection and free memory allocated to user struct
    net_close(usr->c_sock);
    free(usr);
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
