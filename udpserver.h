/*
 * ============================================================================
 *
 *       Filename:  udpserver.h
 *
 *    Description:  network transferring
 *
 *        Version:  1.0
 *        Created:  09/09/2018 12:23:01 PM
 *       Revision:  none
 *       Compiler: 
 *
 *         Author:  Fang Yuan (yfang@nju.edu.cn)
 *   Organization:  nju
 *
 * ============================================================================
 */

#ifndef _UDPSERVER_H
#define _UDPSERVER_H

#define SERV_PORT	9200

struct block {
	char msg[128];
	int data;
};

#endif
