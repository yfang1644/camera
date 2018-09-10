/*
 * ============================================================================
 *
 *       Filename:  yuv2bmp.c
 *
 *    Description:  convert yuv422 to 32bit grayscale
 *
 *        Version:  1.0
 *        Created:  09/10/2018 01:54:36 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Fang Yuan (yfang@nju.edu.cn)
 *   Organization:  nju
 *
 * ============================================================================
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

void bmphead(int width, int height, int pixelbits, int fd)
{
    unsigned char *p;
#pragma    pack(2)
    struct header {
        char bmp[2];
        int fsize;
        short reserved1;
        short reserved2;
        int offset_data;
        int headsize;
        int pwidth;
        int pheight;
        short planes;
        short bpp;
        int compression;
        int bmpsize;
        int hres;
        int vres;
        int ncolors;
        int importantcolor;
    };

    struct header mybmp;
    
    mybmp.bmp[0] = 'B'; mybmp.bmp[1] = 'M';
    mybmp.reserved1 = 0;
    mybmp.reserved2 = 0;
    mybmp.offset_data = sizeof(mybmp);
    mybmp.headsize = 40;
    mybmp.pwidth = width;
    mybmp.pheight = height;
    mybmp.planes = 1;
    mybmp.bpp = pixelbits;
    mybmp.compression = 0;
    mybmp.bmpsize = width * height * pixelbits/8;
    mybmp.hres = 2834;
    mybmp.vres = 2834;
    mybmp.ncolors = 0;
    mybmp.importantcolor = 0;

    mybmp.fsize = mybmp.offset_data+ mybmp.bmpsize;

    write(fd, &mybmp, sizeof(mybmp));
}

static inline int clip(int value, int min, int max) {
    return (value > max ? max : value < min ? min : value);
}

void yuv2rgb(int rfile, int wfile)
{
    //Convert YUV To RGBA888
    unsigned char *in;
    int width=720;
    int height=480;

    unsigned int x,y;
    int y0,u,y1,v;
    unsigned int r,g,b;
    unsigned int color[720*2];
    unsigned char p[720*4];

    for(y = 0; y < height; y++) {
        read(rfile, p, width*2);
        in = p;
        for (x = 0; x < width; x += 2) {
            y0 = *in++;
            u  = (*in++) - 128;
            y1 = *in++;
            v  = (*in++) - 128;

            r = y0 + ((351 * v) >> 8);
            r = clip(r, 0, 255);
            color[x] = (r << 16) | (r << 8) | r;

            r = y1 + ((351 * v) >> 8);
            r = clip(r, 0, 255);
            color[x+1] = (r << 16) | (r << 8) | r;
        }
        write(wfile, color, 4*width);
    }
}

/* 
* ===  FUNCTION  =============================================================
*         Name:  main
*  Description:  
* ============================================================================
*/
int main (int argc, char *argv[])
{
    int fd1, fd2;

    if(3 != argc) {
        printf("needs 2 filenames in command line\n");
        return -1;
    }

    fd1 = open(argv[1], O_RDONLY);
    fd2 = open(argv[2], O_RDWR|O_CREAT, 0644);

    bmphead(720, 480, 32, fd2);
    lseek(fd1, 128, SEEK_SET);   /* skip file head */

    yuv2rgb(fd1, fd2);
    close(fd1);
    close(fd2);

    return 0;
}
