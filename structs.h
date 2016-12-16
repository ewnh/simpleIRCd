#ifndef STRUCTS_H_INCLUDED
#define STRUCTS_H_INCLUDED

#include "socket.h"

struct user {
    SOCK c_sock;
    char message[513];
};

#endif // STRUCTS_H_INCLUDED
