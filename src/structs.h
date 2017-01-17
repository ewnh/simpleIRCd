#ifndef STRUCTS_H_INCLUDED
#define STRUCTS_H_INCLUDED

#include "uthash.h"
#include "socket.h"

struct user {
    SOCK c_sock;
    int is_cap_negotiating;
    char nick[10];
    char username[20];
    char realname[20];
    char modes[7];
    UT_hash_handle hh;
};

struct channel {
    char name[50];
    struct user* users[10];
    UT_hash_handle hh;
};

#endif // STRUCTS_H_INCLUDED
