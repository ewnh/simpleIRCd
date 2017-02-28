/** @file
 *  @brief Contains main entry point for program, which accepts user connections.
 */

#include <stdio.h>

#include "socket.h"
#include "parser.h"

/** @brief Program entry point
 *
 *  Accepts a connection and uses start_handle_thread() to create a
 *  new thread running handle_connection()
 *
 *  @see handle_connection()
 *  @see start_handle_thread()
 *  @return Program status
 */
int main()
{
    //Create a server socket
    SOCK sock = server_setup();

    while(1) {
        //Allocate memory for a new user struct
        struct user* usr = malloc(sizeof(struct user));
        //Assign accepted socket to new user struct
        usr->c_sock = sock_accept(sock);
        //Call start_handle_thread()
        start_handle_thread(usr);
    }
    //Shutdown the server
    server_shutdown();
	return 0;
}
