#include <stdio.h>

#ifdef _WIN32
#else
#include <string.h>
#include <unistd.h>
#endif

#include "socket.h"
#include "parser.h"

int main()
{
    SOCK sock = server_setup();
    char message[513] = {0};
    struct user users[512];

    for(int i = 0; 1; i++) {

        users[i].c_sock = s_accept(sock);
        users[i].message = message;

        start_handle_thread(&users[i]);

    }
    server_shutdown();
	return 0;
}
