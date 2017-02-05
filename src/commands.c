#include <stdio.h>

#include "commands.h"
#include "socket.h"
#include "defines.h"

extern char server_name; //defined in socket.c
extern char startup_time;
extern struct channel* channels; //defined in parser.c

void send_to_channel(struct channel* chn, char* hostname, char* command, char* target, char* message) {
    for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
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

struct channel* get_channel(struct user* usr, char* chn_name) {

    struct channel* chn;
    HASH_FIND_STR(channels, chn_name, chn);

    if(chn == NULL) {
        char error[67]; //Max channel name length (50) + length of text
        sprintf(error, "%s :No such channel", chn_name);

        sock_send(usr->c_sock, "403", usr->nick, error);

        return NULL;
    }

    return chn;
}

//Rearrange user array so there is a continuous set of elements
//Used to fill in any null references, ensuring null only occurs at the end of an array
void reorder_user_array(struct user** usrs) {
    //Loop over users
    for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
        //If user ref is null, move all users down 1 element
        if(usrs[i] == NULL) {
            for(int j = i+1; j < CHANNEL_MAX_USERS; j++) {
                usrs[j-1] = usrs[j];
            }
        }
    }
}

void join_channel(struct user* hc, char* name) {

    if(strlen(name) > 50) {
        printf("Channel name too long\n");
        return;
    }

    struct channel* chn;
    HASH_FIND_STR(channels, name, chn);

    if(chn == NULL) { //channel doesn't already exist
        chn = malloc(sizeof(struct channel));

        strcpy(chn->name, name);

        chn->topic[0] = '\0';

        memset(chn->users, 0, sizeof(chn->users));
        chn->users[0] = hc;

        HASH_ADD_STR(channels, name, chn);
        }

    else {
        for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
            if(chn->users[i] == NULL) {
                chn->users[i] = hc;
                break;
            }
        }
    }
    printf("Joined channel %s\n", name);

    for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
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

    //Send names message
    name_reply(hc, name);
}

void send_privmsg(struct user* usr, char* strptr) {

    char chn_name[50];
    strcpy(chn_name, strtok_r(NULL, " ", &strptr));

    struct channel* chn = get_channel(usr, chn_name);

    if(chn == NULL) {
        return;
    }

    send_to_channel(chn, usr->nick, "PRIVMSG", chn_name, strptr);
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
    for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
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

void set_topic(struct user* usr, char* strptr) {

    struct channel* chn = get_channel(usr, strtok_r(NULL, " ", &strptr));

    if(chn == NULL) {
        return;
    }

    strcpy(chn->topic, ++strptr);

    send_to_channel(chn, usr->nick, "TOPIC", chn->name, chn->topic);
}

void who_request(struct user* usr, char* chn_name) {

    struct channel* chn = get_channel(usr, chn_name);

    if(chn == NULL) {
        return;
    }

    char tempbuffer[128];

    for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
        if(chn->users[i] == NULL) {
            break;
        }

        sprintf(tempbuffer, "%s %s %s %s %s G :%s %s", chn->name, chn->users[i]->username, "TESTHOST", &server_name,
                chn->users[i]->nick, "0", chn->users[i]->realname);
        sock_send(usr->c_sock, "352", usr->nick, tempbuffer);
    }

    sprintf(tempbuffer, "%s :End of WHO list", chn->name);
    sock_send(usr->c_sock, "315", usr->nick, tempbuffer);
}

void name_reply(struct user* usr, char* chn_name) {

    struct channel* chn = get_channel(usr, chn_name);

    if(chn == NULL) {
        return;
    }

    char tempbuffer[128];
    sprintf(tempbuffer, "= ");
    strcat(tempbuffer, chn_name);
    strcat(tempbuffer, " :");

    for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
        if(chn->users[i] == NULL) {
            break;
        }

        strcat(tempbuffer, chn->users[i]->nick);
        strcat(tempbuffer, " ");
    }

    //RPL_NAMREPLY
    sock_send(usr->c_sock, "353", usr->nick, tempbuffer);

    //RPL_ENDOFNAMES
    sprintf(tempbuffer, "%s :End of NAMES list", chn->name);
    sock_send(usr->c_sock, "366", usr->nick, tempbuffer);
}

void user_part(struct user* usr, char* strptr) {

    struct channel* chn = get_channel(usr, strtok_r(NULL, " ", &strptr));

    if(chn == NULL) {
        return;
    }

    for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
        if(chn->users[i] == NULL && usr->channels[i] == NULL) {
            break;
        }

        //Remove user from channel's users list
        if(chn->users[i] == usr) {
            chn->users[i] = NULL;
        }

        //Remove channel from user's channel list
        if(usr->channels[i] == chn) {
            usr->channels[i] = NULL;
        }
    }

    //Reorder channel's user array
    reorder_user_array(chn->users);

    //Reorder user's channel list
    for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
        if(usr->channels[i] == NULL) {
            for(int j = i+1; j < CHANNEL_MAX_USERS; j++) {
                usr->channels[j-1] = usr->channels[j];
            }
        }
    }

    send_to_channel(chn, usr->nick, "PART", chn->name, strtok_r(NULL, " ", &strptr));
}

void user_quit(struct user* usr, char* message) {

    //Loop over every channel the user belongs to
    for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
        if(usr->channels[i] == NULL) {
            break;
        }

        //Loop over users in that channel and remove usr
        for(int j = 0; j < CHANNEL_MAX_USERS; j++) {
            if(usr->channels[i]->users[j] == usr) {
                usr->channels[i]->users[j] = NULL;
                send_to_channel(usr->channels[i], usr->nick, "QUIT", "", message);
                break;
            }
        }
    }

    //Loop over each channel the user is connected to
    for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
        if(usr->channels[i] == NULL) {
            continue;
        }
        //Reorder the channel's user array
        reorder_user_array(usr->channels[i]->users);
    }
    sock_send(usr->c_sock, "ERROR", ":Closing Link:", message);
}
