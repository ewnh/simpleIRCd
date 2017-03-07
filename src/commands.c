/** @file
 *  @brief Contains IRC command implementations
 */

#include <stdio.h>
#include <stdbool.h>

#include "defines.h"
#include "helpers.h"
#include "commands.h"
#include "socket.h"

/** @brief JOIN command - allows a user to join a channel.
 *
 *  Finds the specified channel, checks the user can join and then associates
 *  the user's user struct with that channel's user list. Sends information
 *  (i.e. MODE, NAMES messages) to the user on joining.
 *
 *  @param hc User struct that is joining a channel
 *  @param strptr Pointer to the current reading location in the receiving buffer
 */
void join_channel(struct user* hc, char* strptr) {

    //Copy channel name into buffer
    char buffer[64];
    strcpy(buffer, strtok_r(NULL, " ", &strptr));

    //Return and send an error if specified name is too long
    if(strlen(buffer) > 50) {
        send_error(NULL, hc, 479, buffer);
        return;
    }

    //Find specified channel in hashtable
    struct channel* chn;
    HASH_FIND_STR(channels, buffer, chn);

    //If channel doesn't exist, create it
    if(chn == NULL) {
        //Allocate memory for a channel struct
        chn = malloc(sizeof(struct channel));

        //Set initial channel information
        //Copy specified name into channel name variable
        strcpy(chn->name, buffer);

        //Set topic as empty
        chn->topic[0] = '\0';

        //Clear users array and associate joining user with the channel
        memset(chn->users, 0, sizeof(chn->users));
        chn->users[0] = hc;

        //Clear operators array and set joining user as an operator
        memset(chn->operators, 0, sizeof(chn->operators));
        chn->operators[0] = hc;

        //Clear voiced users array
        memset(chn->voiced, 0, sizeof(chn->voiced));

        //Clear and set default modes
        memset(chn->mode, '\0', sizeof(chn->mode));
        chn->mode[0] = '+';
        chn->mode[1] = 'n';

        //Set default channel limit
        chn->limit = CHANNEL_MAX_USERS;

        //Clear password and bans arrays
        memset(chn->password, '\0', sizeof(chn->password));
        memset(chn->bans, '\0', sizeof(chn->bans));

        //Add new channel to the channel hashtable
        HASH_ADD_STR(channels, name, chn);
        }

    //If channel already exists, check if the user can join and,
    //if so, add user struct to its user list
    else {
        //If maximum user limit reached, return
        if(chn->limit == get_users_in_channel(chn)) {
            send_error(chn, hc, 471, NULL);
            return;
        }
        //Check if sent password is correct
        if(get_flag(chn->mode, 'k') && strcmp(chn->password, strtok_r(NULL, " ", &strptr)) != 0) {
            send_error(chn, hc, 475, NULL);
            return;
        }
        //Check if the user is banned
        if(check_if_banned(chn, hc)) {
            send_error(chn, hc, 474, NULL);
            return;
        }
        //Add user pointer to channel's users array
        for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
            if(chn->users[i] == NULL) {
                chn->users[i] = hc;
                break;
            }
        }
    }

    //Add channel pointer to the user's channels array
    for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
        if(hc->channels[i] == NULL) {
            hc->channels[i] = chn;
            break;
        }
    }

    //Send a JOIN message to the channel, notifying all members
    send_to_channel(chn, hc->nick, "JOIN", chn->name, "");

    //Send mode message to the joining user
    sock_send(hc->c_sock, "MODE", chn->name, chn->mode);

    //If channel topic set, send to the user
    //Don't send a message if no topic set
    if(chn->topic[0] != '\0') {
        sprintf(buffer, "%s :%s", chn->name, chn->topic);
        sock_send(hc->c_sock, "332", hc->nick, buffer);
    }

    //Send names message to the user
    name_reply(hc, chn->name);
}

/** @brief PRIVMSG command - allows a user to send a message to a channel.
 *  @param usr Pointer to user struct
 *  @param strptr Pointer to the current reading location in the receiving buffer
 */
void send_privmsg(struct user* usr, char* strptr) {

    //Copy channel name into buffer
    char buffer[64];
    strcpy(buffer, strtok_r(NULL, " ", &strptr));

    //Find channel
    struct channel* chn = get_channel(usr, buffer);

    //Return if not channel found
    if(chn == NULL) {
        return;
    }

    //Check if the user is in a moderated channel and not voiced
    bool is_voiced = get_flag(chn->mode, 'm') && !(is_present(chn->operators, usr) ||
                                                is_present(chn->voiced, usr));
    //Check if the channel doesn't allow users not in the channel to send messages,
    //and check if the user is in the channel
    bool can_send_from_outside = get_flag(chn->mode, 'n') && !is_present(chn->users, usr);

    //If so, return
    if(is_voiced || can_send_from_outside) {
        send_error(chn, usr, 404, "");
        return;
    }

    //Send the message to the channel
    send_to_channel(chn, usr->nick, "PRIVMSG", chn->name, strptr);
}

/** @brief Send registration messages to a newly joined user.
 *  @param c_sock User socket to send messages to
 *  @param nick User's nickname
 *  @param username User's username
 *  @param address User's IP address
 */
void send_registration_messages(SOCK c_sock, char* nick, char* username, char* address) {
    char tempbuffer[128];
    //Send initial registration messages
    sprintf(tempbuffer, "Welcome to the Internet Relay Network %s!%s@%s", nick, username, address);
    sock_send(c_sock, "001", nick, tempbuffer);
    sprintf(tempbuffer, "Your host is %s, running simpleIRCd", &server_name);
    sock_send(c_sock, "002", nick, tempbuffer);
    sprintf(tempbuffer, "This server was started %s", &startup_time);
    sock_send(c_sock, "003", nick, tempbuffer);
    sprintf(tempbuffer, "%s simpleIRCd mnptovklb", &server_name);
    sock_send(c_sock, "004", nick, tempbuffer);

    //Send message of the day (MOTD)
    sprintf(tempbuffer, ":- %s Message of the day - ", &server_name);
    sock_send(c_sock, "375", nick, tempbuffer);
    sock_send(c_sock, "372", nick, ":- Running simpleIRCd");
    sock_send(c_sock, "372", nick, ":- https://github.com/ewnh/simpleIRCd");
    sock_send(c_sock, "376", nick, ":End of MOTD command");
}

/** @brief WHOIS command - retrieves information about a user.
 *
 *  Tells the requesting user the target's identifying information
 *  (e.g. nickname, username, etc.), which server the target is on
 *  and a list of channel's the target is connected to along with their status on each.
 *
 *  @param usr Pointer to user struct
 *  @param nick Nickname of the target user
 */
void whois_user(struct user* usr, char* nick) {

    //Find the target user in the users hashtable
    struct user* target;
    HASH_FIND_STR(users, nick, target);

    //Send an error and return if they don't exist
    if(target == NULL) {
        send_error(NULL, usr, 401, nick);
        return;
    }

    char buffer[128];

    //Send target's identifying information
    sprintf(buffer, "%s %s %s * :%s", target->nick, target->username, target->address, target->realname);
    sock_send(usr->c_sock, "311", usr->nick, buffer);

    //Send information about which server the target is connected to
    sprintf(buffer, "%s %s :info", target->nick, &server_name);
    sock_send(usr->c_sock, "312", usr->nick, buffer);

    //Send list of channels the target is connected to along with their status on each
    sprintf(buffer, "%s :", target->nick);
    for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
        if(target->channels[i] == NULL) {
            break;
        }

        //Don't send hidden channels (with a +p flag)
        if(get_flag(target->channels[i]->mode, 'p')) {
            continue;
        }

        //Prefix channel with @ if the target is an operator
        if(is_present(target->channels[i]->operators, target)) {
            strcat(buffer, "@");
        }
        //Prefix with + if the user is voiced
        else if(get_flag(target->channels[i]->mode, 'm') && is_present(target->channels[i]->voiced, target)) {
            strcat(buffer, "+");
        }

        strcat(buffer, target->channels[i]->name);
        strcat(buffer, " ");
    }
    sock_send(usr->c_sock, "319", usr->nick, buffer);

    //Send end of WHOIS message
    sprintf(buffer, "%s :End of WHOIS list", target->nick);
    sock_send(usr->c_sock, "318", usr->nick, buffer);
}

/** @brief TOPIC command - sets a channel's topic.
 *  @param usr Pointer to user struct
 *  @param strptr Pointer to the current reading location in the receiving buffer
 */
void set_topic(struct user* usr, char* strptr) {

    //Assign parts of string to variables first so we can check if they are valid
    char* chn_name = strtok_r(NULL, " ", &strptr);
    //Topic = next part of string, excluding the first character (which will be a colon)
    char* topic = ++strptr;

    //If the topic is invalid (too long), return
    if(strlen(topic) > 50) {
        return;
    }

    //Find channel in the channels hashtable
    struct channel* chn = get_channel(usr, chn_name);

    //Return if it doesn't exist
    if(chn == NULL) {
        return;
    }

    //Send an error if the topic can only be changed by operators, and the user isn't one
    if(get_flag(chn->mode, 't') && !is_present(chn->operators, usr)) {
        send_error(chn, usr, 482, NULL);
        return;
    }

    //Set the topic, ignoring the colon that topic messages are prefixed with
    strcpy(chn->topic, topic);

    //Send a TOPIC message, notifying users of the topic change
    send_to_channel(chn, usr->nick, "TOPIC", chn->name, chn->topic);
}

/** @brief NICK command - sets a users nickname.
 *  @param usr Pointer to user struct
 *  @param nick User's requested nickname
 */
void set_nick(struct user* usr, char* nick) {

    //If the nickname is empty or too long, send an error and return
    if(nick[0] == '\0' || strlen(nick) > 9) {
        send_error(NULL, usr, 432, nick);
        return;
    }

    //Search the users hashtable for the given nickname
    struct user* tempusr;
    HASH_FIND_STR(users, nick, tempusr);

    //If the request nickname is not currently in use
    if(tempusr == NULL) {
        //If this is a nickname change, temporarily remove from hashtable
        if(usr->is_registered) {
            HASH_DEL(users, usr);
        }

        //If user isn't in any channels and this is a nick change
        if(usr->channels[0] == NULL && usr->is_registered) {
            //Send the NICK message to only the user
            sock_send_host(usr->c_sock, usr->nick, "NICK", "", nick);
        }
        //If they are in any channels, loop over channels and send a channel message
        else {
            for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
                if(usr->channels[i] == NULL) {
                    break;
                }

                send_to_channel(usr->channels[i], usr->nick, "NICK", "", nick);
            }
        }

        //Change nickname and add back to users hashtable
        strcpy(usr->nick, nick);
        HASH_ADD_STR(users, nick, usr);
    }

    //If nickname is currently in use, send an error
    else {
        //If nickname has been previously set, send error message to that
        if(usr->is_registered) {
            send_error(NULL, usr, 433, usr->nick);
        }
        //Otherwise, send the error to * (the default target for unset nicknames)
        else {
            //Set nick to * to fit send_error() arguments
            usr->nick[0] = '*';
            usr->nick[1] = '\0';
            send_error(NULL, usr, 433, nick);
        }
    }
}

/** @brief WHO command - get information about users in a channel.
 *
    @verbatim WHO messages are of the form "<channel> <user> <host> <server> <nick>
    ( "H" / "G" > ["*"] [ ( "@" / "+" ) ] :<hopcount> <real name>"
    @endverbatim
 *
 *  @param usr Pointer to user struct
 *  @param chn_name Name of channel to get information about
 */
void who_request(struct user* usr, char* chn_name) {

    //Find channel
    struct channel* chn = get_channel(usr, chn_name);

    //Return if it doesn't exist
    if(chn == NULL) {
        return;
    }

    char tempbuffer[128];
    //Loop over every user, and send a WHO message for each
    for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
        if(chn->users[i] == NULL) {
            break;
        }

        //WHO messages send the status of the user in the form H, H@ or H+
        char op_status[3] = {'H', '\0', '\0'};
        //If the user is an operator, add an @
        if(is_present(chn->operators, chn->users[i])) {
            op_status[1] = '@';
        }
        //If the user is voiced, add a +
        else if(get_flag(chn->mode, 'm') && is_present(chn->voiced, chn->users[i])) {
            op_status[1] = '+';
        }

        //Send a WHO message
        sprintf(tempbuffer, "%s %s %s %s %s %s :%s %s", chn->name, chn->users[i]->username, chn->users[i]->address, &server_name,
                op_status, chn->users[i]->nick, "0", chn->users[i]->realname);
        sock_send(usr->c_sock, "352", usr->nick, tempbuffer);
    }

    //Send the end of WHO message
    sprintf(tempbuffer, "%s :End of WHO list", chn->name);
    sock_send(usr->c_sock, "315", usr->nick, tempbuffer);
}

/** @brief NAMES command - get a list of all users in a channel.
 *
    @verbatim
    NAMES messages are of the form "( "=" / "*" / "@" ) <channel>
    :[ "@" / "+" ] <nick> *( " " [ "@" / "+" ] <nick> )"
    @endverbatim
 *
 *  @param usr Pointer to user struct
 *  @param chn_name Name of channel to get information about
 */
void name_reply(struct user* usr, char* chn_name) {

    //Get the specified channel
    struct channel* chn = get_channel(usr, chn_name);

    //Return if it doesn't exist
    if(chn == NULL) {
        return;
    }

    //Add channel status and name to the buffer
    char buffer[128];
    sprintf(buffer, "= ");
    strcat(buffer, chn_name);
    strcat(buffer, " :");

    //Loop over every user
    for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
        if(chn->users[i] == NULL) {
            break;
        }

        //If they are an operator, prefix their name with an @
        if(is_present(chn->operators, chn->users[i])) {
            strcat(buffer, "@");
        }
        //If they are voiced, prefix with +
        else if(get_flag(chn->mode, 'm') && is_present(chn->voiced, chn->users[i])) {
            strcat(buffer, "+");
        }

        //Append the status and name to the buffer
        strcat(buffer, chn->users[i]->nick);
        strcat(buffer, " ");
    }

    //Send the NAMES message
    sock_send(usr->c_sock, "353", usr->nick, buffer);

    //Send an end of NAMES message
    sprintf(buffer, "%s :End of NAMES list", chn->name);
    sock_send(usr->c_sock, "366", usr->nick, buffer);
}

/** @brief PART command - allows a user to leave a channel.
 *  @param usr Pointer to user struct
 *  @param strptr Pointer to the current reading location in the receiving buffer
 */
void user_part(struct user* usr, char* strptr) {

    //Get the appropriate channel
    struct channel* chn = get_channel(usr, strtok_r(NULL, " ", &strptr));

    //Return if it doesn't exist
    if(chn == NULL) {
        return;
    }

    //If no reason is provided, use the user's nickname as the reason
    if(strptr[0] == '\0') {
        //Send a PART message to every user in the channel
        send_to_channel(chn, usr->nick, "PART", chn->name, usr->nick);
    }
    //Otherwise, send the reason
    else {
        send_to_channel(chn, usr->nick, "PART", chn->name, strptr);
    }

    //Remove the user struct from the channel's users list
    remove_from_channel(chn, usr);

    //Delete channel if there are no remaining connected users
    if(get_users_in_channel(chn) == 0) {
        delete_channel(chn);
    }
}

/** @brief QUIT command - allows a user to leave the server.
 *  @param usr Pointer to user struct
 *  @param message User's leaving message
 */
void user_quit(struct user* usr, char* message) {
    struct channel* chn;

    //Loop over every channel the user belongs to
    for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
        //Store a reference to each channel for use later
        chn = usr->channels[i];

        if(chn == NULL) {
            break;
        }

        //Remove the user struct from each channel
        remove_from_channel(chn, usr);

        //If there are any more users connected to the channel, send a QUIT message
        if(get_users_in_channel(chn) != 0) {
            send_to_channel(chn, usr->nick, "QUIT", "", message);
        }
        //Otherwise, delete the channel
        else {
            delete_channel(chn);
        }
    }

    //Send an ERROR message to the user acknowledging the QUIT message
    sock_send(usr->c_sock, "ERROR", ":Closing Link:", message);

    //The user has used QUIT, so set the appropriate variable
    usr->has_sent_quit = true;
}

/** @brief LIST command - gives the user a list of all public channels on the server.
 *  @param usr Pointer to user struct
 */
void list_channels(struct user* usr) {

    struct channel* chn;
    char tempbuffer[128];

    //Loop over every channel on the server
    for(chn = channels; chn != NULL; chn = chn->hh.next) {
        //If the channel is private (+p), don't display it
        if(get_flag(chn->mode, 'p')) {
            continue;
        }
        //Send a LIST message, containing the channel name, topic and number of users
        sprintf(tempbuffer, "%s %i :%s", chn->name, get_users_in_channel(chn), chn->topic);
        sock_send(usr->c_sock, "322", usr->nick, tempbuffer);
    }

    //Send end of LIST message
    sock_send(usr->c_sock, "323", usr->nick, ":End of LIST");
}

/** @brief MODE command - adds or removes a mode to/from a channel.
 *
 *  See code for a list of modes and their uses.
 *  @param usr Pointer to user struct
 *  @param strptr Pointer to the current reading location in the receiving buffer
 */
void set_mode(struct user* usr, char* strptr) {

    //Find the specified channel
    struct channel* chn = get_channel(usr, strtok_r(NULL, " ", &strptr));

    //Return if it doesn't exist
    if(chn == NULL) {
        return;
    }

    char flag[64];
    //If no argument provided, send current modes
    if(strptr[0] == '\0') {
        //Reuse flag array
        sprintf(flag, "%s %s", chn->name, chn->mode);
        sock_send(usr->c_sock, "324", usr->nick, flag);
    }
    //If an argument is provided
    else {
        //Clear the flag array and copy the flag argument given into it
        memset(flag, '\0', sizeof(flag));
        strcpy(flag, strtok_r(NULL, " ", &strptr));

        //Allocate an array to hold arguments (e.g. password) and copy them into it
        char args[64];
        memset(args, '\0', sizeof(args));
        strcpy(args, strtok_r(NULL, " ", &strptr));

        //Check if user wants to display bans
        if(strcmp(flag, "+b") == 0 && args[0] == '\0') {
            display_bans(chn, usr);
            return;
        }

        //All other mode actions require op privileges, so return if they don't have them
        if(!is_present(chn->operators, usr)) {
            send_error(chn, usr, 482, NULL);
            return;
        }

        //Switch on flag character
        switch(flag[1]) {
        //Toggle flags
        case 'm': //Moderated - only allow operators/voiced users to speak
        case 'n': //Only allow users in channel to send privmsgs
        case 'p': //Hide channel from queries e.g. list
        case 't': //Only let operators change topic
            set_flag(chn->mode, flag);
            break;
        //Give/remove operator status
        case 'o':
            if(!set_status(chn->operators, flag, args)) {
                return;
            }
            break;
        //Add/remove voice privileges
        case 'v':
            if(!set_status(chn->voiced, flag, args)) {
                return;
            }
            break;
        //Add/remove channel key (password)
        case 'k':
            if(!set_channel_pass(chn, flag, args)) {
                return;
            }
            break;
        //Change channel user limit
        case 'l':
            if(!set_user_limit(chn, flag, args)) {
                return;
            }
            break;
        //Change bans
        case 'b':
            set_ban(chn, flag, args);
            break;
        //Send an error if the given mode cannot be handled
        default:
            send_error(chn, usr, 472, flag);
            return;
        }

        //Add arguments for sending
        strcat(flag, " ");
        strcat(flag, args);

        //Send MODE message, notifying users of the mode change
        send_to_channel(chn, usr->nick, "MODE", chn->name, flag);
    }
}

/** @brief KICK command - forcibly removes a user from a channel.
 *  @param usr Pointer to user struct
 *  @param strptr Pointer to the current reading location in the receiving buffer
 */
void kick_user(struct user* usr, char* strptr) {

    //Find the channel
    struct channel* chn = get_channel(usr, strtok_r(NULL, " ", &strptr));

    //If the channel does exist, return
    if(chn == NULL) {
        return;
    }

    //Store the nickname to kick
    char buffer[64];
    strcpy(buffer, strtok_r(NULL, " ", &strptr));

    //Find the user with that nickname
    struct user* kicked;
    HASH_FIND_STR(users, buffer, kicked);

    //If the target user doesn't exist, send an error response and return
    if(kicked == NULL) {
        send_error(NULL, usr, 401, buffer);
        return;
    }

    //If no reason is provided, send the user's nickname as the reason
    if(strptr[0] == '\0') {
        sprintf(buffer, "%s %s", kicked->nick, kicked->nick);
    }
    //Otherwise, send the reason
    else {
        sprintf(buffer, "%s %s", kicked->nick, strptr);
    }
    //Send a KICK message to the channel, notifying the users
    send_to_channel(chn, usr->nick, "KICK", chn->name, buffer);

    //Remove kicked user from channel
    remove_from_channel(chn, kicked);
}
