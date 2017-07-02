/** @file
 *  @brief Contains helper functions
 */
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>

#include "defines.h"

/** @brief Implementation of strsplit() - splits char arrays on the delimiter character.
 *
 *  @warning Destructive - removes delimiter character(s) from original string, and replaces
 *  with NULL character(s)
 *
 *  @note MinGW does not contain an implementation of strsplit, making this necessary
 *
 *  @note Public domain strsplit() by Charlie Gordon from comp.lang.c  9/14/2007
 *  @note http://groups.google.com/group/comp.lang.c/msg/2ab1ecbb86646684
 *  @note (Declaration that it's public domain):
 *  @note http://groups.google.com/group/comp.lang.c/msg/7c7b39328fefab9c
 *
 *  @param str Char array to split - may be NULL when splitting multiple times
 *  @param delim Character to split on when encountered
 *  @param nextp Pointer pointing at location of last split; modified internally
 *  @return Pointer to char array containing string resulting from split
 */
char* strsplit(char *str, const char *delim, char **nextp) {
    char *ret;

    //If passed in array is empty, set pointer to location of last split
    if (str == NULL) {
        str = *nextp;
    }

    //Find occurrences of delimiter in string
    str += strspn(str, delim);
    //If none, return string
    if (*str == '\0') {
        return str;
    }

    //Store new string
    ret = str;
    //Find delimiter characters
    str += strcspn(str, delim);

    //Remove delimiter(s)
    if (*str) {
        *str++ = '\0';
        //If delimiter is \r, set the next character (\n) to NULL
        if(*delim == '\r') {
            *str++ = '\0';
        }
    }

    //Move pointer forwards
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
    char buffer[512];

    //Convert the error numeric into a char array
    char errorstr[4];
    sprintf(errorstr, "%i", error);

    //Switch on the numeric to determine the appropriate action to take
    switch(error) {
    case 401:
        //Add the formatted error message to the buffer
        sprintf(buffer, "%s :No such nick", arg);
        //And exit this switch statement
        break;
    case 403:
        sprintf(buffer, "%s :No such channel", arg);
        break;
    case 404:
        sprintf(buffer, "%s :Cannot send to channel", chn->name);
        break;
    case 421:
        sprintf(buffer, "%s :Unknown command", arg);
        break;
    case 432:
        sprintf(buffer, "%s :Erroneous nickname", arg);
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
    net_send(usr->c_sock, errorstr, usr->nick, buffer);
}

/** @brief Sends a message to every user in a channel.
 *
 *  Used by commands such as send_privmsg()
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
        net_send_host(chn->users[i]->c_sock, hostname, command, target, message);
    }
}

/** @brief Retrieve a channel struct from the channels hashtable.
 *  @param usr Pointer to user struct
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
 *  @param usrs Pointer to an array of user pointers to sort
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

/** @brief Deletes a channel
 *  @param chn Pointer to the channel to delete
 */
void delete_channel(struct channel* chn) {
    //Remove the channel from the hashtable
    HASH_DEL(channels, chn);
    //And free the allocated memory
    free(chn);
}

/** @brief Check if a channel has a certain mode flag set.
 *  @param modes Char array containing the channel's modes
 *  @param flag Flag to search for
 *  @return True if the flag has been set, false otherwise
 */
bool get_flag(char* modes, char flag) {
    //Loop over each flag
    for(int i = 0; i < CHANNEL_MAX_FLAGS; i++) {
        //If the end of the array has been reached, the flag isn't set
        if(modes[i] == '\0') {
            return false;
        }
        //If the flag has been found, return true
        if(modes[i] == flag) {
            return true;
        }
    }
    return false;
}

/** @brief Sets a mode flag for a channel
 *  @brief modes Char array containing the channel's modes
 *  @brief flag Flag to set
 */
void set_flag(char* modes, char* flag) {
    //If the first character is +, the flag is being added
    if(flag[0] == '+') {
        //Loop over every set flag
        for(int i = 0; i < CHANNEL_MAX_FLAGS; i++) {
            //Set the new flag in the first available space
            if(modes[i] == '\0') {
                modes[i] = flag[1];
                return;
            }
        }
    }
    //Otherwise, the flag is being removed
    else {
        //Loop over every flag and remove the specified flag
        for(int i = 0; i < CHANNEL_MAX_FLAGS; i++) {
            if(modes[i] == flag[1]) {
                modes[i] = '\0';
                return;
            }
        }
    }
}

/** @brief Searches a user array for a given user.
 *
 *  Used to determine whether a user is present in a channel, and whether the
 *  user is an operator or voiced.
 *
 *  @param userlist Pointer to the array of user pointers
 *  @param usr User struct to search for
 *  @return True if the user is present, false otherwise
 */
bool is_present(struct user** userlist, struct user* usr) {
    //Loop over every user
    for(int i = 0; i < 256; i++) {
        //If the end of the array is reached, the user isn't present
        if(userlist[i] == NULL) {
            return false;
        }
        //If they are, return true
        if(userlist[i] == usr) {
            return true;
        }
    }
    return false;
}

/** @brief Adds/removes a user to/from a given user array.
 *  @param array Pointer to a user array
 *  @param flag Mode flag, specifying how to change the array (e.g. +o, -v)
 *  @param args Nickname of the user to add/remove
 *  @return True if the user has successfully been added/removed, false otherwise
 */
bool set_status(struct user** array, char* flag, char* args) {
    //Find the specified user in the users hashtable
    struct user* op;
    HASH_FIND_STR(users, args, op);

    //Return if they don't exist
    if(op == NULL) {
        return false;
    }

    //If the first character of the flag is -, we are removing this user
    if(flag[0] == '-') {
        //Loop over given array
        for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
            //Remove the user if found
            if(array[i] == op) {
                array[i] = NULL;
                return true;
            }
        }
        //If the user isn't present, return false
        return false;
    }

    //Otherwise, we are adding the user to the array
    for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
        //Loop over the array and add them in the next available space
        if(array[i] == NULL) {
            array[i] = op;
            return true;
        }
    }
    return false;
}

/** @brief Sets the user limit of a channel.
 *  @param chn Pointer to channel struct being modified
 *  @param flag Flag, determining whether the limit is being changed or removed
 *  @param args New user limit
 *  @return True if the user limit has been changed, false otherwise
 */
bool set_user_limit(struct channel* chn, char* flag, char* args) {
    //If the first character of the flag is -, remove the limit
    if(flag[0] == '-' && get_flag(chn->mode, 'l')) {
        //Set the limit to default
        chn->limit = CHANNEL_MAX_USERS;
        //Remove the flag
        set_flag(chn->mode, "-l");
        return true;
    }
    //Otherwise, change the limit
    else {
        //Check if args is an int
        for(int i = 0; i < 8; i++) {
            //If null encountered and an argument (length > 0) is present, args is an int
            if(args[i] == '\0' && i > 0) {
                break;
            }

            //If one of the character is not an int, return false
            if(!isdigit(args[i])) {
                return false;
            }
        }

        //Set the new channel limit
        chn->limit = atoi(args);
        //Add the limit flag
        set_flag(chn->mode, "+l");
        return true;
    }
    return false;
}

/** @brief Sets the password (channel key) of a channel.
 *  @param chn Pointer to channel struct being modified
 *  @param flag Flag, determining whether the password is being changed or removed
 *  @param args New password
 *  @return True if the password has been changed, false otherwise
 */
bool set_channel_pass(struct channel* chn, char* flag, char* args) {
    //If the first character of the flag is -, remove the password
    if(flag[0] == '-' && get_flag(chn->mode, 'k')) {
        //Remove the password
        chn->password[0] = '\0';
        //Remove the flag
        set_flag(chn->mode, "-k");
        return true;
    }
    else {
        //If no argument provided
        if(args[0] == '\0') {
            return false;
        }

        //Set the new password
        strcpy(chn->password, args);
        //Set the correct flag
        set_flag(chn->mode, "+k");
    }
    return true;
}

/** @brief Add/remove a channel ban.
 *  @param chn Pointer to channel struct being modified
 *  @param flag Flag, determining how the ban is being changed
 *  @param args Ban to add
 */
void set_ban(struct channel* chn, char* flag, char* args) {
    //Again, f the first character of the flag is -, remove the ban
    if(flag[0] == '-') {
        char* banptr;
        //Get the first set ban
        char* ban = strsplit(chn->bans, " ", &banptr);

        //Loop over all set bans
        for(int i = 0; i < 256; i++) {
            //Return if the end of the bans array has been reached
            if(ban[0] == '\0') {
                return;
            }

            //If this is the ban we want to remove
            if(strcmp(ban, args) == 0) {
                int len = strlen(ban);
                //Replace the ban with null characters
                for(int j = 0; j < len; j++) {
                    *ban = '\0';
                    ban++;
                }
                //Preserve the space separating the bans
                *ban = ' ';

                int nullcount = 0;
                //Loop over all bans again
                for(int j = 0; j < 256; j++) {
                    //Count all the continuous null characters
                    if(chn->bans[j] == '\0') {
                        nullcount += 1;
                    }
                    else {
                        //Move each character down, removing all null characters
                        for(int k = j; k < 256; k++) {
                            chn->bans[k-nullcount] = chn->bans[k];
                        }
                        nullcount = 0;
                    }
                }
            }

            //Get the next ban
            ban = strsplit(NULL, " ", &banptr);
        }
    }

    //Otherwise, add the ban
    strcat(chn->bans, args);
    strcat(chn->bans, " ");
}

/** @brief Send the list of bans to the user.
 *  @param chn Pointer to the channel struct containing the bans
 *  @param usr Pointer to user struct
 */
void display_bans(struct channel* chn, struct user* usr) {
    char banlist[256];
    //Copy original ban array as strsplit is destructive
    strcpy(banlist, chn->bans);

    //Allocate message array
    char message[64];
    char* banptr;
    //Get the first ban
    char* ban = strsplit(banlist, " ", &banptr);

    //Loop over every ban
    for(int i = 0; i < 256; i++) {
        //If the end of the ban arrays is reached, break
        if(ban[0] == '\0') {
            break;
        }

        //Store the ban in the message buffer
        sprintf(message, "%s %s", chn->name, ban);
        //Send the ban to the user
        net_send(usr->c_sock, "367", usr->nick, message);
        //Get the next ban
        ban = strsplit(NULL, " ", &banptr);
    }

    //Send the end of bans list message
    sprintf(message, "%s :End of channel ban list", chn->name);
    net_send(usr->c_sock, "368", usr->nick, message);
}

/** @brief Check if the user is banned from a channel.
 *  @param chn Pointer to channel struct
 *  @param usr Pointer to user struct
 *  @return True if the user is banned, false otherwise
 */
bool check_if_banned(struct channel* chn, struct user* usr) {
    //Copy original ban array as strsplit is destructive
    char banlist[256];
    strcpy(banlist, chn->bans);

    char* banptr;
    //Get the first ban
    char* ban = strsplit(banlist, " ", &banptr);

    for(int i = 0; i < 256; i++) {
        //If the end of the bans array is reached, return
        if(ban[0] == '\0') {
            return false;
        }
        //Check if nick is banned
        if(strcmp(ban, usr->nick) == 0) {
            return true;
        }
        //Check if IP is banned
        if(strcmp(ban, usr->address) == 0) {
            return true;
        }

        //If the ban is a mask, check against the user
        //Ban masks are of the form nick!user@address
        char* ptr;
        //Check mask to see if the nick is banned
        if(strcmp(strsplit(ban, "!", &ptr), usr->nick) == 0) {
            return true;
        }
        //Ignore text before @, we don't handle username bans
        strsplit(NULL, "@", &ptr);
        //Check if the address is banned
        if(strcmp(strsplit(NULL, "@", &ptr), usr->address) == 0) {
            return true;
        }

        //Get the next ban
        ban = strsplit(NULL, " ", &banptr);
    }
    //Otherwise, the user is not currently banned
    return false;
}

/** @brief Remove the user from a channel.
 *
 *  Dissociates the user struct from the channel's users list, and the
 *  channel struct from the user's channels list.
 *  @param chn Pointer to channel struct to remove the user from
 *  @param usr Pointer to user struct being removed from the channel
 */
void remove_from_channel(struct channel* chn, struct user* usr) {
    //Loop over every user and channel
    for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
        //If we have reached the end of both the user and channel arrays, break the loop
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

    //Check if the channel should be removed
    //Removing the channel is handled in user_quit() and user_part()
    if(get_users_in_channel(chn) != 0) {
        //If not, reorder channel's user array
        reorder_user_array(chn->users);
    }

    //Reorder user's channel list
    for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
        if(usr->channels[i] == NULL) {
            //Move every channel down a space if a NULL reference is encountered
            for(int j = i+1; j < CHANNEL_MAX_USERS; j++) {
                usr->channels[j-1] = usr->channels[j];
            }
        }
    }
}
