/** @file
 *  @brief Holds preprocessor defines, along with user and channel struct definitions
 */

#ifndef STRUCTS_H_INCLUDED
#define STRUCTS_H_INCLUDED

#include <stdbool.h>
#include "../lib/uthash.h"
#include "socket.h"

/** @brief Maximum number of users in any channel */
#define CHANNEL_MAX_USERS 100
/** @brief Maximum number of channel flags */
#define CHANNEL_MAX_FLAGS 10

extern char server_name[]; //defined in socket.c
extern char startup_time[];

extern struct channel* channels; //defined in parser.c
extern struct user* users;

/** @struct user
 *  @brief Holds information about a user.
 */
struct user {
    /** Socket the user is connected to */
    SOCK c_sock;
    /** User's IP address */
    char address[16];
    /** Whether the user has completed registration (i.e. sent valid NICK
     *  and USER commands) or not
     */
    bool is_registered;
    /** Whether the user is quitting and has used the QUIT command or not.
     *  Used to determine if a QUIT message should be automatically sent on disconnection
     */
    bool has_sent_quit;
    /** User's nickname, set using the NICK command */
    char nick[10];
    /** User's username, set using the USER command */
    char username[20];
    /** User's real name, set using the USER command */
    char realname[20];
    /** List of channels this user is connected to */
    struct channel* channels[CHANNEL_MAX_USERS];
    /** Struct used by uthash - allows user structs to be used in a hashtable */
    UT_hash_handle hh;
};

/** @struct channel
 *  @brief Holds information about a channel.
 */
struct channel {
    /** Channel name */
    char name[50];
    /** Channel topic */
    char topic[50];
    /** List of channel modes */
    char mode[CHANNEL_MAX_FLAGS];
    /** List of users connected to this channel */
    struct user* users[CHANNEL_MAX_USERS];
    /** List of voiced (permitted to speak in a moderated channel) users */
    struct user* voiced[CHANNEL_MAX_USERS];
    /** List of operators */
    struct user* operators[CHANNEL_MAX_USERS];
    /** List of bans */
    char bans[256];
    /** Channel password (used with +k) */
    char password[10];
    /** Channel maximum user limit (used with +l) */
    int limit;
    /** Struct used by uthash - allows channel structs to be used in a hashtable */
    UT_hash_handle hh;
};

#endif // STRUCTS_H_INCLUDED
