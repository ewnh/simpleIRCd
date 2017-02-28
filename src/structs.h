/** @file
 *  @brief Defines user and channel structs.
 */

#ifndef STRUCTS_H_INCLUDED
#define STRUCTS_H_INCLUDED

#include "uthash.h"
#include "socket.h"

/** @struct user
 *  @brief Holds information about a user.
 *  @see handle_connection()
 */
struct user {
    /** Socket the user is connected to */
    SOCK c_sock;
    /** User's nickname, set using the NICK command */
    char nick[10];
    /** User's username, set using the USER command */
    char username[20];
    /** User's real name, set using the USER command */
    char realname[20];
    /** User's modes - indications about the user's status */
    char modes[7];
};

/** @struct channel
 *  @brief Holds information about a channel.
 */
struct channel {
    /** Channel name */
    char name[50];
    /** List of users connected to channel */
    struct user* users[10];
    /** Struct used by uthash - allows channel structs to be used in a hashtable */
    UT_hash_handle hh;
};

#endif // STRUCTS_H_INCLUDED
