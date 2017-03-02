/** @file
 *  @brief Header file for commands.c
 */

#ifndef COMMANDS_H_INCLUDED
#define COMMANDS_H_INCLUDED

#include "defines.h"

void join_channel(struct user*, char*);
void send_privmsg(struct user*, char*);
void send_registration_messages(SOCK, char*, char*, char*);
void whois_user(SOCK, char*, char*);
void set_topic(struct user*, char*);
void set_nick(struct user*, char*);
void who_request(struct user*, char*);
void name_reply(struct user*, char*);
void user_part(struct user*, char*);
void user_quit(struct user*, char*);
void list_channels(struct user*);
void set_mode(struct user*, char*);
void kick_user(struct user*, char*);

#endif // COMMANDS_H_INCLUDED
