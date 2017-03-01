/** @file
 *  @brief Contains code implementing some IRC commands.
 */

#include <stdio.h>

#include "socket.h"
#include "structs.h"

//Defined in socket.c
extern char server_name;
extern char startup_time;

/** @brief Sends a message to every user in a channel.
 *
 *  Used  by commands that need to send a custom hostname (e.g. send_privmsg()).
 *  @note Accepts arguments similar to sock_send_host()
 *  @param chn Channel to send message to
 *  @param hostname Server hostname
 *  @param command Command (or numeric) sent in response
 *  @param target Target of the message
 *  @param message Message to send
 */
void send_to_channel(struct channel* chn, char* hostname, char* command, char* target, char* message) {
    for(int i = 0; i < 10; i++) {
        //If user pointer is invalid, stop
        if(chn->users[i] == NULL) {
            return;
        }

        //Don't send PRIVMSG commands back to the sender
        if(strcmp(hostname, chn->users[i]->nick) == 0 && strcmp(command, "PRIVMSG") == 0) {
            continue;
        }

        //Send message to user
        sock_send_host(chn->users[i]->c_sock, hostname, command, target, message);
    }
}

/** @brief JOIN command - Allows a user to join a channel.
 *
 *  Finds specified channel and associates the user's user struct with that channel's user list.
 *  @param channels Channel hashtable
 *  @param hc User struct that is joining a channel
 *  @param name Name of channel to join
 */
void join_channel(struct channel** channels, struct user* hc, char* name) {

    //Return if specified name is too long
    if(strlen(name) > 50) {
        printf("Channel name too long\n");
        return;
    }

    //Return if the user has not fully registered with the server
    if(hc->nick[0] == '\0' || hc->username[0] == '\0' || hc->realname[0] == '\0') {
        return;
    }

    //Find specified channel in hashtable
    struct channel* chn;
    HASH_FIND_STR(*channels, name, chn);

    //If channel doesn't exist, create it
    if(chn == NULL) {
        //Allocate memory for a channel struct
        chn = malloc(sizeof(struct channel));

        //Copy specified name into channel name variable
        strcpy(chn->name, name);

        //Clear channel's user list
        memset(chn->users, 0, sizeof(chn->users));
        //Add user as the first entry
        chn->users[0] = hc;

        //Add new channel to the channel hashtable
        HASH_ADD_STR(*channels, name, chn);
    }

    //If channel already exists, add user struct to its user list
    else {
        for(int i = 0; i < 10; i++) {
            if(chn->users[i] == NULL) {
                chn->users[i] = hc;
                break;
            }
        }
    }
    printf("Joined channel %s\n", name);

    //Send a JOIN message to the channel, notifying all members
    send_to_channel(chn, hc->nick, "JOIN", name, "");
}

/** @brief PRIVMSG command - Allows a user to send a message to a channel.
 *  @param channels Channel hashtable
 *  @param target Channel to send message to
 *  @param sender Nickname of the user that sent the message
 *  @param raw_message Message to send
 */
void send_privmsg(struct channel** channels, char* target, char* sender, char* raw_message) {

    //Find channel in hashtable
    struct channel* chn;
    HASH_FIND_STR(*channels, target, chn);

    //Return if no channel found
    if(chn == NULL) {
        return;
    }

    //Send message to channel
    send_to_channel(chn, sender, "PRIVMSG", target, raw_message);
}

/** @brief Send registrations messages to a newly joined user.
 *  @param c_sock User socket to send messages to
 *  @param nick User's nickname
 *  @param username User's username
 */
void send_registration_messages(SOCK c_sock, char* nick, char* username) {
    char tempbuffer[128];
    //Send initial registration messages
    sprintf(tempbuffer, "Welcome to the Internet Relay Network %s!%s", nick, username);
    sock_send(c_sock, "001", nick, tempbuffer);
    sprintf(tempbuffer, "Your host is %s, running simpleIRCd", &server_name);
    sock_send(c_sock, "002", nick, tempbuffer);
    sprintf(tempbuffer, "This server was started %s", &startup_time);
    sock_send(c_sock, "003", nick, tempbuffer);
    sprintf(tempbuffer, "%s simpleIRCd TODO", &server_name);
    sock_send(c_sock, "004", nick, tempbuffer);

    //Send message of the day (MOTD)
    sprintf(tempbuffer, ":- %s Message of the day - ", &server_name);
    sock_send(c_sock, "375", nick, tempbuffer);
    sock_send(c_sock, "372", nick, ":- MOTD goes here");
    sock_send(c_sock, "376", nick, ":End of MOTD command");
}
