
#include <stdio.h>
#include <stdlib.h> //stdio.h and stdlib.h are needed by perror function
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h> //O_RDWR
#include <unistd.h>
#include <sys/mman.h> //unistd.h and sys/mman.h are needed by mmap function
#include <stdbool.h>//false and true
#include <sys/ioctl.h>
#include <linux/videodev2.h>//v4l API

typedef struct _v4l_struct
{
 int fd;
 struct video_capability capability;
 struct video_picture picture;
 struct video_mmap mmap;
 struct video_mbuf mbuf;
 unsigned char *map;
 int frame_current;
 int frame_using[VIDEO_MAX_FRAME];
}v4l_device;

extern int v4l_open(char*,v4l_device*);
extern int v4l_close(v4l_device*);
extern int v4l_get_capability(v4l_device*);
extern int v4l_get_picture(v4l_device*);
extern int v4l_get_mbuf(v4l_device*);
extern int v4l_set_picture(v4l_device*,int,int,int,int,int);
extern int v4l_grab_picture(v4l_device*,unsigned int);
extern int v4l_mmap_init(v4l_device*);
extern int v4l_grab_init(v4l_device*,int,int);
extern int v4l_grab_frame(v4l_device*,int);
extern int v4l_grab_sync(v4l_device*);

#define DEFAULT_DEVICE "/dev/video0"

int v4l_open(char *dev,v4l_device *vd)
{
 if(!dev)
 dev=DEFAULT_DEVICE;
 if((vd->fd=open(dev,O_RDWR))<0)
 {
  perror("v4l_open fail");
  return -1;
 
 }
 if(v4l_get_capability(vd))
  return -1;
 printf("video capture device name:%s\n",vd->capability.name);
 if(v4l_get_picture(vd))
  return -1;
 printf("frames number is %d\n",vd->mbuf.frames);
 return 0;
}

int v4l_close(v4l_device *vd)
{
 munmap(vd->map,vd->mbuf.size);
 close(vd->fd);
 return 0;
}

int v4l_get_capability(v4l_device *vd)
{
 if(ioctl(vd->fd,VIDIOCGCAP,&vd->capability)<0)
 { 
  perror("v4l_get_capability fail");
  return -1;
 }
 return 0; 
}

int v4l_get_picture(v4l_device *vd)
{
 if(ioctl(vd->fd,VIDIOCGPICT,&vd->picture)<0)
 {
  perror("v4l_get_picture fail");
  return -1;
 }
 return 0;
}

int v4l_get_mbuf(v4l_device *vd)
{
 if(ioctl(vd->fd,VIDIOCGMBUF,&vd->mbuf)<0)
 {
  perror("v4l_get_mbuf fail");
  return -1;
 }
 return 0;
}

int v4l_set_picture(v4l_device *vd,int br,int hue,int col,int cont,int white)
{
 if(br)
  vd->picture.brightness=br;
 if(hue)
  vd->picture.hue=hue;
 if(col) 
  vd->picture.colour=col;
 if(cont)
  vd->picture.contrast=cont;
 if(white)
  vd->picture.whiteness=white;
 if(ioctl(vd->fd,VIDIOCSPICT,&vd->picture)<0)
 {
  perror("v4l_set_picture fail");
  return -1;
 }
 return 0;
}

int v4l_grab_picture(v4l_device *vd,unsigned int size)
{
 if(read(vd->fd,vd->map,size)==0)
 return -1;
 return 0;
}

int v4l_mmap_init(v4l_device *vd)
{
 if(v4l_get_mbuf(vd)<0)
  return -1;
 if((vd->map=(unsigned char*)mmap(0,vd->mbuf.size,PROT_READ|PROT_WRITE,MAP_SHARED,vd->fd,0))<0)
 {
  perror("v4l_mmap_init fail");
  return -1;
 }
 return 0;
}

int v4l_grab_init(v4l_device *vd,int width,int height)
{
 vd->mmap.width=width;
 vd->mmap.height=height;
 vd->mmap.format=vd->picture.palette;
 vd->frame_current=0;
 vd->frame_using[0]=false;
 vd->frame_using[1]=false;
 return v4l_grab_frame(vd,0);
}

int v4l_grab_frame(v4l_device *vd,int frame)
{
 if(vd->frame_using[frame])
 {
  fprintf(stderr,"v4l_grab_frame %d is already used\n",frame);
  return -1;
 }
 vd->mmap.frame=frame;
 if(ioctl(vd->fd,VIDIOCMCAPTURE,&vd->mmap)<0)
 {
 
  perror("v4l_grab_frame fail");
 
  return -1;
 
 }
 vd->frame_using[frame]=true;
 vd->frame_current=frame;
 return 0;
}

int v4l_grab_sync(v4l_device *vd)
{
 if(ioctl(vd->fd,VIDIOCSYNC,&vd->frame_current)<0)
 { 
  perror("v4l_grab_sync fail");
  return -1;
 }
 vd->frame_using[vd->frame_current]=false;
 return 0;
}

main()
{
}
