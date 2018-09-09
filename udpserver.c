/*
 * ============================================================================
 *
 *       Filename:  udpserver.c
 *
 *    Description:  transfer video through network
 *
 *        Version:  1.0
 *        Created:  09/09/2018 11:03:42 AM
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

#include <sys/time.h>
#include <signal.h>

#include "camera.h"
#include "udpserver.h"

static int sockfd;

/* send YUV to network */
int send_picture(void)
{
    struct sockaddr_in cliaddr;
    int addrlen, nbyte;
    struct block netmsg;
    char buf[4096];

    if (sockfd < 0)     return -1;

    addrlen = sizeof(cliaddr);
    nbyte = recvfrom(sockfd, &netmsg, sizeof(netmsg), 0,
                     (struct sockaddr *)&cliaddr, &addrlen);

    printf("got connection from %s, port %d\n",
        inet_ntoa(cliaddr.sin_addr),htons(cliaddr.sin_port));

    if(0 == strncmp("request", netmsg.msg, 7)) {
        sendto(sockfd, buf, netmsg.data, 0,
               (struct sockaddr *)&cliaddr, addrlen);
    }
    return 0;
}

/* alarm */
void sigtimer_handler(int signum)
{
    printf("ALARM\n");
    send_picture();
}

/* initialize network UDP and alarm */
int init_udp(int timeval)
{
    int addrlen;
    struct sockaddr_in srvaddr;
    struct itimerval itimer;
    struct sigaction act;

    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        return -1;

    addrlen = sizeof(srvaddr);
    memset(&srvaddr, 0, addrlen);
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_port = htons(SERV_PORT);
    srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(sockfd, (struct sockaddr *)&srvaddr, addrlen) < 0)
        return -2;

    /* set alarm */
    itimer.it_interval.tv_sec = timeval;     /* period in seconds */
    itimer.it_interval.tv_usec = 0;
    itimer.it_value.tv_sec = timeval;
    itimer.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &itimer, NULL);

    act.sa_handler = sigtimer_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags=0;
    sigaction(SIGALRM, &act, NULL);

    return 0;
}
