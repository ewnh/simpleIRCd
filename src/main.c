#include <stdio.h>

#include "socket.h"
#include "defines.h"

void start_handle_thread(struct user*);

int main()
{
    SOCK sock = server_setup();

    while(1) {
        struct user* usr = malloc(sizeof(struct user));
        sock_accept(sock, &(usr->c_sock), usr->address);
        start_handle_thread(usr);
    }
    server_shutdown();
    return 0;
}
