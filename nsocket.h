#ifndef NSOCKET_H
#define NSOCKET_H

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>

extern int nsend(int sockfd, const void *msg, int len, int flags, int seconds);
extern int nrecv(int sockfd, void *buf, int len, unsigned int flags, int seconds);

#endif
