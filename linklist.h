#ifndef LINKLIST_H
#define LINKLIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>

typedef struct _clientinf {
    unsigned char name[10];
    struct sockaddr_in addr_in;
    int sockfd;
    pthread_t pid;
} ClientInf;

typedef struct _LNode {
    ClientInf data;
    struct _LNode *next;
} ListNode, *LinkList;


extern LinkList createLinkList(void);
extern void insertEnd(LinkList L, ClientInf user);
extern int inList(LinkList L, ClientInf user);
extern void addUser(LinkList L, ClientInf user);
extern void deleteUser(LinkList L, ClientInf user);

#endif
