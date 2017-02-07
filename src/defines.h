#ifndef STRUCTS_H_INCLUDED
#define STRUCTS_H_INCLUDED

#include "../lib/uthash.h"
#include "socket.h"

#define CHANNEL_MAX_USERS 100

struct user {
    SOCK c_sock;
    char address[16];
    int is_cap_negotiating;
    char nick[10];
    char username[20];
    char realname[20];
    char modes[7];
    struct channel* channels[CHANNEL_MAX_USERS];
    UT_hash_handle hh;
};

struct channel {
    char name[50];
    char topic[50];
    char mode[10];
    struct user* users[CHANNEL_MAX_USERS];
    UT_hash_handle hh;
};

#endif // STRUCTS_H_INCLUDED
