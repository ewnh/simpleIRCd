#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>

#include "defines.h"

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
        if(*delim == '\r') {
            *str++ = '\0';
        }
    }

    *nextp = str;
    return ret;
}

void to_upper(char* str) {

    //Max length of received command is 512 bytes
    for(int i = 0; i < 512; i++) {
        if(str[i] == '\0') {
            return;
        }

        str[i] = toupper(str[i]);
    }
}

void send_to_channel(struct channel* chn, char* hostname, char* command, char* target, char* message) {
    for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
        if(chn->users[i] == NULL) {
            return;
        }

        //Don't send PRIVMSG commands back to the sender
        if(strcmp(hostname, chn->users[i]->nick) == 0 && strcmp(command, "PRIVMSG") == 0) {
            continue;
        }

        sock_send_host(chn->users[i]->c_sock, hostname, command, target, message);
    }
}

struct channel* get_channel(struct user* usr, char* chn_name) {

    struct channel* chn;
    HASH_FIND_STR(channels, chn_name, chn);

    if(chn == NULL) {
        char error[67]; //Max channel name length (50) + length of text
        sprintf(error, "%s :No such channel", chn_name);

        sock_send(usr->c_sock, "403", usr->nick, error);

        return NULL;
    }

    return chn;
}

//Rearrange user array so there is a continuous set of elements
//Used to fill in any null references, ensuring null only occurs at the end of an array
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

int get_users_in_channel(struct channel* chn) {
    for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
        if(chn->users[i] == NULL) {
            return i;
        }
    }
}

//Check if a channel needs removing and remove if necessary
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

            //TODO: this only kinda works
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
        }char* strtok_r(char *, const char *, char **);
void to_upper(char*);
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
