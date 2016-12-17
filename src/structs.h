#ifndef STRUCTS_H_INCLUDED
#define STRUCTS_H_INCLUDED

#include "uthash.h"
#include "socket.h"

struct user {
    SOCK c_sock;
    char message[513];
};

struct channel {
    struct user* users;
    UT_hash_handle hh;
};

#endif // STRUCTS_H_INCLUDED
