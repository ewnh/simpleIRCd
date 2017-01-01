#ifndef STRUCTS_H_INCLUDED
#define STRUCTS_H_INCLUDED

#include "uthash.h"
#include "socket.h"

struct user {
    SOCK c_sock;
    char nick[9];
    char username[20];
    char realname[20];
    char modes[7];
};

struct channel {
    char name[50];
    struct user* users[10];
    UT_hash_handle hh;
};

#endif // STRUCTS_H_INCLUDED
