#include <stdio.h>

#include "socket.h"
#include "parser.h"

int main()
{
    SOCK sock = server_setup();
    struct user users[512];

    for(int i = 0; 1; i++) {

        users[i].c_sock = s_accept(sock);

        start_handle_thread(&users[i]);

    }
    server_shutdown();
	return 0;
}
