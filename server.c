#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <memory.h>
#include <netdb.h>
#include "server.h"
#include "common.h"
#include "sock5.h"

int authSock5Connection(struct Config config,int clientSock)
{
    printf("[*] Authing identification by username && password. \n");

    byte buf[SOCK5_VALID_REQ_MAX_LEN] = {0};

    if (retryRecv(clientSock,buf,SOCK5_VALID_REQ_MAX_LEN) < 0)
    {
        printf("[-] Auth failed! \n");
        return -1;
    }

    // 读取客户端发来的用户名与密码
    int ok = 0;
    struct SOCK5_AUTH_REQ request = Sock5AuthReq_read(buf);

    if( strcmp((char *)request.uname,config.username) == 0  && strcmp((char *) request.passwd,config.password) == 0 && request.version == 0x01 )
    {
        printf("[*] Auth SUCCESS! \n");
        ok = 1;
    }
    else
    {
        if(strcmp((char *)request.uname,config.username) != 0)
        {
            printf("[-] Error: username. \n");
            printf("[+] Request: %s\n",(char *) request.uname);
            printf("[+] config: %s\n",config.username);
        }
        if(strcmp((char *)request.passwd ,config.password) != 0)
        {
            printf("[-] Error: password. \n");
        }
        if(request.version != 0x01)
        {
            printf("[-] Error: Version. \n");
        }
    }
    union SOCKS_REP response;
    response.auth_rep.version = 0x01;
    response.auth_rep.status = ok ? (byte)0x00 : (byte)0xFF;

    // 返回认证结果
    return (int)retrySend(clientSock, Sock5Response_toString(response,1), 2);
}




/// @brief sock5第一个报文的验证，根据客户端支持的验证方式选择服务端支持的
/// @param config 
/// @param clientSock 建立的第一个连接
/// @return 
int validateSock5Connection(struct Config config,int clientSock)
{

    // 验证客户端发来的用于是否为用来建立 sock5 数据
    printf("[*] Validate sock5 connection. \n");

    char buf[SOCK5_VALID_REQ_MAX_LEN];


    // 接收客户端用于表明建立 sock5 连接的第一个数据包
    /*
        
        | VER | NMETHODS | METHODS |
        | --- | -------- | ------- |
        |  1  |     1    | 1 to 255| 
    */

    // 此处对于方法实际只能指定一种，无法指定多种
    // 因为 retryRecv 只接受了 SOCK5_VALID_REQ_MAX_LEN 也就是 3 个字节的大小
    if (retryRecv(clientSock,buf,SOCK5_VALID_REQ_MAX_LEN) < 0)
    {
        return -1;
    }

    // 将接收到的报文按照 sock5 规定的第一个请求包的格式进行拆分
    // 将拆分出的各个字段放到 SOCK5_VALID_REQ 结构体的实例 request 对应的字段中
    struct SOCK5_VALID_REQ request = Sock5ValidReq_read((byte*)buf);

    // 验证 VER == 0X05 
    // 感觉此处该写法没必要
    // int ok = 1;
    // ok &= request.version == 0x05;
    // 是否可以改为：

    if (request.version != 0x05)
    {
        printf("[error] not sock5 data! \n");
        return -1;
    }


    union SOCKS_REP response;

    // 检查客户端请求的认证方式
    int i = 0;
    config.auth = UNREC_AUTH;
    for (i; i < request.method_num; i++)
    {
        // 目前仅支持这两种认证方式
        if (request.methods[i] == 0x02)
        {
            // 若有密码认证的方式,优先使用该方式
            config.auth = PASS_AUTH;
            printf("[*] Client_Req_Method: Username:Password. \n");
            break;
        }
        else if (request.methods[i] == 0x00)
        {
            config.auth = NO_AUTH;
            printf("[*] Client_Req_Method: NO_AUTH. \n");
        }
        // else
        // {
        //     response.methods = (byte)0xFF;
        //     return 
        // }

    }


    response.valid_rep.version = 0x05;

    if (config.auth == PASS_AUTH)
    {
        response.valid_rep.methods = (byte)0x02;
        retrySend(clientSock,Sock5Response_toString(response,0),2);

        // 之后进入密码验证阶段
        return (int)authSock5Connection(config,clientSock);

    }
    else if (config.auth == NO_AUTH)
    {
        response.valid_rep.methods = (byte)0x00;

        // 不进行密码验证，直接建立
        return (int)retrySend(clientSock,Sock5Response_toString(response,0),2);
    }
    else
    {
        response.valid_rep.methods = (byte)0xFF;
        // 建立出错
        printf("[-] Sock5 build failed!. \n");
        return (int)retrySend(clientSock,Sock5Response_toString(response,0),2);
    }


}


int createSock5Connection(struct Config config,int clientSock)
{

    printf("[*] create sock5 connection. \n");

    char buf[SOCK5_BUILD_REQ_MAX_LEN] = {0};

    if (retryRecv(clientSock,buf,SOCK5_BUILD_REQ_MAX_LEN) < 0)
    {
        return -1;
    }

    struct SOCK5_BUILD_REQ request = Sock5BuildRequest_read((byte *)buf);
    if (request.version < 0)
    {
        return -1;
    }
    if (request.cmd != 0x01) //目前支持CONNECT方式
    {
        return -1;
    }

    // 代理服务器创建到远程主机的连接
    int remoteSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (remoteSock < 0)
    {
        return -1;
    }

    int val = 1;
    if (setsockopt(remoteSock,SOL_SOCKET,SO_REUSEADDR,&val,sizeof(val)) < 0)
    {
        perror("setsockopt(). \n");
        printf("port reuse failed! \n");
    }

    // 响应给客户端的结构体
    union SOCKS_REP response;
    // struct SOCK5_BUILD_REP response;
    memset(&response.build_rep,0,sizeof(response.build_rep));

    struct sockaddr_in remote_addr;
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = request.dstport;

    if (request.atyp == 0x01)
    {
        in_addr_t ip;
        memcpy(&ip, request.dstaddr, 4);
        remote_addr.sin_addr.s_addr = ip;
    }
    else if (request.atyp == 0x03)
    {
        struct hostent *dsthost = gethostbyname(Byte_arrayToStr(request.dstaddr, request.addlen));
        remote_addr.sin_addr.s_addr = *(in_addr_t *)dsthost->h_addr;
    }
    else
    {
        printf("[Error] Address type not supported. \n");
        response.build_rep.rep = 0x08;
        // return -1;
    }

    if (retryConnect(remoteSock, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) < 0)
    {
        // 与远程目标建立连接失败
        // 统一返回错误代码 0x02
        printf("[Error] connection to ruleset failed \n");
        response.build_rep.rep = 0x02;
    }

    // 响应客户端

    response.build_rep.version = 0x05;
    response.build_rep.rsv = 0x00;
    response.build_rep.atyp = 0x01;
    response.build_rep.addrlen = 4;
    uint ip = (127U << 24) + 1;
    response.build_rep.bndaddr = (byte *)&ip;
    response.build_rep.bndport = (ushort)config.localPort;

    char* responseStr = Sock5Response_toString(response,-1);

    if (retrySend(clientSock, responseStr, Sock5BuildResponse_getLength(response)) < 0)
    {
        return -1;
    }

    return remoteSock;


}
void handleClientRequest(struct Config config,int clientSock)
{

    printf("[*] Handling with client socket. \n");
    if (validateSock5Connection(config,clientSock) < 0)
    {
        return;
    }

    int remoteSock = createSock5Connection(config, clientSock);
    if (remoteSock < 0)
    {
        return;
    }

    if (fork() == 0)
    {
        forwardData(clientSock, remoteSock, 0);
        exit(0);
    }
    if (fork() == 0)
    {
        forwardData(remoteSock, clientSock, 1);
        exit(0);
    }

}


void serverLoop(struct Config config,int server_socket)
{

    printf("[*] server start waiting. \n");

    struct sockaddr client_addr;
    socklen_t client_addr_len = sizeof(client_addr);


    printf("[*] waiting for client....\n");
    while (1)
    {

        int client_socket = accept(server_socket,(struct sockaddr *)&client_addr,&client_addr_len);
        if (client_socket < 0)
        {
            // continue // accept 本身为阻塞，而且此处没有使用 select 等多路复用 所以无需 continue
            // perror("accept()");
            // exit(-1);
            continue;
        }


        if (config.debugging)
        {
            printf("[+] Accepting a client scoket:%d\n",client_socket);
        }

        if (fork() == 0)
        {
            fflush(server_socket);
            close(server_socket);
            handleClientRequest(config,client_socket);
            exit(1);
        }

        fflush(client_socket);
        close(client_socket);


    }


}


void startServer(struct Config config)
{

    fprintf(stdout,"start server...\n");

    int server_socket = createListeningSocket(config.localPort);

    serverLoop(config,server_socket);

}
                                                                                                                                                                                                                                                                                                                                     
