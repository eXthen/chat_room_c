#include "linklist.h"

LinkList createLinkList()
{
    LinkList L = (LinkList) malloc(sizeof(ListNode));
    L->next = NULL;
    return L;
}

void insertEnd(LinkList L, ClientInf user)
{
    LinkList s,p;
    p = L;
    while(p->next != NULL)
        p = p->next;
    
    s = (LinkList) malloc(sizeof(ListNode));
    s->data = user;
    s->next = p->next;
    p->next = s;    
    printf("user %s add in \n", user.name);
}

int inList(LinkList L, ClientInf user)
{
    if (L->next == NULL) return 0;
    LinkList p;
    p = L->next;

    while (p->next != NULL) {
        if (!strcmp(user.name, p->data.name)) return 1;
        p = p->next;
    }

    if (!strcmp(user.name, p->data.name)) return 1;

    return 0;
}

void addUser(LinkList L, ClientInf user)
{
    if (!inList(L, user))
        insertEnd(L, user);
}

void deleteUser(LinkList L, ClientInf user)
{
    if (inList(L, user)) {
        LinkList s, p;
        s = L;
        p = L->next;

        while (p->next != NULL) {
            if (!strcmp(user.name, p->data.name)) {
                printf("user %s delete OR disconnect\n", user.name);
                s->next = p->next;
                return;
            }
            p = p->next;
            s = s->next;
        }

        if (!strcmp(user.name, p->data.name)) {
            s->next = NULL;
            printf("user %s delete OR disconnect\n", user.name);

        }
    }
}
