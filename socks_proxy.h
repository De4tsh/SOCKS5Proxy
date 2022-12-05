#ifndef __CONFIG_SOCKS_
#define __CONFIG_SOCKS_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

// 采用的认证方式
enum auth_method
{
    NO_AUTH,
    PASS_AUTH,
    UNREC_AUTH
};

struct Config
{
    short int localPort;
    int client;
    int server;
    char *serverHost;
    short int serverPort;
    char* username;
    char* password;
    int debugging;
    enum auth_method auth;
};

#endif
