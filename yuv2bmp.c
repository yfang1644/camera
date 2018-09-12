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

#pragma    pack(2)
struct FileHeader {
    char bfType[2];
    int bfSize;
    short bfReserved1;
    short bfReserved2;
    int bfOffBits;
};

struct ImageHeader {
    int biSize;
    int biWidth;
    int biHeight;
    short biPlanes;
    short biBitCount;
    int biCompression;
    int biSizeImage;
    int biXPelsPerMeter;
    int biYPelsPerMeter;
    int biClrUsed;
    int biClrImportant;
};

void bmphead(int width, int height, int pixelbits, int fd)
{
    unsigned char *p;
    unsigned int i, pallete[256];

    struct header {
        struct FileHeader fh;
        struct ImageHeader ih;
    } mybmp;

    mybmp.fh.bfType[0] = 'B';
    mybmp.fh.bfType[1] = 'M';
    mybmp.fh.bfSize = 0;
    mybmp.fh.bfReserved1 = 0;
    mybmp.fh.bfReserved2 = 0;
    mybmp.fh.bfOffBits = sizeof(struct ImageHeader)
                       + sizeof(struct FileHeader)
                       + sizeof(pallete);

    mybmp.ih.biSize = sizeof(struct ImageHeader);
    mybmp.ih.biWidth = width;
    mybmp.ih.biHeight = height;
    mybmp.ih.biPlanes = 1;
    mybmp.ih.biBitCount = 8;
    mybmp.ih.biCompression = 0;
    mybmp.ih.biSizeImage = width * height * pixelbits/8;
    mybmp.ih.biXPelsPerMeter = 2835;
    mybmp.ih.biYPelsPerMeter = 2835;
    mybmp.ih.biClrUsed = 0;
    mybmp.ih.biClrImportant = 0;

    mybmp.fh.bfSize = mybmp.fh.bfOffBits + mybmp.ih.biSizeImage;

    for (i = 0; i < 256; i++) {
        pallete[i] = (i << 16) | (i << 8) | i;
    }

    /* TODO:  needs pading for structure alignment  */

    write(fd, &mybmp, sizeof(mybmp));
    write(fd, pallete, sizeof(pallete));
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
    unsigned char color[720*2];
    unsigned char p[720*4];

    for(y = 0; y < height; y++) {
        read(rfile, p, width*2);
        in = p;
        for (x = 0; x < width; x += 2) {
            y0 = *in++;
            u  = (*in++) - 128;
            y1 = *in++;
            v  = (*in++) - 128;

            /* for grayscale, r=g=b */
            r = y0 + ((351 * v) >> 8);
            r = clip(r, 0, 255);
            color[x] = r;

            r = y1 + ((351 * v) >> 8);
            r = clip(r, 0, 255);
            color[x+1] = r;
        }
        write(wfile, color, width);
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

    bmphead(720, 480, 8, fd2);
    lseek(fd1, 128, SEEK_SET);   /* skip file head */

    yuv2rgb(fd1, fd2);
    close(fd1);
    close(fd2);

    return 0;
}
