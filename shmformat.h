#ifndef SHMFORMAT_H
#define SHMFORMAT_H

#include "linklist.h"

struct shmData {
    int mark; // 共享内存的标志位，有进程在读取这设置为1，此时其他进程处于等待状态
    int number; // 目前连接的用户数量
    ListNode client[500];
};

void shmAddUser(struct shmData *shmaddr, ClientInf clientNode); // 向共享内存添加用户连接
void shmDeleteUser(struct shmData *shmaddr, ClientInf clientNode);  // 向共享内存删除用户连接
void shm2list(LinkList clientLink, struct shmData *shmaddr); // 把保存用户连接的共享内存转化为链表
void list2shm(LinkList clientLink, struct shmData *shmaddr); // 把链表存储到共享内存里面

#endif
