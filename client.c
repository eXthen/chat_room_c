#include <libev/ev.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "number.h"
#include "nullstring.h"
#include "nsocket.h"

#define MAXLEN 1024
#define MAXNAME 20
#define WAITTIME 10

static void StdioCallback (EV_P_ ev_io *watcher, int revents); 
static void SocketCallback (EV_P_ ev_io *watcher, int revents); 
int SetUserName();
void FormatMessage(unsigned char *g_send_message, unsigned char *g_user_input);
void PrintMessage(unsigned char *g_recv_message);

EV_P;
ev_io g_stdio_watcher;
ev_io g_socket_watcher;
int g_socket_fd;
unsigned char g_user_name[MAXNAME];
unsigned char g_user_input[MAXLEN];
unsigned char g_send_message[MAXLEN];  
unsigned char g_recv_message[MAXLEN];

int main(int argc, char *argv[])
{
    /* connect to socket */
    struct sockaddr_in server_addr;
    int port;
    
    if (argc != 3) {
        printf("./client IP port\n");
        exit(1);
    }
    
    port = atoi(argv[2]);
    
    if ((g_socket_fd = socket(AF_INET,SOCK_STREAM,0)) == -1) {
        perror("socket");
        exit(1);
    }
    
    bzero(&server_addr,sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(port);

    if (connect(g_socket_fd, (struct sockaddr *) &server_addr, sizeof(struct sockaddr)) == -1) {
	perror("connect");
        exit(2);
    }

    /* set g_socker_fd to nonblock */
    int flags;
    if ((flags = fcntl(g_socket_fd, F_GETFL, 0)) < 0)
        printf("F_GETFL error");
    flags |= O_NONBLOCK;
    if (fcntl(g_socket_fd, F_SETFL, flags) < 0)
        printf("F_SETFL, error");

    /* send user name to the server */
    if (SetUserName() == 0) {
	printf("Set User Name Failed!\n");
	return 0;
    }

    printf("flag@message: ");
    fflush(stdout);

    /* start loop & watch the socket/stdio */
    loop = EV_DEFAULT;
    ev_io_init(&g_stdio_watcher, StdioCallback, STDIN_FILENO, EV_READ);
    ev_io_start(EV_A_ &g_stdio_watcher);
    ev_io_init(&g_socket_watcher, SocketCallback, g_socket_fd, EV_READ);
    ev_io_start(EV_A_ &g_socket_watcher);
    ev_run(loop, 0);

    return 0;
}

/* Get the user name and send it to the server */
int SetUserName()
{
    printf("Enter Your Name: ");
    scanf("%s", g_user_name);

    /* format the user_name and send to server */
    convertNumber(g_send_message, 3 + strlen(g_user_name) + 1);
    nullInsertType(g_send_message, 'a');
    nullStrcat(g_send_message, g_user_name, 6);

    if (!nsend(g_socket_fd, g_send_message, nullStrlen(g_send_message, 6) + 1, 0, WAITTIME)) {
        printf("Failed to login!\n");
        close(g_socket_fd);
        return 0;
    }
    
    return 1;
}

/* read from the stdio
 * set the g_socket_watcher to READ | WRITE, this will send the message
 */
static void StdioCallback (EV_P_ ev_io *watcher, int revents)
{
    scanf("%s", g_user_input);
    FormatMessage(g_send_message, g_user_input);

    /* set to read and write to send messages */
    ev_io_stop(EV_A_ &g_socket_watcher);
    ev_io_set(&g_socket_watcher, g_socket_fd, EV_READ | EV_WRITE);
    ev_io_start(EV_A_ &g_socket_watcher);
    
    printf("flag@message: ");
    fflush(stdout);
}

/* if READ, read it to the g_recv_message
 * if WRITE, send it to the server
 */
static void SocketCallback (EV_P_ ev_io *watcher, int revents)
{
    if (revents & EV_READ) {
        if (!nrecv(g_socket_fd, g_recv_message, MAXLEN, 0, WAITTIME)) {
            printf("### Disconnect from server! ####### \n");
            close(g_socket_fd);
            exit(0);
            return;
        } else {
            PrintMessage(g_recv_message);
            printf("flag@message: ");
	    fflush(stdout);
        }
    } else if (revents & EV_WRITE){
        if (!nsend(g_socket_fd, g_send_message, nullStrlen(g_send_message, 6) + 1, 0, WAITTIME)) {
            printf("Disconnect!");
            close(g_socket_fd);
            return;
        }

	/* set to READ */
	ev_io_stop(loop, &g_socket_watcher);
	ev_io_set(&g_socket_watcher, g_socket_fd, EV_READ);
	ev_io_start(EV_A_ &g_socket_watcher);
    }
}

void FormatMessage(unsigned char *g_send_message, unsigned char *g_user_input)
{
    /* format list messages */
    if (strcmp(g_user_input, "list") == 0) {
        convertNumber(g_send_message, 3 + strlen(g_user_name) + 1);
        nullInsertType(g_send_message, 'd');
        nullStrcat(g_send_message, g_user_name, 6);
        return;
    }

    /* logout: exit immediately */
    if (!strcmp(g_user_input, "logout")) {
        convertNumber(g_send_message, 3 + strlen(g_user_name) + 1);
        nullInsertType(g_send_message, 'b');
        nullStrcat(g_send_message, g_user_name, 6);
	
	/* send at here and disconnect immediately */
	nsend(g_socket_fd, g_send_message, nullStrlen(g_send_message, 6) + 1, 0, WAITTIME);
	
        printf("### Disconnect ###\n");
        exit(1);
        return;
    }

    /* flag_name: all OR the user name
     * send_message: the messge the client want to send
     */
    unsigned char flag_name[MAXLEN];
    unsigned char send_message[MAXLEN];
    int mark_at_sign = 0; /* the position of '@' */
    int iterator;

    for (iterator = 0; iterator != strlen(g_user_input); ++iterator) {
        if (g_user_input[iterator] == '@') {
            mark_at_sign = iterator;
            continue;
        }

        if (mark_at_sign == 0)
            flag_name[iterator] = g_user_input[iterator];
        else
            send_message[iterator - 1 - mark_at_sign] = g_user_input[iterator];
    }

    flag_name[mark_at_sign] = '\0';
    send_message[iterator - 1 - mark_at_sign] = '\0';

    /* format the g_user_input to g_send_message */
    convertNumber(g_send_message, 3 + strlen(g_user_name) + 1 + strlen(flag_name) + 1 + strlen(send_message) + 1);
    nullInsertType(g_send_message, 'c');
    nullStrcat(g_send_message, g_user_name, 6);
    nullStrcat(g_send_message, "|", 6);
    nullStrcat(g_send_message, flag_name, 6);
    nullStrcat(g_send_message, "|", 6);
    nullStrcat(g_send_message, send_message, 6);
}

void PrintMessage(unsigned char *g_recv_message)
{
    unsigned char from_name[MAXLEN]; /* who send the message */
    unsigned char to_name[MAXLEN]; /* who receive the message all OR user*/
    unsigned char send_message[MAXLEN]; /* the real message in the g_recv_message */
    int mark_at_sign; /* the position of the sign '|' */
    int iterator;

    /* recv_message format: size|from|to|message[|]
     * 0xFC for send to user message
     * others for send to user user_list
     */
    if (g_recv_message[5] == 0xFC) {
        for (iterator = 7; iterator != nullStrlen(g_recv_message, 6); ++iterator) {
            if (g_recv_message[iterator] != '|') {
                from_name[iterator - 7] = g_recv_message[iterator];
                from_name[iterator - 6] = '\0';
            } else
                break;
        }
            
        mark_at_sign = iterator++;

        for (iterator; iterator != nullStrlen(g_recv_message, 6); ++iterator) {
            if (g_recv_message[iterator] != '|') {
                to_name[iterator - 1 - mark_at_sign] = g_recv_message[iterator];
                to_name[iterator - mark_at_sign] = '\0';
            } else
                break;
        }
        
        mark_at_sign = iterator++;
        
        for (iterator; iterator != nullStrlen(g_recv_message, 6); ++iterator) {
            send_message[iterator - 1 - mark_at_sign] = g_recv_message[iterator];
            send_message[iterator - mark_at_sign] = '\0';
        }

        printf("\n %s send to %s: %s \n", from_name, to_name, send_message);
    } else {
        printf("\n*** User list: *** \n");
        mark_at_sign = 4;
        
        for (iterator = 5; iterator != nullStrlen(g_recv_message, 6); ++iterator) {
            if (g_recv_message[iterator] != '|') {
                from_name[iterator - 1 - mark_at_sign] = g_recv_message[iterator];
                from_name[iterator - mark_at_sign] = '\0';
            } else {
                mark_at_sign = iterator;
                printf("    %s \n", from_name);
            }
        }
    }
}
