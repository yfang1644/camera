/*
 * ============================================================================
 *
 *       Filename:  camera.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  12/14/2016 03:04:12 PM
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
#include <getopt.h>  
#include <fcntl.h>  
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <linux/fb.h>
#include <assert.h>
#define CLEAR(x) memset (&(x), 0, sizeof (x))

#include "font.h"

struct buffer {
    void * start;
    size_t length;
};

static int fd = -1;
struct buffer * buffers = NULL;
static unsigned int n_buffers = 0;
static int time_in_sec_capture=5;
static int fbfd = -1;
static struct fb_var_screeninfo vinfo;
static struct fb_fix_screeninfo finfo;
static short *fbp=NULL;
static long screensize=0;
 
static void errno_exit(const char * s)
{
    fprintf (stderr, "%s error %d, %s/n",s, errno, strerror (errno));
    exit (EXIT_FAILURE);
}
 
int xioctl(int fd,int request,void * arg)
{
    int r;
    do {
        r = ioctl(fd, request, arg);
    } while (-1 == r && EINTR == errno);
    return r;
}
 
static inline int clip(int value, int min, int max) {
    return (value > max ? max : value < min ? min : value);
}

void pixel(int x, int y, short color)
{
    long location;
    if((x < vinfo.xres) && (y < vinfo.yres)) {
        location = y*vinfo.xres + x;
        *(short *)(fbp + location) = color;
    }
}

void put_char(int x, int y, int c, unsigned short colidx)
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

void put_string(int x, int y, char *s, unsigned short colidx)
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

void process_image(const void * p)
{
    //Convert YUV To RGB565
    unsigned char* in=(char*)p;
    int width=320;
    int height=240;
    int istride=640;
    unsigned int x,y,j;
    int y0,u,y1,v;
    unsigned int r,g,b;
    unsigned short color;
    time_t ticks;
    char timebuf[256];
 
    for(y = 0; y < height; ++y) {
        for (j = 0, x = 0; j < width * 2 ; j += 4,x += 2) {
            y0 = in[j];
            u = in[j + 1] - 128;                
            y1 = in[j + 2];        
            v = in[j + 3] - 128;        

            r = y0 + ((351 * v) >> 8);
            g = y0 - ((179 * v + 86 * u) >> 8);
            b = y0 + ((444 * u) >> 8);

            r >>= 3; r = clip(r, 0, 31);
            g >>= 2; g = clip(g, 0, 63);
            b >>= 3; b = clip(b, 0, 31);

            color = (r << 11) | (g << 5) | b;
            pixel(x, y, color);
            r = y1 + ((351 * v) >> 8);
            g = y1 - ((179 * v + 86 * u) >> 8);
            b = y1 + ((444 * u) >> 8);

            r >>= 3; r = clip(r, 0, 31);
            g >>= 2; g = clip(g, 0, 63);
            b >>= 3; b = clip(b, 0, 31);

            color = (r << 11) | (g << 5) | b;
            pixel(x+1, y, color);
        }
        in +=istride;
    }
    ticks = time(NULL);
    sprintf(timebuf, "%.24s", ctime(&ticks));
    put_string(10, 250, timebuf, 0xff00);
}
 
int read_frame(void)
{
    struct v4l2_buffer buf;
    unsigned int i;

//    CLEAR (buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
 
    if(-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
        switch (errno) {
        case EAGAIN:
            return 0;
        case EIO:    
 
        default:
            errno_exit("VIDIOC_DQBUF");
        }
    }
 
    assert(buf.index < n_buffers);
    //printf("v4l2_pix_format->field(%d)\n", buf.field);
    //assert (buf.field ==V4L2_FIELD_NONE);
    process_image(buffers[buf.index].start);

    if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
        errno_exit("VIDIOC_QBUF");
 
    return 1;
}
 
void run(void)
{
    fd_set fds;
    struct timeval tv;
    int r;

    while (1) {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        tv.tv_sec = 2;
        tv.tv_usec = 0;

        r = select(fd + 1, &fds, NULL, NULL, &tv);

        if(-1 == r) {
            if (EINTR == errno)
            continue;
            errno_exit("select");
        }

        if(0 == r) {
            fprintf(stderr, "select timeout\n");
            exit(EXIT_FAILURE);
        }

        read_frame();
    }
}

static void stop_capturing(void)
{
    enum v4l2_buf_type type;
 
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(-1 == xioctl (fd, VIDIOC_STREAMOFF, &type))
        errno_exit("VIDIOC_STREAMOFF");
}
 
static void start_capturing(void)
{
    unsigned int i;
    enum v4l2_buf_type type;
 
    for(i = 0; i < n_buffers; ++i) {
        struct v4l2_buffer buf;
        CLEAR (buf);
 
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
 
        if(-1 == xioctl(fd, VIDIOC_QBUF, &buf))
            errno_exit ("VIDIOC_QBUF");
    }
 
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 
    if(-1 == xioctl(fd, VIDIOC_STREAMON, &type))
        errno_exit ("VIDIOC_STREAMON");
}
 
static void uninit_device (void)
{
    unsigned int i;
 
    for(i = 0; i < n_buffers; ++i)
        if (-1 == munmap (buffers[i].start, buffers[i].length))
            errno_exit("munmap");

    if(-1 == munmap(fbp, screensize)) {
        printf(" Error: framebuffer device munmap() failed.\n");
        exit(EXIT_FAILURE) ;
    }    
    free(buffers);
}
 
 
static void init_mmap(char *dev_name)
{
    struct v4l2_requestbuffers req;
 
    //mmap framebuffer
    fbp = (short *)mmap(NULL,screensize, PROT_READ | PROT_WRITE,
        MAP_SHARED, fbfd, 0);
    if(fbp == NULL) {
        printf("Error: failed to map framebuffer device to memory.\n");
        exit (EXIT_FAILURE) ;
    }
    memset(fbp, 0, screensize);
    CLEAR(req);
 
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
 
    if (-1 == xioctl (fd, VIDIOC_REQBUFS, &req)) {
        if(EINVAL == errno) {
            fprintf(stderr, "%s does not support memory mapping\n", dev_name);
            exit(EXIT_FAILURE);
        } else {
            errno_exit("VIDIOC_REQBUFS");
        }
    }
 
    if(req.count < 4) {    //if (req.count < 2)
        fprintf(stderr, "Insufficient buffer memory on %s\n",dev_name);
        exit(EXIT_FAILURE);
    }
 
    buffers = calloc(req.count, sizeof (*buffers));
 
    if(!buffers) {
        fprintf (stderr, "Out of memory\n");
        exit (EXIT_FAILURE);
    }
 
    for(n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        struct v4l2_buffer buf;

        CLEAR (buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;
 
        if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
            errno_exit("VIDIOC_QUERYBUF");

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = mmap (NULL,buf.length,PROT_READ | PROT_WRITE ,MAP_SHARED,fd, buf.m.offset);
 
        if (MAP_FAILED == buffers[n_buffers].start)
            errno_exit("mmap");
    }
}
 
static void init_device(char *dev_name)
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
 
    // Get fixed screen information
    if (-1==xioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
        printf("Error reading fixed information.\n");
        exit (EXIT_FAILURE);
    }
 
        // Get variable screen information
    if (-1==xioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
        printf("Error reading variable information.\n");
        exit (EXIT_FAILURE);
    }
    printf("FB-format %d %d %d\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
 
    if (-1 == xioctl (fd, VIDIOC_QUERYCAP, &cap)) {
        if (EINVAL == errno) {
            fprintf (stderr, "%s is no V4L2 device\n",dev_name);
            exit(EXIT_FAILURE);
        } else {
            errno_exit("VIDIOC_QUERYCAP");
        }
    }
 
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf (stderr, "%s is no video capture device\n",dev_name);
        exit(EXIT_FAILURE);
    }
 
    if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
        fprintf(stderr, "%s does not support streaming i/o\n",dev_name);
        exit(EXIT_FAILURE);
    }
 
    CLEAR(cropcap);
 
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 
    if(0 == xioctl (fd, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect;
 
        if(-1 == xioctl (fd, VIDIOC_S_CROP, &crop)) {
            switch (errno) {
            case EINVAL:    
            break;
            default:
            break;
            }
        }
    }
 
    CLEAR (fmt);
 
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(-1 == xioctl(fd,VIDIOC_G_FMT,&fmt))
       errno_exit("VIDIOC_G_FMT");
   
    printf("width:%d  height:%d\n",fmt.fmt.pix.width,fmt.fmt.pix.height);

    printf("pixelformat = %c%c%c%c\n",
           fmt.fmt.pix.pixelformat & 0xFF,
           (fmt.fmt.pix.pixelformat >> 8) & 0xFF,
           (fmt.fmt.pix.pixelformat >> 16) & 0xFF,
           (fmt.fmt.pix.pixelformat >> 24) & 0xFF);

    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
        errno_exit("VIDIOC_S_FMT");
 
    init_mmap(dev_name);
}
 
static void close_device(void)
{
    if (close(fd) < 0)
        errno_exit("close");
    fd = -1;
    close(fbfd);
}
 
static void open_device(char *dev_name)
{
    struct stat st;  
 
    if(-1 == stat (dev_name, &st)) {
        fprintf(stderr, "Cannot identify '%s': %d, %s\n",dev_name, errno, strerror (errno));
        exit(EXIT_FAILURE);
    }
 
    if (!S_ISCHR (st.st_mode)) {
        fprintf(stderr, "%s is no device\n", dev_name);
        exit(EXIT_FAILURE);
    }
 
    //open framebuffer
    fbfd = open("/dev/fb0", O_RDWR);
    if(fbfd < 0) {
        printf("Error: cannot open framebuffer device.\n");
        exit(EXIT_FAILURE);
    }
 
    //open camera
    fd = open(dev_name, O_RDWR| O_NONBLOCK, 0);
 
    if(-1 == fd) {
        fprintf(stderr, "Cannot open '%s': %d, %s/n",dev_name, errno, strerror (errno));
        exit(EXIT_FAILURE);
    }
}
 
static void usage(FILE * fp,int argc,char ** argv)
{
    fprintf(fp,
            "Usage: %s [options]\n\n"
            "Options:\n"
            "-d | --device name Video device name [/dev/video]\n"
            "-h | --help Print this message\n"
            "-t | --how long will display in seconds\n"
            "",
            argv[0]);
}
 
static const char short_options [] = "d:ht:";
static const struct option long_options [] = {
    { "device", required_argument, NULL, 'd' },
    { "help", no_argument, NULL, 'h' },
    { "time", no_argument, NULL, 't' },
    { 0, 0, 0, 0 }
};
 
int main(int argc,char ** argv)
{
    char *dev_name = "/dev/video1";
 
    for(;;) {
        int index;
        int c;

        c = getopt_long (argc, argv,short_options, long_options,&index);
        if(-1 == c)
            break;
 
        switch(c) {
        case 0:
        break;
 
        case 'd':
            dev_name = optarg;
        break;
 
        case 'h':
            usage(stdout, argc, argv);
            exit(EXIT_SUCCESS);
        case 't':
            time_in_sec_capture = atoi(optarg);
            break;
 
        default:
            usage(stderr, argc, argv);
            exit(EXIT_FAILURE);
        }
    }
 
    open_device(dev_name);
    init_device(dev_name);
    start_capturing();
    run();
    stop_capturing();
    uninit_device();
    close_device();
    exit(EXIT_SUCCESS);
}
