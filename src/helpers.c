#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>

#include "defines.h"

/** @brief Implementation of strtok_r() - splits char arrays on the delimiter character.
 *
 *  @warning Destructive - removes delimiter character(s) from original string, and replaces
 *  with NULL character(s)
 *
 *  @note MinGW does not contain an implementation of strtok_r, making this necessary
 *
 *  @note Public domain strtok_r() by Charlie Gordon from comp.lang.c  9/14/2007
 *  @note http://groups.google.com/group/comp.lang.c/msg/2ab1ecbb86646684
 *  @note (Declaration that it's public domain):
 *  @note http://groups.google.com/group/comp.lang.c/msg/7c7b39328fefab9c
 *
 *  @param str Char array to split - may be NULL when splitting multiple times
 *  @param delim Character to split on when encountered
 *  @param nextp Pointer pointing at location of last split; modified internally
 *  @return Pointer to char array containing string resulting from split
 */
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
        if(*delim == '\r') {
            *str++ = '\0';
        }
    }

    *nextp = str;
    return ret;
}

/** @brief Converts a character array into uppercase.
 *  @param str String to convert
 */
void to_upper(char* str) {

    //Loop over the array - max length of received command is 512 bytes
    for(int i = 0; i < 512; i++) {
        if(str[i] == '\0') {
            return;
        }

        //Convert each character to uppercase
        str[i] = toupper(str[i]);
    }
}

/** @brief Send an error message to a user.
 *
 *  Both the chn and arg arguments are optional, and the passed values for each
 *  depend on the error being sent. This is because some errors are not related
 *  to a channel, and some errors require additional information
 *  (e.g. Invalid command, which requires an additional argument but no channel struct).
 *
 *  @param chn Pointer to channel struct
 *  @param usr Pointer to user struct
 *  @param error Error numeric
 *  @param arg Optional additional argument
 */
void send_error(struct channel* chn, struct user* usr, int error, char* arg) {
    char buffer[64];

    //Convert the error numeric into a char array
    char errorstr[4];
    sprintf(errorstr, "%i", error);

    //Switch on the numeric to determine the appropriate action to take
    switch(error) {
    case 403:
        //Add the formatted error message to the buffer
        sprintf(buffer, "%s :No such channel", arg);
        //And exit this switch statement
        break;
    case 404:
        sprintf(buffer, "%s :Cannot send to channel", chn->name);
        break;
    case 421:
        sprintf(buffer, "%s :Unknown command", arg);
        break;
    case 433:
        sprintf(buffer, "%s :Nickname is already in use", arg);
        break;
    case 471:
        sprintf(buffer, "%s :Cannot join channel (+l)", chn->name);
        break;
    case 472:
        sprintf(buffer, "%s :is unknown mode char to me for %s", arg, chn->name);
        break;
    case 474:
        sprintf(buffer, "%s :Cannot join channel (+b)", chn->name);
        break;
    case 475:
        sprintf(buffer, "%s :Cannot join channel (+k)", chn->name);
        break;
    case 479:
        sprintf(buffer, "%s :Illegal channel name", arg);
        break;
    case 482:
        sprintf(buffer, "%s :You're not a channel operator", chn->name);
        break;
    }

    //Send the error message to the user
    sock_send(usr->c_sock, errorstr, usr->nick, buffer);
}

/** @brief Sends a message to every user in a channel.
 *
 *  Used by commands such as PRIVMSG.
 *  @param chn Pointer to channel the message should be sent to
 *  @param hostname Hostname to prefix the message with
 *  @param command Command to send
 *  @param target Target of the message (generally the channel)
 *  @param message Message to send
 */
void send_to_channel(struct channel* chn, char* hostname, char* command, char* target, char* message) {
    //Loop over every user in the channel
    for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
        if(chn->users[i] == NULL) {
            return;
        }

        //Don't send PRIVMSG commands back to the sender
        if(strcmp(hostname, chn->users[i]->nick) == 0 && strcmp(command, "PRIVMSG") == 0) {
            continue;
        }

        //Send the message to each user
        sock_send_host(chn->users[i]->c_sock, hostname, command, target, message);
    }
}

/** @brief Retrieve a channel struct from the channels hashtable.
 *  @param chn_name Name of channel to retrieve
 *  @return Pointer to the appropriate channel struct
 */
struct channel* get_channel(struct user* usr, char* chn_name) {

    //Look up the channel in the hashtable
    struct channel* chn;
    HASH_FIND_STR(channels, chn_name, chn);

    //If the channel doesn't exist, send an error message
    if(chn == NULL) {
        send_error(NULL, usr, 403, chn_name);
        return NULL;
    }

    //Otherwise, return the channel
    return chn;
}

/** @brief Rearrange a user array so there is a continuous set of elements.
 *
 *  The user array is reordered so that it contains a continuous set of elements,
 *  followed by NULL references. This is done because every command that loops over
 *  the user list of a channel breaks when encountering a NULL reference - this is because
 *  it would be inefficient to keep searching the whole array. However, this means
 *  that any users separated by a NULL reference cannot be reached, meaning that the
 *  array must be reordered every time the ordering of the array changes (e.g. PART commands).
 *
 *  @param usrs Pointer to an array of user pointers
 */
void reorder_user_array(struct user** usrs) {
    //Loop over users
    for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
        //If user ref is null, move all users down 1 element
        if(usrs[i] == NULL) {
            for(int j = i+1; j < CHANNEL_MAX_USERS; j++) {
                usrs[j-1] = usrs[j];
            }
        }
    }
}

/** @brief Get the number of users in a channel.
 *  @param chn Pointer to the channel to check
 *  @return Number of users in the channel
 */
int get_users_in_channel(struct channel* chn) {
    //Loop over each users
    for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
        //When a NULL reference, we know we have found the last user, so return this number
        if(chn->users[i] == NULL) {
            return i;
        }
    }
    //If the loop doesn't terminate as above, the array must be full
    return CHANNEL_MAX_USERS;
}

/** @brief Check if a channel should be deleted, and do so if necessary.
 *
 *  Checks whether there are any users left in the channel. If there are none,
 *  the channel is deleted. Called by user_quit()
 *  @param chn Pointer to the channel to check
 *  @return Whether the channel was deleted or not
 */
bool check_remove_channel(struct channel* chn) {

    if(get_users_in_channel(chn) == 0) {
        HASH_DEL(channels, chn);
        free(chn);
        return true;
    }

    return false;
}

bool get_flag(char* modes, char flag) {
    for(int i = 0; i < CHANNEL_MAX_FLAGS; i++) {
        if(modes[i] == '\0') {
            return false;
        }
        if(modes[i] == flag) {
            return true;
        }
    }
    return false;
}

void set_flag(char* modes, char* flag) {
    //If adding a flag
    if(flag[0] == '+') {
        for(int i = 0; i < CHANNEL_MAX_FLAGS; i++) {
            if(modes[i] == '\0') {
                modes[i] = flag[1];
                return;
            }
        }
    }
    //Otherwise, remove it
    else {
        for(int i = 0; i < CHANNEL_MAX_FLAGS; i++) {
            if(modes[i] == flag[1]) {
                modes[i] = '\0';
                return;
            }
        }
    }
}

//Searches given user array for given user
//Used to check if user is chanop or voiced
bool is_present(struct user** userlist, struct user* usr) {
    for(int i = 0; i < 256; i++) {
        if(userlist[i] == NULL) {
            return false;
        }
        if(userlist[i] == usr) {
            return true;
        }
    }
    return false;
}

bool set_status(struct user** array, char* flag, char* args) {
    struct user* op;
    HASH_FIND_STR(users, args, op);

    if(op == NULL) {
        return false;
    }

    if(flag[0] == '-') {
        for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
            if(array[i] == op) {
                array[i] = NULL;
                return true;
            }
        }
        return false;
    }

    for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
        if(array[i] == NULL) {
            array[i] = op;
            return true;
        }
    }
    return false;
}

bool set_user_limit(struct channel* chn, char* flag, char* args) {
    //Remove limit
    if(flag[0] == '-' && get_flag(chn->mode, 'l')) {
        chn->limit = CHANNEL_MAX_USERS;
        set_flag(chn->mode, "-l");
        return true;
    }
    //Otherwise, change limit
    else {
        //Check if args is an int
        for(int i = 0; i < 8; i++) {

            //If null encountered and an argument (length > 0) is present
            if(args[i] == '\0' && i > 0) {
                break;
            }

            if(!isdigit(args[i])) {
                return false;
            }
        }

        chn->limit = atoi(args);
        set_flag(chn->mode, "+l");
        return true;
    }
    return false;
}

bool set_channel_pass(struct channel* chn, char* flag, char* args) {
    //Remove password
    if(flag[0] == '-' && get_flag(chn->mode, 'k')) {
        chn->password[0] = '\0';
        set_flag(chn->mode, "-k");
        return true;
    }
    else {
        //No argument provided
        if(args[0] == '\0') {
            return false;
        }

        strcpy(chn->password, args);
        set_flag(chn->mode, "+k");
    }
    return true;
}

//Returns true if mode message should be sent to channel
void set_ban(struct channel* chn, char* flag, char* args) {
    //Remove ban
    if(flag[0] == '-') {
        char* banptr;
        char* ban = strtok_r(chn->bans, " ", &banptr);

        for(int i = 0; i < 256; i++) {
            if(ban[0] == '\0') {
                return;
            }

            if(strcmp(ban, args) == 0) {
                int len = strlen(ban);
                for(int j = 0; j < len; j++) {
                    *ban = '\0';
                    ban++;
                }
                *ban = ' ';

                int nullcount = 0;
                for(int j = 0; j < 256; j++) {
                    if(chn->bans[j] == '\0') {
                        nullcount += 1;
                    }
                    else {
                        for(int k = j; k < 256; k++) {
                            chn->bans[k-nullcount] = chn->bans[k];
                        }
                        nullcount = 0;
                    }
                }
            }

            ban = strtok_r(NULL, " ", &banptr);
        }
    }

    strcat(chn->bans, args);
    strcat(chn->bans, " ");
}

void display_bans(struct channel* chn, struct user* usr) {
    char banlist[256];
    //Copy original ban array as strtok_r is destructive
    strcpy(banlist, chn->bans);

    char message[64];
    char* banptr;
    char* ban = strtok_r(banlist, " ", &banptr);

    for(int i = 0; i < 256; i++) {
        if(ban[0] == '\0') {
            break;
        }

        sprintf(message, "%s %s", chn->name, ban);
        sock_send(usr->c_sock, "367", usr->nick, message);
        ban = strtok_r(NULL, " ", &banptr);
    }

    sprintf(message, "%s :End of channel ban list", chn->name);
    sock_send(usr->c_sock, "368", usr->nick, message);
}

bool check_if_banned(struct channel* chn, struct user* usr) {
    char banlist[256];
    strcpy(banlist, chn->bans);

    char* banptr;
    char* ban = strtok_r(banlist, " ", &banptr);

    for(int i = 0; i < 256; i++) {
        if(ban[0] == '\0') {
            return false;
        }
        //Check if nick banned
        if(strcmp(ban, usr->nick) == 0) {
            return true;
        }
        //Check if IP banned
        if(strcmp(ban, usr->address) == 0) {
            return true;
        }

        char* ptr;
        //Check nick from mask
        if(strcmp(strtok_r(ban, "!", &ptr), usr->nick) == 0) {
            return true;
        }
        //Ignore text before @
        strtok_r(NULL, "@", &ptr);
        //Check address from mask
        if(strcmp(strtok_r(NULL, "@", &ptr), usr->address) == 0) {
            return true;
        }

        ban = strtok_r(NULL, " ", &banptr);
    }
    return false;
}

void remove_from_channel(struct channel* chn, struct user* usr) {
    for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
        if(chn->users[i] == NULL && usr->channels[i] == NULL) {
            break;
        }

        //Remove user from channel's users list
        if(chn->users[i] == usr) {
            chn->users[i] = NULL;
        }

        //Remove channel from user's channel list
        if(usr->channels[i] == chn) {
            usr->channels[i] = NULL;
        }
    }

    if(!check_remove_channel(chn)) {
        //Reorder channel's user array
        reorder_user_array(chn->users);
    }

    //Reorder user's channel list
    for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
        if(usr->channels[i] == NULL) {
            for(int j = i+1; j < CHANNEL_MAX_USERS; j++) {
                usr->channels[j-1] = usr->channels[j];
            }
        }
    }
}
