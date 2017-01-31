#ifndef COMMANDS_H_INCLUDED
#define COMMANDS_H_INCLUDED

#include "defines.h"

void join_channel(struct channel**, struct user*, char*);
void send_privmsg(struct channel**, char*, char*);
void send_registration_messages(SOCK, char*, char*);
void whois_user(struct user**, SOCK, char*, char*);
void set_topic(struct channel**, char*, char*);
void who_request(struct channel**, struct user*, char*);
void name_reply(struct channel**, struct user*, char*);

#endif // COMMANDS_H_INCLUDED
