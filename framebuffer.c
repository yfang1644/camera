/*
 * ============================================================================
 *
 *       Filename:  framebuffer.c
 *
 *    Description:  Frame Buffer
 *
 *        Version:  1.0
 *        Created:  09/09/2018 01:18:01 PM
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
#include <string.h>
#include <fcntl.h>  
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>

#include "camera.h"
#include "font.h"

static int fd = -1;
static int fbfd = -1;
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;
static unsigned char *fbp=NULL;
static long screensize=0;
 
void pixel(int x, int y, RGBCOLOR color)
{
    unsigned int ptr;
    if((x < vinfo.xres) && (y < vinfo.yres)) {
        ptr = y*finfo.line_length + x*sizeof(RGBCOLOR);
        *(RGBCOLOR *)(fbp + ptr) = color;
    }
}

void put_char(int x, int y, int c, RGBCOLOR colidx)
{
    int i, j, bits;

    for(i = 0; i < font_vga_8x16.height; i++) {
        bits = font_vga_8x16.data [font_vga_8x16.height * c + i];
        for (j = 0; j < font_vga_8x16.width; j++, bits <<= 1)
            if (bits & 0x80)   // TODO: 0x80 should replaced by 2^width
                pixel(x + j, y + i, colidx);
            else
                pixel(x + j, y + i, 0);
    }
}

void put_string(int x, int y, char *s, RGBCOLOR colidx)
{
    static char oldstr[256];
    int i = 0;

    do {
        if(oldstr[i] != s[i])
            put_char(x, y, s[i], colidx);
        oldstr[i] = s[i];
        x += font_vga_8x16.width;
        i++;
    } while (s[i] != '\0');
}

void init_framebuffer(char *dev_name)
{
    //open framebuffer
    fbfd = open(dev_name, O_RDWR);
    if(fbfd < 0) {
        printf("Error: cannot open framebuffer device.\n");
        exit(EXIT_FAILURE);
    }
 
    /* Get fixed screen information */
    if (-1 == ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
        printf("Error reading fixed information.\n");
        exit (EXIT_FAILURE);
    }
 
    /* Get variable screen information */
    if (-1 == ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
        printf("Error reading variable information.\n");
        exit (EXIT_FAILURE);
    }
    printf("FB-format %d %d %d\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
 
    //mmap framebuffer
    fbp = (unsigned char *)mmap(NULL, screensize, PROT_READ | PROT_WRITE,
        MAP_SHARED, fbfd, 0);
    if(fbp == NULL) {
        printf("Error: failed to map framebuffer device to memory.\n");
        exit (EXIT_FAILURE) ;
    }
    memset(fbp, 0, screensize);
}
 
void close_framebuffer(int fd)
{
    free(fbp);
    close(fd);
}
