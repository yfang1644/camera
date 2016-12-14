/*
 * ============================================================================
 *
 *       Filename:  font.h
 *
 *    Description:  font.h copy from tslib-1.0.0
 *
 *        Version:  1.0
 *        Created:  12/14/2016 02:54:01 PM
 *       Revision:  none
 *       Compiler: 
 *
 *         Author:  Fang Yuan (yfang@nju.edu.cn)
 *   Organization:  nju
 *
 * ============================================================================
 */

#ifndef _FONT_H
#define _FONT_H

struct fbcon_font_desc {
    int idx;
    char *name;
    int width, height;
    char *data;
    int pref;
};

#define VGA8x8_IDX	0
#define VGA8x16_IDX	1
#define PEARL8x8_IDX	2
#define VGA6x11_IDX	3
#define SUN8x16_IDX	4
#define SUN12x22_IDX	5
#define ACORN8x8_IDX	6

extern struct fbcon_font_desc	font_vga_8x8,
			font_vga_8x16;

/* Max. length for the name of a predefined font */
#define MAX_FONT_NAME	32

#endif
