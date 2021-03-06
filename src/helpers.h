/** @file
 *  @brief Header file for helpers.c
 */

#ifndef HELPERS_H_INCLUDED
#define HELPERS_H_INCLUDED

#include <stdbool.h>
#include "defines.h"

char* strsplit(char *, const char *, char **);
void to_upper(char*);
void send_error(struct channel*, struct user*, int, char*);
void send_to_channel(struct channel*, char*, char*, char*, char*);
struct channel* get_channel(struct user*, char*);
void reorder_user_array(struct user**);
int get_users_in_channel(struct channel*);
void delete_channel(struct channel*);
bool get_flag(char*, char);
void set_flag(char*, char*);
bool is_present(struct user**, struct user*);
bool set_status(struct user**, char*, char*);
bool set_user_limit(struct channel*, char*, char*);
bool set_channel_pass(struct channel*, char*, char*);
void set_ban(struct channel*, char*, char*);
void display_bans(struct channel*, struct user*);
bool check_if_banned(struct channel*, struct user*);
void remove_from_channel(struct channel*, struct user*);

#endif // HELPERS_H_INCLUDED
