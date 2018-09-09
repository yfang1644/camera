/*
 * ============================================================================
 *
 *       Filename:  framebuffer.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  09/09/2018 01:33:15 PM
 *       Revision:  none
 *       Compiler: 
 *
 *         Author:  Fang Yuan (yfang@nju.edu.cn)
 *   Organization:  nju
 *
 * ============================================================================
 */

#ifndef _FRAMEBUFFER_H
#define _FRAMEBUFFER_H

typedef unsigned int   RGBCOLOR;

void init_framebuffer(char *dev_name);
void close_framebuffer(int fd);

void pixel(int x, int y, RGBCOLOR color);
void put_string(int x, int y, char *s, RGBCOLOR colidx);
#endif
