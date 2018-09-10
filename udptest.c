/*
 * ============================================================================
 *
 *       Filename:  udptest.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  09/09/2018 02:15:02 PM
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

#include "udpserver.h"

/* 
* ===  FUNCTION  =============================================================
*         Name:  main
*  Description:  
* ============================================================================
*/
int main(int argc, char *argv[])
{
    init_udp(2);
    while(1)
        sleep(1000);

    return EXIT_SUCCESS;
}
