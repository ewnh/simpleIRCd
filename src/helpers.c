#include <stdio.h>
#include <stdbool.h>

#include "defines.h"

extern struct channel* channels;

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

void set_flag(char* modes, char flag) {
    for(int i = 0; i < CHANNEL_MAX_FLAGS; i++) {
        if(modes[i] == '\0') {
            modes[i] = flag;
        }
    }
}
