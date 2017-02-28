/** @file
 *  @brief Contains server code.
 */

#include <stdio.h>

#ifdef _WIN32
#include <process.h>
#else
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#endif

#include "socket.h"

/**
 *  @brief Contains data about a connected user
 */
struct user {
    /** Socket containing user connection*/
    SOCK c_sock;
    /** Pointer to char array used to hold input */
    char* message;
};

/** @brief Handles network input
 *
 *  Receives network input, prints to the screen and closes connections when appropriate.
 *  @param hc Pointer to user struct
 */
void handle_connection(struct user* hc) {
    while(1) {
        //Receive data and check status to see if we should close the connection
        int recvstat = s_recv(hc->c_sock, hc->message);
        if(recvstat == 1) {
            break;
        }
        //Print received data to the console
        printf("%i: %s", hc->c_sock, hc->message);
        //Clear message buffer
        memset(hc->message, '\0', 513);
    }
    //Close connection
    printf("Connection closed\n");
    #ifdef _WIN32
    closesocket(hc->c_sock);
	#else
    close(hc->c_sock);
    #endif
    return;
}

/** @brief Program entry point
 *
 *  Accepts a connection and creates a new thread running handle_connection()
 *
 *  @see handle_connection()
 *  @return Program status
 */
int main()
{
    //Setup the server
    SOCK sock = server_setup();
    char message[513] = {0};
    struct user users[512];

    for(int i = 0; 1; i++) {

        //Accept a connection and store it in a user struct
        users[i].c_sock = s_accept(sock);
        users[i].message = message;

        //Send a message to check everything works
        s_send(users[i].c_sock, "Test\n");

        //Start a new thread running handle_connection()
        #ifdef _WIN32
        _beginthread((void *)handle_connection, 0, &users[i]);
        #else
        pthread_t conthread;
        pthread_create(&conthread, NULL, (void *)handle_connection, &users[i]);
        #endif
    }

    //Cleanup Winsock
    #ifdef _WIN32
    WSACleanup();
	#endif

	return 0;
}
