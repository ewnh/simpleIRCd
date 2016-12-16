#ifndef STRUCTS_H_INCLUDED
#define STRUCTS_H_INCLUDED

#include "socket.h"

struct user {
    SOCK c_sock;
    char* message;
};

#endif // STRUCTS_H_INCLUDED
