/*
 * ============================================================================
 *
 *       Filename:  udpget.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  09/09/2018 02:44:46 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Fang Yuan (yfang@nju.edu.cn)
 *   Organization:  nju
 *
 * ============================================================================
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "udpserver.h"

/* 
* ===  FUNCTION  =============================================================
*         Name:  main
*  Description:  
* ============================================================================
*/

int main(int argc, char *argv[])
{
    int sockfd, nbytes, port, addrlen ;
    char buf[1024];
    struct sockaddr_in srvaddr ;
    struct block netmsg;

    if(argc != 3) {
        printf("usage : %s hostname %d\n", argv[0], SERV_PORT);
        return -1;
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    port = atoi(argv[2]);

    memset(&srvaddr, 0, sizeof(srvaddr));
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_port = htons(port);
    srvaddr.sin_addr.s_addr = inet_addr(argv[1]);

    addrlen = sizeof(srvaddr);
    strncpy(netmsg.msg, "request", 7);
    netmsg.data = 1024;

    sendto(sockfd, (void *)&netmsg, sizeof(netmsg), 0, 
           (struct sockaddr *)&srvaddr, addrlen);

    nbytes = recvfrom(sockfd, buf, 1024, 0,
                      (struct sockaddr *)&srvaddr, &addrlen);

    printf("%d bytes received.\n", nbytes);
    close(sockfd);
}
