#ifndef SOCKET_H_INCLUDED
#define SOCKET_H_INCLUDED

#ifdef _WIN32
    #include <winsock2.h>
    #define SOCK SOCKET
#else
    #include <sys/socket.h>
    #define SOCK int
#endif

SOCK server_setup();
void server_shutdown();
void sock_accept(SOCK, SOCK*, char*);
void sock_send_host(SOCK, char*, char*, char*, char*);
void sock_send(SOCK, char*, char*, char*);
int sock_recv(SOCK, char*, char*, char**);
void sock_close(SOCK);

#endif // SOCKET_H_INCLUDED
