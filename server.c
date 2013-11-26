#include <libev/ev.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/shm.h>

#include "number.h"
#include "nullstring.h"
#include "nsocket.h"
#include "linklist.h"
#include "shmformat.h"

#define MAXLEN 1024
#define MAXNAME 20
#define WAITTIME 10

static void ServerSocketIoCallback(EV_P_ ev_io *watcher, int revents);
static void ClientSocketIoCallback(EV_P_ ev_io *watcher, int revents);
static void ClientFifoIoCallback(EV_P_ ev_io *watcher, int revents);
void SendToFifo();


/* shared memory */
int g_shm_id;
struct shmData *g_shm_addr = NULL; 
    
unsigned char g_recv_message[MAXLEN];
unsigned char g_send_message[MAXLEN];
char g_client_fifo[MAXLEN]; /* hold the client fifo the first time, then others */
int g_client_fifo_fd; /* hold the client fifo descriptor */

socklen_t g_client_len = sizeof(struct sockaddr_in);
ClientInf g_client_node;
struct sockaddr_in g_client_addr;

int g_server_socket_fd;

int main(int argc, char *argv[])
{
    /* create socket to accept */
    struct sockaddr_in server_addr;
    int port;

    if(argc != 2) {
        printf("./server <port>\n");
        exit(1);
    }

    if((g_server_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	perror("socket");
        exit(1);
    } 

    port = atoi(argv[1]);
    bzero(&server_addr, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if(bind(g_server_socket_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
	perror("bind");
        exit(2);
    }

    if(listen(g_server_socket_fd, 500) == -1) {
	perror("listen");
        exit(3);
    }

    /* set g_server_socket_fd to nonblock */
    int flags;
    if ((flags = fcntl(g_server_socket_fd, F_GETFL, 0)) < 0)
        printf("F_GETFL error");
    flags |= O_NONBLOCK;
    if (fcntl(g_server_socket_fd, F_SETFL, flags) < 0)
        printf("F_SETFL, error");    
    
    printf("listening...\n");
    
    /* create shared memory to hold the user client */
    if ((g_shm_id = shmget(IPC_PRIVATE, sizeof(struct shmData), 0666 | IPC_CREAT)) < 0) {
        perror("shmget");
        exit(1);
    }
    g_shm_addr = (struct shmData *)(shmat(g_shm_id, NULL, 0666));
    g_shm_addr->mark = 0;
    g_shm_addr->number = 0;

    /* initial loop and watcher */
    EV_P;
    loop = EV_DEFAULT;
    ev_io server_socket_io_w;
    ev_io_init(&server_socket_io_w, ServerSocketIoCallback, g_server_socket_fd, EV_READ);
    ev_io_start(EV_A_ &server_socket_io_w);
    ev_run(loop, 0);
	
    return 0;    
}

static void ServerSocketIoCallback(EV_P_ ev_io *watcher, int revents)
{
    /* accept client accept */
    pid_t pid = 1; /* client process ID */

    if((g_client_node.sockfd = accept(g_server_socket_fd, (struct sockaddr *)&g_client_addr, &g_client_len)) == -1)
	perror("accept");
    else {
        printf("Connect to %s at %d success\n",
	       inet_ntoa(g_client_addr.sin_addr), ntohs(g_client_addr.sin_port));
        g_client_node.addr_in = g_client_addr;
        pid = fork();
    }

    /* child continue & parent return */
    if (pid != 0) {
	close(g_client_node.sockfd);
	return;
    }
    
    /* add user to shared memory */
    g_shm_addr = (struct shmData *)(shmat(g_shm_id, NULL, 0666));
    
    if(!nrecv(g_client_node.sockfd, g_recv_message, MAXLEN, 0, WAITTIME)) {
        close(g_client_node.sockfd);
        return;
    }
    nullStrcpyFrom(g_client_node.name, g_recv_message, 7);
    shmAddUser(g_shm_addr, g_client_node);

    /* set to nonblock, don't use it before add user */
    int flags;
    if ((flags = fcntl(g_client_node.sockfd, F_GETFL, 0)) < 0)
        printf("F_GETFL error");
    flags |= O_NONBLOCK;
    if (fcntl(g_client_node.sockfd, F_SETFL, flags) < 0)
        printf("F_SETFL, error");

    
    /* create client fifo to communicate with each other */
    /* set the path */
    strcpy(g_client_fifo, "/tmp/chat-process-");
    strcat(g_client_fifo, g_client_node.name);
    strcat(g_client_fifo, ".fifo");

    if ((mkfifo(g_client_fifo, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH) < 0) && (errno != EEXIST))
        printf("can't creat %s", g_client_fifo);
    /* open with O_NONBLOCK */
    g_client_fifo_fd = open(g_client_fifo, O_RDONLY | O_NONBLOCK, 0);
    
    /* initial loop and watcher
     * destroy server loop */
    ev_loop_destroy(loop);
    struct ev_loop *process_loop = EV_DEFAULT;

    ev_io client_socket_io_w;
    ev_io client_fifo_io_w;
    ev_io_init(&client_fifo_io_w, ClientFifoIoCallback, g_client_fifo_fd, EV_READ);
    ev_io_start(process_loop, &client_fifo_io_w);
    ev_io_init(&client_socket_io_w, ClientSocketIoCallback, g_client_node.sockfd, EV_READ);
    ev_io_start(process_loop, &client_socket_io_w);
    ev_run(loop, 0);

    return;
}

static void ClientSocketIoCallback(EV_P_ ev_io *watcher, int revents)
{
    /* receive message & send them to the right fifo */
    /* receive message */
    if (!nrecv(g_client_node.sockfd, g_recv_message, MAXLEN, 0, WAITTIME)) {
        shmDeleteUser(g_shm_addr, g_client_node);
        return;
    }

    /* logout */
    if (g_recv_message[5] == 0xFB) {
        shmDeleteUser(g_shm_addr, g_client_node);
        //close(g_client_node.sockfd);
        //unlink(g_client_fifo);
        exit(0);
        return;
    }

    /* list users */
    if (g_recv_message[5] == 0xFD) {
        g_send_message[0] = g_send_message[1] = g_send_message[2] = g_send_message[3] = '0';
        g_send_message[4] = '|';
        g_send_message[5] = '\0';
        LinkList client_node_link = createLinkList();

        /* read from the shared memory and set a mark in the memory */
        shm2list(client_node_link, g_shm_addr); 
        LinkList S = client_node_link; /* S for format g_send_message */
        LinkList P = client_node_link; /* P for send g_send_message*/
        S = S->next;
        P = P->next;

        /* format g_send_messgae */
        while (S != NULL) {
            nullStrcat(g_send_message, S->data.name, 5);
            nullStrcat(g_send_message, "|", 5);
            S = S->next;
        }
            
        convertNumber(g_send_message, nullStrlen(g_send_message, 5) - 4 + 1);

        /* get from_name from the g_recv_message */
        unsigned char from_name[MAXLEN];
        int iterator;
            
        for (iterator = 7; iterator != nullStrlen(g_recv_message, 5); ++iterator) {
            if (g_recv_message[iterator] != '|') {
                from_name[iterator - 7] = g_recv_message[iterator];
                from_name[iterator - 6] = '\0';
            } else
                break;
        }

        /* TODO(lvyiwang): Never check the send watcher, all socket and fifo */
        /* send the list message to client */
        while(P != NULL) {
            if (strcmp(P->data.name, from_name) == 0) {
                if(!nsend(P->data.sockfd, g_send_message, nullStrlen(g_send_message, 5) + 1, 0, WAITTIME)) {
                    deleteUser(client_node_link, g_client_node);
                    close(g_client_node.sockfd);
                    return;
                }
                break;
            }
            P = P->next;
        }
        
        list2shm(client_node_link, g_shm_addr);
	    
	    return;
    }
	
    /* if the message are not list & logout, flag@message format will be send below
     * check the message & send them to the write fifo
     */ 
    SendToFifo();
}

static void ClientFifoIoCallback(EV_P_ ev_io *watcher, int revents)
{
    /* read a message from the fifo */    
    read(g_client_fifo_fd, g_send_message, 4);
    int size = getNumber(g_send_message);
    read(g_client_fifo_fd, g_send_message + 4, size);
    
    /* send the message to the user */
    /* TODO(lvyiwang): not a good way to disconnect from the server */
    if(!nsend(g_client_node.sockfd, g_send_message, nullStrlen(g_send_message, 5) + 1, 0, WAITTIME)) {
        shmDeleteUser(g_shm_addr, g_client_node);
        close(g_client_node.sockfd);
	exit(1); /* TODO(lvyiwang): How to stop sending all the time */
        return;
    }    
}

void SendToFifo()
{
    /* send to all or specific person flag@messge format */
    if (g_recv_message[5] == 0xFC) {
        unsigned char from_name[MAXLEN]; /* who send the message */
        unsigned char to_name[MAXLEN]; /* who will receive thsi message */
        int mark_at_sign, iterator; /* mark_at_sign '|' */
            
        for (iterator = 7; iterator != nullStrlen(g_recv_message, 6); ++iterator) {
            if (g_recv_message[iterator] != '|') {
                from_name[iterator - 7] = g_recv_message[iterator];
                from_name[iterator - 6] = '\0';
            } else
                break;
        }
            
        mark_at_sign = iterator++;
            
        for (; iterator != nullStrlen(g_recv_message, 6); ++iterator) {
            if (g_recv_message[iterator] != '|') {
                to_name[iterator - 1 - mark_at_sign] = g_recv_message[iterator];
                to_name[iterator - mark_at_sign] = '\0';
             } else
                break;
        }

        LinkList client_node_link = createLinkList();
        shm2list(client_node_link, g_shm_addr); // set mark_at_sign to 1
        LinkList L, M; /* L for send to all fifo; M for send to the specific fifo */
        L = client_node_link;
        L = L->next;
        M = client_node_link;
        M = M->next;

	/* if "all", send to every fifo of the process */
        if (strcmp("all", to_name) == 0) {
	    int other_client_fifo_fd;
            char other_client_fifo[MAXLEN];
	    
            while (M != NULL) {
		/* :todo: no check the write here */
	        strcpy(other_client_fifo, "/tmp/chat-process-");
	        strcat(other_client_fifo, M->data.name);
	        strcat(other_client_fifo, ".fifo");
	        
	        other_client_fifo_fd = open(other_client_fifo, O_WRONLY | O_NONBLOCK, 0);

                if ((write(other_client_fifo_fd, g_recv_message, nullStrlen(g_recv_message, 5) + 1)) == -1) {
                    LinkList temp = M->next;
                    deleteUser(client_node_link, (*M).data);
                    M = temp;
		    //close(other_client_fifo_fd);
                    continue;
                }

		/* TODO(lvyiwang): Will they receive this message after I close the fifo */
		/* The fifo will hold the message and never delete them, if I use close */
		//close(other_client_fifo_fd);

                M = M->next;
            }
            
            list2shm(client_node_link, g_shm_addr);
	    return;
        }

	/* if send to "user", just send to the specific user */
        while(L != NULL) {
	    int other_client_fifo_fd;
            char other_client_fifo[MAXLEN];
            if (strcmp(L->data.name, to_name) == 0) {
	        strcpy(other_client_fifo, "/tmp/chat-process-");
	        strcat(other_client_fifo, L->data.name);
	        strcat(other_client_fifo, ".fifo");		
	        other_client_fifo_fd = open(other_client_fifo, O_WRONLY | O_NONBLOCK, 0);
		
                if ((write(other_client_fifo_fd, g_recv_message, nullStrlen(g_recv_message, 5) + 1)) == -1) {
                    LinkList temp = L->next;
                    deleteUser(client_node_link, (*L).data);
                    L = temp;
                }

		//close(other_client_fifo_fd);
                break;
            }
            
            L = L->next;
        }

        list2shm(client_node_link, g_shm_addr);
	return;
    }
}

// TODO(lvyiwang): I can't use close & unlink to fifo, otherwise the watcher will not work well
// I don't know why
