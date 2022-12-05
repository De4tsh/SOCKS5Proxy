
#include <stdio.h>
#include <sys/socket.h>
#include <memory.h>
#include <netinet/in.h>
#include "common.h"
#include "exception.h"

// 尝试接收/发送的最大次数
#define RETRY_TIME 10



byte* byteCopy(byte* dest,const byte* src,size_t num)
{

    memset(dest,0,num);
    memcpy(dest,src,num);

    return dest;
}


char* Byte_arrayToStr(byte *src, size_t n)
{
    char* result = (char *)malloc(n + 1);
    memcpy(result, src, n);
    result[n] = '\0';
    return result;
}


/// @brief 代替客户端与要连接的目标建立连接
/// @param sockfd 用于与对端建立连接的 sockfd
/// @param addr 对端 IP
/// @param addrlen IP 长度
/// @return 
int retryConnect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    for (int i = 0; i < RETRY_TIME; i++)
    {
        int result = connect(sockfd, addr, addrlen);
        if (result >= 0)
        {
            return result;
        }
    }
    return -1;
}




// 接收发送来的数据
ssize_t retryRecv(int sockfd,void* buf,size_t len)
{
    int i = 0;
    for (i = 0; i < RETRY_TIME; i++)
    {
        ssize_t result = recv(sockfd,buf,len,0);
        if (result > 0)
        {
            return result;
        }
        else if ( result < 0 )
        {
            perror("[error] recv. \n");
            return -1;
        }

        printf("[-] no data recving ,recv again [%d/10]. \n",i);
    }
    printf("[-] no recv data. [10/10]. \n");

    return 0;

}

ssize_t retrySend(int sockfd,const void *buf,size_t len)
{
    int i = 0;
    for (i; i < RETRY_TIME; i++)
    {
        ssize_t result = send(sockfd,buf,len,0);
        if (result >= 0)
        {
            printf("[*] successed to send %d bits data. \n",result);
            return result;
        }
        else
        {
            perror("[error] send. \n");
            return -1;
        }

        // printf("[-] send data failed ,send again [%d/10]. \n",i);
    }

    // printf("[-] send data failed. [10/10]\n");
    return -1;
}


int createListeningSocket(short int port)
{

    int sfd = socket(AF_INET,SOCK_STREAM,0);
    if (sfd < 0)
    {
        perror("socket()");
        return SERVER_SOCKET_CREATE_ERROR;
    }

    // 设置端口复用
    int val = 1;
    if (setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&val,sizeof(val)) < 0)
    {
        perror("setsockopt(). \n");
        printf("port reuse failed! \n");
    }

    struct sockaddr_in server_addr;
    memset(&server_addr,0,sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 直接给 0 也可


    printf("check1\n");

    if (bind(sfd,(struct sockaddr *)&server_addr,sizeof(server_addr)) < 0)
    {

        perror("bind()");
        // if (config)
        printf("bind [port]:%d failed. \n",port);
        return SERVER_SOCKET_BIND_ERROR;
    }

    printf("check2\n");

    return sfd;

}

void forwardData(int srcSock, int dstSock, int encryption)
{
    char buf[8192];
    ssize_t n;
    while ((n = retryRecv(srcSock, buf, 8000)) > 0) {
        if (retrySend(dstSock, buf, (size_t)n) < 0) {
            break;
        }
    }
    shutdown(srcSock, SHUT_RDWR);
    shutdown(dstSock, SHUT_RDWR);
}                                             
