#ifndef STRUCTS_H_INCLUDED
#define STRUCTS_H_INCLUDED

#include <stdbool.h>
#include "../lib/uthash.h"
#include "socket.h"

#define CHANNEL_MAX_USERS 100
#define CHANNEL_MAX_FLAGS 10

extern char server_name[]; //defined in socket.c
extern char startup_time[];

extern struct channel* channels; //defined in parser.c
extern struct user* users;

struct user {
    SOCK c_sock;
    char address[16];
    bool is_registered;
    char nick[10];
    char username[20];
    char realname[20];
    struct channel* channels[CHANNEL_MAX_USERS];
    UT_hash_handle hh;
};

struct channel {
    char name[50];
    char topic[50];
    char mode[CHANNEL_MAX_FLAGS];
    struct user* users[CHANNEL_MAX_USERS];
    struct user* voiced[CHANNEL_MAX_USERS];
    struct user* operators[CHANNEL_MAX_USERS];
    char bans[256];
    char password[10];
    int limit;
    UT_hash_handle hh;
};

#endif // STRUCTS_H_INCLUDED
