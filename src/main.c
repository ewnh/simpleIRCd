/** @file
 *  @brief Contains main entry point for program, and accepts user connections
 */

#include <stdio.h>

#include "socket.h"
#include "defines.h"

void start_handle_thread(struct user*);

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
    SOCK sock = net_setup();

    while(1) {
        //Allocate memory for a new user struct
        struct user* usr = malloc(sizeof(struct user));
        //Assign accepted socket to new user struct
        net_accept(sock, &(usr->c_sock), usr->address);
        //Call start_handle_thread()
        start_handle_thread(usr);
    }
    //Shutdown the server
    net_shutdown();
    return 0;
}
