#ifndef _V4L_H_
#define _V4L_H_

#include <linux/videodev2.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <error.h>
#include<errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>


struct   video_window   win;  
struct   video_capability   cap;  
struct   video_picture   pic;  
struct   video_mbuf   mbuf;//={320*240*3,1,0};
/* mbuf.size=320*240*3;
mbuf.frames=1;
mbuf.offsets[VIDEO_MAX_FRAME]=0;*/
  
char picbuff[320*240*3];
int size=sizeof(picbuff);  
int fd;
int fp;

// 1. 打开摄像
void open_device()
{
if((fd=open("/dev/video0",O_RDWR))<0)
   perror("open_dev");
else
   printf("open device success!\n");
}

// 2.   获取相关参数  
void get_capability()
{     
if(-1==ioctl(fd,VIDIOCGCAP,&cap))
perror("VIDIOCGCAP");
else
   printf("get capability success\n"
    "the name :%s\n"
    "the maxwidth:%d\n"
    "the maxheight:%d\n"
    "the minwidth:%d\n"
    "the minheight:%d\n"
    "the channels:%d\n"
"the type:%d\n"
"the audios:%s\n",
cap.name,cap.maxwidth,cap.maxheight,cap.minwidth,cap.minheight,cap.channels,cap.type,cap.audios);
}

void get_window()
{
if(-1==ioctl(fd,VIDIOCGWIN,&win))
perror("get_win");
else
   printf("get window success\n"
   "the position : x %d y %d\n"
   "the window size: width %d height %d\n",win.x,win.y,win.width,win.height);
}

void get_picture()
{
if(-1==ioctl(fd,VIDIOCGPICT,&pic))
perror("get picture");
else
printf("get picture success\n""the brightness:%d\n"
"the hue:%d\n"
"the colour:%d\n"
"the contrast:%d\n"
"the whiteness:%d\n"
"the depth:%d\n"
"the palette:%d\n",pic.brightness,pic.hue,pic.colour,pic.contrast,pic.whiteness,pic.depth,pic.palette);
}
void mmap_chek()
{
//Check   to   see   if   we   can   use   mmap  
if(-1==ioctl(fd,VIDIOCGMBUF,&mbuf))
perror("get_mbuf");
else
       printf("mbuf_size:%d\n"
      "mbuf_frames:%d\n"
      "mbuf_offset:%d\n",mbuf.size,mbuf.frames,mbuf.offsets[0]);
}
   
/*
//Check   to   see   if   this   camera   uses   MJPEG  
if(vid_caps.type&VID_TYPE_MJPEG_ENCODER);  
   
// 3.   设置相关参数  
ioctl(fd,VIDIOCSPICT,&vid_pic);  
ioctl(fd,VIDIOCSWIN,&vid_win);  
   
// 4.   获取图象数据 */
void cap_picture()
{
if(read(fd,picbuff,size)<0)
{
perror("cap_picture");
fprintf(stderr,"cap picture%s\n",strerror(errno));
}
else
if(-1==write(fp,picbuff,size))
   perror("write wrong");
}

void open_file()
{
    if(-1==(fp=open("picture",O_RDWR|O_CREAT,0666)))//获取文件的描述符
        perror("open file");
    else
        printf("open file success!\n");
}
int main()
{
open_device();
open_file();
get_capability();
get_window();
get_picture();
cap_picture();
return 0;
}
#endif
