#include <stdio.h>

#include "socket.h"
#include "parser.h"

int main()
{
    SOCK sock = server_setup();

    while(1) {
        struct user* usr = malloc(sizeof(struct user));
        usr->c_sock = s_accept(sock);
        start_handle_thread(usr);
    }
    server_shutdown();
	return 0;
}
