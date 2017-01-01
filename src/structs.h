#ifndef STRUCTS_H_INCLUDED
#define STRUCTS_H_INCLUDED

#include "uthash.h"
#include "socket.h"

struct user {
    SOCK c_sock;
    char nick[9];
};

struct channel {
    char name[50];
    struct user* users[10];
    UT_hash_handle hh;
};

#endif // STRUCTS_H_INCLUDED
