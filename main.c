#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include "client.h"
#include "server.h"
#include "socks_proxy.h"


void options_init(struct Config* config) 
{
    config.client = 0;
    config.server = 0;
    config.debugging = 0;
}


int main(int argc,char **argv)
{

    struct Config config;
    
    int opt;

    if ( argc < 2)
    {
        fprintf(stderr,"Usage:\nClient: -P [local port] -c -h [remote ip] -p [remote port]\nServer: -P [local port] -s -u [username] -w [password]\n-d [debugging]");
        exit(-1);
    }

    options_init(config);

    while ((opt = getopt(argc,argv,"P:csh:u:w:p:d")) != EOF)
    {

        switch (opt)
        {

            case 'P': config.localPort = atoi(optarg); break;

            case 'c': config.client++; break;

            case 's': config.server++; break;

            case 'h': config.serverHost = optarg; break;

            case 'p': config.serverPort = atoi(optarg); break;

            case 'u': config.username = optarg; break;

            case 'w': config.passwrd = optarg; break;

            case 'd': config.debugging++;
            
        }
    }

    if (config.client && config.server)
    {
        fprintf(stderr,"[Error] You must to figure out server or client!");
        exit(-1);
    }

    signal(SIGCHLD,SIG_IGN);
    signal(SIGPIPE,SIG_IGN);


    if (config.client)
    {
        startClient(config);
    }
    else if (config.server)
    {
        if (config.username != NULL && config.password != NULL)
        {
                fprintf(stdout,"[*] Username:%s\n[*] Password:%s. \n",config.username,config.password);
        }
        fprintf(stdout,"[*] no_authentication_mode");

        startServer(config);
    }

    printf("Finish\n");

    return 0;


}
