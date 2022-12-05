#ifndef __COMMON_H
#define __COMMON_H

#include <stdlib.h>
#include <sys/socket.h>

typedef unsigned char byte;

byte* byteCopy(byte* dest,const byte* src,size_t num);

char* Byte_arrayToStr(byte *src, size_t n);

int retryConnect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

ssize_t retryRecv(int sockfd,void* buf,size_t len);

ssize_t retrySend(int sockfd,const void *buf,size_t len);

int createListeningSocket(short int port);

void forwardData(int srcSock, int dstSock, int encryption);

#endif
        
