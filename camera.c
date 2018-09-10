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
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <assert.h>

#include "camera.h"
#include "framebuffer.h"
#include "font.h"
#include "udpserver.h"

static int fd = -1;
struct buffer *buffers = NULL;
int gindex = -1;
static unsigned int n_buffers = 0;
 
static void errno_exit(const char * s)
{
    fprintf (stderr, "%s error %d, %s/n",s, errno, strerror (errno));
    exit (EXIT_FAILURE);
}
 
int xioctl(int fd, int request, void * arg)
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

int save_image(const void *p)
{
    int width = 720;
    int height = 480;
    int fsize;
    struct tm now;
    time_t ticks;
    char timebuf[256];
    int fdimg;

    ticks = time(NULL);
    localtime_r(&ticks, &now);
    fsize = width * height * 4/2;

    /* save file to /mnt/usb1 */
    sprintf(timebuf, "/mnt/usb1/%04d-%02d-%02d-%02d-%02d-%02d.yuv",
            now.tm_year+1900, now.tm_mon+1, now.tm_mday,
            now.tm_hour, now.tm_min, now.tm_sec);

    fdimg = open(timebuf, O_RDWR|O_CREAT, 0644);
    if (fdimg < 0)
        return -1;

    write(fdimg, timebuf, 128);
    write(fdimg, p, fsize);
    close(fdimg);

    return 0;
}

int process_image(const void *p)
{
    //Convert YUV To RGB565
    unsigned char* in=(char*)p;
    int width = 720;
    int height = 480;
    unsigned int x, y;
    int y0, u, y1, v;
    unsigned int r, g, b;
    RGBCOLOR color;
    time_t ticks;
    char timebuf[256];

    for(y = 0; y < height; y++) {
        for (x = 0; x < width; x += 2) {
            y0 = *in++;
            u  = (*in++) - 128;
            y1 = *in++;
            v  = (*in++) - 128;

            r = y0 + ((351 * v) >> 8);

            r = clip(r, 0, 255);

            color = (r << 16)| (r << 8) | r;  /*gray, r=g=b */
            pixel(x, y, color);
            r = y1 + ((351 * v) >> 8);

            r = clip(r, 0, 255);

            color = (r << 16)| (r << 8) | r;
            pixel(x+1, y, color);
        }
    }

    ticks = time(NULL);
    sprintf(timebuf, "%.24s", ctime(&ticks));
    put_string(10, height - 20, timebuf, 0x00ff00);

    return 0;
}
 
int read_frame(void)
{
    struct v4l2_buffer buf;
    static int count = 0;

//  CLEAR(buf);
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

    gindex = buf.index;
    process_image(buffers[buf.index].start);

    if (0 == (count % 30)) {
        save_image(buffers[buf.index].start);
    }
    count++;

    if (-1 == xioctl (fd, VIDIOC_QBUF, &buf))
        errno_exit("VIDIOC_QBUF");
 
    return 1;
}
 
void select_run(int itimeval)
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
        sleep(itimeval);
    }
}

void capture(int signum)
{
    read_frame();
}

void sleep_run(int itimeval)
{
    struct itimerval itimer;
    struct sigaction act;

    /* set alarm */
    itimer.it_interval.tv_sec = itimeval;     /* period in seconds */
    itimer.it_interval.tv_usec = 0;
    itimer.it_value.tv_sec = itimeval;
    itimer.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &itimer, NULL);

    act.sa_handler = capture;
    sigemptyset(&act.sa_mask);
    act.sa_flags=0;
    sigaction(SIGALRM, &act, NULL);

    while(1) {
        sleep(600);
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
        CLEAR(buf);
 
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
 
static void close_camera(void)
{
    unsigned int i;
 
    for(i = 0; i < n_buffers; ++i)
        if (-1 == munmap (buffers[i].start, buffers[i].length))
            errno_exit("munmap");

    free(buffers);

    if (close(fd) < 0)
        errno_exit("close");
    fd = -1;
}
 
void open_camera(char *dev_name)
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
 
    //open camera
    fd = open(dev_name, O_RDWR| O_NONBLOCK, 0);
 
    if(-1 == fd) {
        fprintf(stderr, "Cannot open '%s': %d, %s/n",dev_name, errno, strerror (errno));
        exit(EXIT_FAILURE);
    }
}

static void camera_format(char *dev_name)
{
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
 
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
 
    if(0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect;
 
        if(-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
            switch (errno) {
            case EINVAL:    
            break;
            default:
            break;
            }
        }
    }
 
    CLEAR(fmt);
 
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(-1 == xioctl(fd, VIDIOC_G_FMT, &fmt))
       errno_exit("VIDIOC_G_FMT");
   
    printf("width:%d  height:%d\n",fmt.fmt.pix.width, fmt.fmt.pix.height);

    printf("pixelformat = %c%c%c%c\n",
           fmt.fmt.pix.pixelformat & 0xFF,
           (fmt.fmt.pix.pixelformat >> 8) & 0xFF,
           (fmt.fmt.pix.pixelformat >> 16) & 0xFF,
           (fmt.fmt.pix.pixelformat >> 24) & 0xFF);

    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
        errno_exit("VIDIOC_S_FMT");
}
 
static void camera_mmap(char *dev_name)
{
    struct v4l2_requestbuffers req;
 
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

        CLEAR(buf);

        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;
 
        if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
            errno_exit("VIDIOC_QUERYBUF");

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = mmap(NULL, buf.length,
									    PROT_READ | PROT_WRITE,
										MAP_SHARED,
										fd,
										buf.m.offset);
 
        if (MAP_FAILED == buffers[n_buffers].start)
            errno_exit("mmap");
    }
}
 
void init_camera(char *dev_name)
{
    open_camera(dev_name);
    camera_format(dev_name);
    camera_mmap(dev_name);
}

static void usage(FILE * fp,int argc,char ** argv)
{
    fprintf(fp,
            "Usage: %s [options]\n\n"
            "Options:\n"
            "-d | --device name Video device name [/dev/video0]\n"
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
    char *dev_name = "/dev/video0";
    int time_in_sec_capture = 2;
    int index, c;
 
    for(;;) {
        c = getopt_long(argc, argv, short_options, long_options, &index);
        if(-1 == c)
            break;
 
        switch(c) {
        case 0:
			break;
 
        case 'd':
            dev_name = optarg;
			break;
 
        case 't':
            time_in_sec_capture = atoi(optarg);
            break;
 
        case 'h':
        default:
            usage(stderr, argc, argv);
            exit(EXIT_FAILURE);
        }
    }
 
    init_camera(dev_name);
    init_framebuffer("/dev/fb0");
    start_capturing();
    /* use select_run or sleep_run */
    select_run(time_in_sec_capture);
    //sleep_run(time_in_sec_capture);
    stop_capturing();
    close_camera();
    close_framebuffer();
    exit(EXIT_SUCCESS);
}
