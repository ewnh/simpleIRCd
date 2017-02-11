#ifndef HELPERS_H_INCLUDED
#define HELPERS_H_INCLUDED

void send_to_channel(struct channel*, char*, char*, char*, char*);
struct channel* get_channel(struct user*, char*);
void reorder_user_array(struct user**);
int get_users_in_channel(struct channel*);
bool check_remove_channel(struct channel*);
bool get_flag(char*, char);
void set_flag(char*, char*);
bool set_oper(struct channel*, char*, char*);
bool set_user_limit(struct channel*, char*, char*);
bool set_channel_pass(struct channel*, char*, char*);
bool set_ban(struct channel*, struct user*, char*, char*);

#endif // HELPERS_H_INCLUDED
