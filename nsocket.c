#include <time.h>
#include <stdio.h>

#include "nsocket.h"
#include "number.h"

int continueRecv(void);
int continueSend(void);

extern int errno;

int nsend(int sockfd, const void *msg, int len, int flags, int seconds)
{
    clock_t start = clock();
    clock_t end;
    int totalSendSize = 0;
    int sendSize = 0;
    while (totalSendSize < len) {
        sendSize = send(sockfd, msg + totalSendSize, len - totalSendSize, flags);
        
        if (sendSize < 0) {
            if (!continueSend()) {
                return 0;
            }
			
            sendSize = 0;
        }
        
        totalSendSize += sendSize;

        end = clock();
        if ((end - start) > (CLOCKS_PER_SEC * seconds)) {
            return 0;
        }
    }

    return 1;
}

int nrecv(int sockfd, void *buf, int len, unsigned int flags, int seconds)
{
    clock_t start;
    clock_t end;
    int totalRecvSize = 0;
    int recvSize = 0;
    int size;
    
    totalRecvSize = recv(sockfd, buf + recvSize, len, flags);
    start = clock();
    
    while (totalRecvSize < 4) {

        recvSize = recv(sockfd, buf + recvSize, len, flags);
   
        if (recvSize < 0) {
            if (!continueRecv()) {
                return 0;
          }
			
            recvSize = 0;
        }
        
        totalRecvSize += recvSize;
        
        end = clock();
        if ((end - start) > (CLOCKS_PER_SEC * seconds)) {
            return 0;
        }
    }
    
    size = getNumber(buf) + 4;

    while (totalRecvSize < size) {
        recvSize = recv(sockfd, buf + totalRecvSize, len - totalRecvSize, flags);
        
        if (recvSize < 0) {
            if (!continueRecv()) {
                return 0;
            }
			
            recvSize = 0;
        }
        
        totalRecvSize += recvSize;

        end = clock();
		
        if ((end - start) > (CLOCKS_PER_SEC * seconds)) {
            return 0;
        }
    }

    return 1;
}

int continueRecv(void)
{
    switch (errno) {
        case EINTR:
        case EAGAIN:
            return 1;
        case EBADF:
            printf("recv EBADF\n");
            return 0;
        case EFAULT:
            printf("recv EFAULT\n");
            return 0;
        case EINVAL:
            printf("recv ENOMEM\n");
            return 0;
        case ENOTCONN:
            printf("recv ENOTCONN");
            return 0;
        case ENOTSOCK:
            printf("recv ENOTSOCK\n");
            return 0;
        default:
            return 0;
         }
}

int continueSend(void)
{
    switch (errno) {
        case EINTR:
        case EAGAIN:
            return 1;
        case EBADF:
            printf("EBADF\n");
            return 0;
        case EFAULT:
            printf("EFAULT\n");
            return 0;
        case EINVAL:
            printf("ENOMEM\n");
            return 0;
        case ENOTCONN:
            printf("ENOTCONN");
            return 0;
        case ENOTSOCK:
            printf("ENOTSOCK\n");
            return 0;
        default:
            return 0;
    }
}