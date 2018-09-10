/*
 * ============================================================================
 *
 *       Filename:  camera.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  09/09/2018 12:21:00 PM
 *       Revision:  none
 *       Compiler: 
 *
 *         Author:  Fang Yuan (yfang@nju.edu.cn)
 *   Organization:  nju
 *
 * ============================================================================
 */

#ifndef _CAMERA_H
#define _CAMERA_H

#define CLEAR(x)    memset(&(x), 0, sizeof(x))

struct buffer {
    void * start;
    size_t length;
};

#endif
