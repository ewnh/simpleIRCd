#include <stdio.h>

#include "socket.h"
#include "structs.h"

extern char server_name; //defined in socket.c
extern char startup_time;

void send_to_channel(struct channel* chn, char* hostname, char* command, char* target, char* message) {
    for(int i = 0; i < 10; i++) {
        if(chn->users[i] == NULL) {
            return;
        }

        //Don't send PRIVMSG commands back to the sender
        if(strcmp(hostname, chn->users[i]->nick) == 0 && strcmp(command, "PRIVMSG") == 0) {
            continue;
        }

        sock_send_host(chn->users[i]->c_sock, hostname, command, target, message);
    }
}

void join_channel(struct channel** channels, struct user* hc, char* name) {

    if(strlen(name) > 50) {
        printf("Channel name too long\n");
        return;
    }

    struct channel* chn;
    HASH_FIND_STR(*channels, name, chn);

    if(chn == NULL) { //channel doesn't already exist
        chn = malloc(sizeof(struct channel));

        strcpy(chn->name, name);

        chn->topic[0] = '\0';

        memset(chn->users, 0, sizeof(chn->users));
        chn->users[0] = hc;

        HASH_ADD_STR(*channels, name, chn);
        }

    else {
        for(int i = 0; i < 10; i++) {
            if(chn->users[i] == NULL) {
                chn->users[i] = hc;
                break;
            }
        }
    }
    printf("Joined channel %s\n", name);

    for(int i = 0; i < 10; i++) {
        if(hc->channels[i] == NULL) {
            hc->channels[i] = chn;
            break;
        }
    }

    send_to_channel(chn, hc->nick, "JOIN", name, "");

    //If channel topic set
    if(chn->topic[0] != '\0') {
        char tempbuffer[128];
        sprintf(tempbuffer, "%s :%s", name, chn->topic);
        sock_send(hc->c_sock, "332", hc->nick, tempbuffer);
    }
    //Don't send a message if no topic set
}

void send_privmsg(struct channel** channels, char* sender, char* strptr) {

    char target[50];
    strcpy(target, strtok_r(NULL, " ", &strptr));

    struct channel* chn;
    HASH_FIND_STR(*channels, target, chn);

    if(chn == NULL) {
        return;
    }

    send_to_channel(chn, sender, "PRIVMSG", target, strptr);
}

void send_registration_messages(SOCK c_sock, char* nick, char* username) {
    char tempbuffer[128];
    sprintf(tempbuffer, "Welcome to the Internet Relay Network %s!%s", nick, username);
    sock_send(c_sock, "001", nick, tempbuffer);
    sprintf(tempbuffer, "Your host is %s, running simpleIRCd", &server_name);
    sock_send(c_sock, "002", nick, tempbuffer);
    sprintf(tempbuffer, "This server was started %s", &startup_time);
    sock_send(c_sock, "003", nick, tempbuffer);
    sprintf(tempbuffer, "%s simpleIRCd TODO", &server_name);
    sock_send(c_sock, "004", nick, tempbuffer);

    //MOTD
    sprintf(tempbuffer, ":- %s Message of the day - ", &server_name);
    sock_send(c_sock, "375", nick, tempbuffer);
    sock_send(c_sock, "372", nick, ":- MOTD goes here");
    sock_send(c_sock, "376", nick, ":End of MOTD command");
}

void whois_user(struct user** users, SOCK c_sock, char* sender, char* target) {

    struct user* usr;
    HASH_FIND_STR(*users, target, usr);

    if(usr == NULL) {
        return;
    }

    char tempbuffer[128];

    //Send RPL_WHOISUSER: <nick> <user> <host> * :<real name>
    sprintf(tempbuffer, "%s %s %s * :%s", usr->nick, usr->username, "TESTHOST", usr->realname);
    sock_send(c_sock, "311", sender, tempbuffer);

    //Send RPL_WHOISSERVER: <nick> <server> :<server info>
    sprintf(tempbuffer, "%s %s :info", usr->nick, &server_name);
    sock_send(c_sock, "312", sender, tempbuffer);

    //Send RPL_WHOISCHANNELS: <nick> :*( ( "@" / "+" ) <channel> " " )
    sprintf(tempbuffer, "%s :", usr->nick);
    for(int i = 0; i < 10; i++) {
        if(usr->channels[i] == NULL) {
            break;
        }

        strcat(tempbuffer, usr->channels[i]->name);
        strcat(tempbuffer, " ");
    }
    sock_send(c_sock, "319", sender, tempbuffer);

    //Send RPL_ENDOFWHOIS: <nick> :End of WHOIS list
    sprintf(tempbuffer, "%s :End of WHOIS list", usr->nick);
    sock_send(c_sock, "318", sender, tempbuffer);
}

void set_topic(struct channel** channels, char* nick, char* strptr) {

    char chn_name[50];
    strcpy(chn_name, strtok_r(NULL, " ", &strptr));

    struct channel* chn;
    HASH_FIND_STR(*channels, chn_name, chn);

    if(chn == NULL) {
        return;
    }

    strcpy(chn->topic, ++strptr);

    send_to_channel(chn, nick, "TOPIC", chn->name, chn->topic);
}
