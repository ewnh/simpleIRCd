/** @file
 *  @brief Header file for commands.c
 */
 
#ifndef COMMANDS_H_INCLUDED
#define COMMANDS_H_INCLUDED

void join_channel(struct channel**, struct user*, char*);
void send_privmsg(struct channel**, char*, char*, char*);
void send_registration_messages(SOCK, char*, char*);

#endif // COMMANDS_H_INCLUDED
