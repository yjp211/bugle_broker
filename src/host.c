#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

#include "include.h"

extern char g_host_flag[1024];
extern struct config_t g_config;

static int fill_by_host_name()
{
    if(gethostname(g_host_flag, sizeof(g_host_flag)) != 0){
        fprintf(stderr, "get local host name faile, %m\n");
        return ERR_SYS_FAILED;
    }
    return SUCCESS;
}


int fill_host_flag(){
    bzero(g_host_flag, 1024);
    if(strlen(g_config.fill_host_by) == 0 ){
        fill_by_host_name();
        return SUCCESS;
    }
    
    if(strcmp(g_config.fill_host_by, "0") == 0){
        strcat(g_host_flag, "127.0.0.1");
        return SUCCESS;
    }

    struct ifaddrs * ifAddrStruct=NULL;
    void * tmpAddrPtr=NULL;

    if(getifaddrs(&ifAddrStruct) != 0){
        fprintf(stderr, "get interface addrs  faile, %m\n");
        return ERR_SYS_FAILED;
    }

    while (ifAddrStruct!=NULL) {
        if(strcmp(ifAddrStruct->ifa_name, g_config.fill_host_by) != 0){
            ifAddrStruct=ifAddrStruct->ifa_next;
            continue;
        }
        if (ifAddrStruct->ifa_addr->sa_family !=AF_INET) { // check it is IP4
            ifAddrStruct=ifAddrStruct->ifa_next;
            continue;
        }
                // is a valid IP4 Address
        tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
        char addressBuffer[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
        
        strcat(g_host_flag, addressBuffer);

        return SUCCESS;
    }

    fprintf(stderr, "no address on interface: %s\n", g_config.fill_host_by);
        //not found
    return ERR_SYS_FAILED;
}

