#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>

#include "defines.h"
#include "helpers.h"
#include "commands.h"
#include "socket.h"

extern char server_name; //defined in socket.c
extern char startup_time;
extern struct channel* channels; //defined in parser.c

void join_channel(struct user* hc, char* strptr) {

    char name[64];
    strcpy(name, strtok_r(NULL, " ", &strptr));

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

        memset(chn->operators, 0, sizeof(chn->operators));

        memset(chn->mode, '\0', sizeof(chn->mode));
        //Default modes
        chn->mode[0] = '+';
        chn->mode[1] = 'n';

        chn->limit = CHANNEL_MAX_USERS;

        memset(chn->password, '\0', sizeof(chn->password));

        HASH_ADD_STR(channels, name, chn);
        }

    else {
        //If channel limit reached
        if(chn->limit == get_users_in_channel(chn)) {
            char tempbuffer[64];
            sprintf(tempbuffer, "%s :Cannot join channel (+l)", chn->name);
            sock_send(hc->c_sock, "471", hc->nick, tempbuffer);
            return;
        }

        if(get_flag(chn->mode, 'k') && strcmp(chn->password, strtok_r(NULL, " ", &strptr)) != 0) {
            char tempbuffer[64];
            sprintf(tempbuffer, "%s :Cannot join channel (+k)", chn->name);
            sock_send(hc->c_sock, "475", hc->nick, tempbuffer);
            return;
        }
        //Add user point to channel's users array
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

void send_registration_messages(SOCK c_sock, char* nick, char* username, char* address) {
    char tempbuffer[128];
    sprintf(tempbuffer, "Welcome to the Internet Relay Network %s!%s@%s", nick, username, address);
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
    sprintf(tempbuffer, "%s %s %s * :%s", usr->nick, usr->username, usr->address, usr->realname);
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

void set_nick(struct user** users, struct user* usr, char* nick) {

    struct user* tempusr;
    HASH_FIND_STR(*users, nick, tempusr);

    if(tempusr == NULL) {
        //If nick previously set
        if(usr->nick[0] != '\0') {
            HASH_DEL(*users, usr);
        }

        //If user isn't in any channels and this is a nick change
        if(usr->channels[0] == NULL && usr->nick[0] != '\0') {
            sock_send_host(usr->c_sock, usr->nick, "NICK", "", nick);
        }
        //Loop over channels and send a channel message
        else {
            for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
                if(usr->channels[i] == NULL) {
                    break;
                }

                send_to_channel(usr->channels[i], usr->nick, "NICK", "", nick);
            }
        }

        strcpy(usr->nick, nick);
        HASH_ADD_STR(*users, nick, usr);
    }

    else {
        char error[50];
        sprintf(error, "%s :Nickname is already in use", nick);
        sock_send(usr->c_sock, "433", "*", error);
    }
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

        sprintf(tempbuffer, "%s %s %s %s %s G :%s %s", chn->name, chn->users[i]->username, chn->users[i]->address, &server_name,
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

    send_to_channel(chn, usr->nick, "PART", chn->name, strtok_r(NULL, " ", &strptr));

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

    if(!check_remove_channel(chn)) {
        //Reorder channel's user array
        reorder_user_array(chn->users);
    }

    //Reorder user's channel list
    for(int i = 0; i < CHANNEL_MAX_USERS; i++) {
        if(usr->channels[i] == NULL) {
            for(int j = i+1; j < CHANNEL_MAX_USERS; j++) {
                usr->channels[j-1] = usr->channels[j];
            }
        }
    }
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

                if(!check_remove_channel(usr->channels[i])) {
                    send_to_channel(usr->channels[i], usr->nick, "QUIT", "", message);
                }

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

void list_channels(struct user* usr) {

    struct channel* chn;
    char tempbuffer[128];

    for(chn = channels; chn != NULL; chn = chn->hh.next) {
        sprintf(tempbuffer, "%s %i :%s", chn->name, get_users_in_channel(chn), chn->topic);
        sock_send(usr->c_sock, "322", usr->nick, tempbuffer);
    }

    sock_send(usr->c_sock, "323", usr->nick, ":End of LIST");
}

void channel_mode(struct user* usr, char* strptr) {

    struct channel* chn = get_channel(usr, strtok_r(NULL, " ", &strptr));

    if(chn == NULL) {
        return;
    }

    //If no argument provided, send current modes
    if(strptr[0] == '\0') {
        sock_send(usr->c_sock, "MODE", chn->name, chn->mode);
    }
    else {
        char flag[8];
        memset(flag, '\0', sizeof(flag));
        char args[8];
        memset(args, '\0', sizeof(args));
        strcpy(flag, strtok_r(NULL, " ", &strptr));

        //Switch on flag character
        switch(flag[1]) {
        //Toggle flags
        case 'm':
        case 'n':
        case 'p':
        case 't':
            set_flag(chn->mode, flag);
            break;
        //Make specified user an operator for this channel
        case 'o':
        case 'v':
        //Channel key (password)
        case 'k':
            //Remove password
            if(flag[0] == '-' && get_flag(chn->mode, 'k')) {
                chn->password[0] = '\0';
                set_flag(chn->mode, "-k");
            }
            else {
                strcpy(args, strtok_r(NULL, " ", &strptr));

                //No argument provided
                if(args[0] == '\0') {
                    return;
                }

                strcpy(chn->password, args);
                set_flag(chn->mode, "+k");
            }
            break;
        //Channel limit
        case 'l':
            //Remove limit
            if(flag[0] == '-' && get_flag(chn->mode, 'l')) {
                chn->limit = CHANNEL_MAX_USERS;
                set_flag(chn->mode, "-l");
            }
            //Otherwise, change limit
            else {
                strcpy(args, strtok_r(NULL, " ", &strptr));

                //Check if args is an int
                for(int i = 0; i < 8; i++) {

                    //If null encountered and an argument (length > 0) is present
                    if(args[i] == '\0' && i > 0) {
                        break;
                    }

                    if(!isdigit(args[i])) {
                        return;
                    }
                }

                chn->limit = atoi(args);
                set_flag(chn->mode, "+l");
            }
            break;
        case 'b':
        case 'I':
        default:
            break;
        }

        //Add arguments for sending
        strcat(flag, " ");
        strcat(flag, args);

        send_to_channel(chn, usr->nick, "MODE", chn->name, flag);
    }
}
