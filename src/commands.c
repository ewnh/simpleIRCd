#include <stdio.h>

#include "structs.h"

void join_channel(struct channel** channels, struct user* hc, char* name) {

    struct channel* chn;
    HASH_FIND_STR(*channels, name, chn);

    if(chn == NULL) { //channel doesn't already exist
        chn = malloc(sizeof(struct channel));
        if(strlen(name) > 50) {
            printf("Channel name too long\n");
            return;
        }
        strcpy(chn->name, name);

        memset(chn->users, 0, sizeof(chn->users));
        chn->users[0] = hc;

        HASH_ADD_STR(*channels, name, chn);
        }

    else {
        for(int i = 0; i < 10; i++) {
            if(chn->users[i] == NULL) {
                chn->users[i] = hc;
                break;
            }
        }
    }
    printf("Joined channel %s\n", name);
}
