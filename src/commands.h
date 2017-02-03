#ifndef COMMANDS_H_INCLUDED
#define COMMANDS_H_INCLUDED

#include "defines.h"

void join_channel(struct user*, char*);
void send_privmsg(char*, char*);
void send_registration_messages(SOCK, char*, char*);
void whois_user(struct user**, SOCK, char*, char*);
void set_topic(char*, char*);
void who_request(struct user*, char*);
void name_reply(struct user*, char*);
void user_part(struct user*, char*);
void user_quit(struct user*, char*);

#endif // COMMANDS_H_INCLUDED
