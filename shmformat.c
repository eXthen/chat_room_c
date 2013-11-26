#include "shmformat.h"
#include "linklist.h"


void shmAddUser(struct shmData *shmaddr, ClientInf clientNode)
{
    LinkList P = createLinkList();
    shm2list(P, shmaddr);
    addUser(P, clientNode);
    list2shm(P, shmaddr);
}

void shmDeleteUser(struct shmData *shmaddr, ClientInf clientNode)
{
    LinkList P = createLinkList();
    shm2list(P, shmaddr);
    deleteUser(P, clientNode);
    list2shm(P, shmaddr);
}

void shm2list(LinkList clientLink, struct shmData *shmaddr) // set mark to 1
{
    int i;
    while (shmaddr->mark == 1)
        usleep(1000);
    
    shmaddr->mark = 1;
    
    LinkList P = clientLink;
    
    for (i = 0; i < (shmaddr->number); ++i) {
        P->next = &(shmaddr->client[i]);
        P = P->next;
        *P = shmaddr->client[i];
    }
    
    P->next = NULL;
}

void list2shm(LinkList clientLink, struct shmData *shmaddr) // set mark to 0
{
    int number = 0;
    LinkList P = clientLink;
    
    while ((P->next) != NULL) {
        P = P->next;
        shmaddr->client[number] = *P;
        ++number;
    }
    
    shmaddr->number = number;
    shmaddr->mark = 0;
}
