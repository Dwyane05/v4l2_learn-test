/*
 * main.c
 *
 *  Created on: Apr 25, 2016
 *      Author: anzyelay
 */
#include <unistd.h>
#include <linux/fb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>
#include <linux/videodev2.h>

#include "bmp.h"

#define CAMERA_DEVICE "/dev/video0"
#define FB_DEVICE "/dev/fb0"

#define VIDEO_WIDTH 640
#define VIDEO_HEIGHT 480
#define BUFFER_COUNT 4

typedef struct VideoBuffer {
    void   *start;
    size_t  length;
} VideoBuffer;

VideoBuffer framebuf[BUFFER_COUNT];

//查表法，把YUV422转换为RGB32
void process(unsigned long *rgb_buf, unsigned char *v_buf)
{
    int r,g,b;
    int u,v;
    int y[2];
    int rv,guv,bu,i;
    unsigned int *fb_buf = (unsigned int *)rgb_buf;
    y[0]=(int)*v_buf;
    v=(int)*(v_buf+1);
    y[1]=(int)*(v_buf+2);
    u=(int)*(v_buf+3);
    rv=rvarrxyp[v];
    guv=guarrxyp[u]+gvarrxyp[v];
    bu=buarrxyp[u];

    for(i=0;i<2;i++){
        r=y[i]+rv;
        g=y[i]-guv;
        b=y[i]+bu;
        if (r>255) r=255;
        if (r<0) r=0;
        if (g>255) g=255;
        if (g<0) g=0;
        if (b>255) b=255;
        if (b<0) b=0;
        *(fb_buf+i)=(b<<16)+(g<<8)+r;
    }
}


//------------save image picture captured--------///////
int GenBmpFile(U8 *pData, U8 bitCountPerPix, U32 width, U32 height, const char *filename)
{
    FILE *fp = fopen(filename, "wb");
    if(!fp)
    {
        printf("fopen failed : %s, %d\n", __FILE__, __LINE__);
        return 0;
    }
    U32 bmppitch = ((width*bitCountPerPix + 31) >> 5) << 2;
    U32 filesize = bmppitch*height;

    BITMAPFILE bmpfile;
    bmpfile.bfHeader.bfType = 0x4D42;
    bmpfile.bfHeader.bfSize = filesize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmpfile.bfHeader.bfReserved1 = 0;
    bmpfile.bfHeader.bfReserved2 = 0;
    bmpfile.bfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    bmpfile.biInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpfile.biInfo.bmiHeader.biWidth = width;
    bmpfile.biInfo.bmiHeader.biHeight = height;
    bmpfile.biInfo.bmiHeader.biPlanes = 1;
    bmpfile.biInfo.bmiHeader.biBitCount = bitCountPerPix;
    bmpfile.biInfo.bmiHeader.biCompression = 0;
    bmpfile.biInfo.bmiHeader.biSizeImage = filesize;
    bmpfile.biInfo.bmiHeader.biXPelsPerMeter = 0;
    bmpfile.biInfo.bmiHeader.biYPelsPerMeter = 0;
    bmpfile.biInfo.bmiHeader.biClrUsed = 0;
    bmpfile.biInfo.bmiHeader.biClrImportant = 0;

    fwrite(&(bmpfile.bfHeader), sizeof(BITMAPFILEHEADER), 1, fp);
    fwrite(&(bmpfile.biInfo.bmiHeader), sizeof(BITMAPINFOHEADER), 1, fp);
    fwrite(pData,filesize,1, fp);
//    U8 *pEachLinBuf = (U8*)malloc(bmppitch);
//    memset(pEachLinBuf, 0, bmppitch);
//    U8 BytePerPix = bitCountPerPix >> 3;
//    U32 pitch = width * BytePerPix;
//    if(pEachLinBuf)
//    {
//        int h,w;
//        for(h = height-1; h >= 0; h--)
//        {
//            for(w = 0; w < width; w++)
//            {
//                //copy by a pixel
//                pEachLinBuf[w*BytePerPix+0] = pData[h*pitch + w*BytePerPix + 0];
//                pEachLinBuf[w*BytePerPix+1] = pData[h*pitch + w*BytePerPix + 1];
//                pEachLinBuf[w*BytePerPix+2] = pData[h*pitch + w*BytePerPix + 2];
//            }
//            fwrite(pEachLinBuf, bmppitch, 1, fp);
//
//        }
//        free(pEachLinBuf);
//    }

    fclose(fp);

    return 1;
}

int main()
{
    int i, ret;
    // 打开摄像头设备
    int fd;
    fd = open(CAMERA_DEVICE, O_RDWR, 0);
    if (fd < 0) {
        printf("Open %s failed\n", CAMERA_DEVICE);
        return -1;
    }

    // 获取驱动信息
    struct v4l2_capability cap;
    ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
    if (ret < 0) {
        printf("VIDIOC_QUERYCAP failed (%d)\n", ret);
//        goto __error1;
        close(fd);
    }

    printf("Capability Informations:\n");
    printf(" driver: %s\n", cap.driver);
    printf(" card: %s\n", cap.card);
    printf(" bus_info: %s\n", cap.bus_info);
    printf(" version: %u.%u.%u\n", (cap.version>>16)&0XFF, (cap.version>>8)&0XFF,cap.version&0XFF);
    printf(" capabilities: %08X\n", cap.capabilities);

    // 设置视频格式
    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = VIDEO_WIDTH;
    fmt.fmt.pix.height      = VIDEO_HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
    ret = ioctl(fd, VIDIOC_S_FMT, &fmt);
    if (ret < 0) {
        printf("VIDIOC_S_FMT failed (%d)\n", ret);
//        goto 	error1;
        close(fd);
    }

    //获取视频格式(原因是设置视频格式时如果有错误，VIDIOC_S_FMT时会自动转换成合作的格式，所以再获取一次)
    ret = ioctl(fd, VIDIOC_G_FMT, &fmt);
    if (ret < 0) {
        printf("VIDIOC_G_FMT failed (%d)\n", ret);
//        goto 	error1;
        close(fd);
    }
    // Print Stream Format
    printf("Stream Format Informations:\n");
    printf(" type: %d\n", fmt.type);
    printf(" width: %d\n", fmt.fmt.pix.width);
    printf(" height: %d\n", fmt.fmt.pix.height);
    char fmtstr[8];
    memset(fmtstr, 0, 8);
    memcpy(fmtstr, &fmt.fmt.pix.pixelformat, 4);
    printf(" pixelformat: %s\n", fmtstr);
    printf(" field: %d\n", fmt.fmt.pix.field);
    printf(" bytesperline: %d\n", fmt.fmt.pix.bytesperline);
    printf(" sizeimage: %d\n", fmt.fmt.pix.sizeimage);
    printf(" colorspace: %d\n", fmt.fmt.pix.colorspace);
    printf(" priv: %d\n", fmt.fmt.pix.priv);
    printf(" raw_date: %s\n", fmt.fmt.raw_data);

//-----------------------------------------------
    //请求分配内存
    struct v4l2_requestbuffers reqbuf;
    reqbuf.count = BUFFER_COUNT;
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(fd , VIDIOC_REQBUFS, &reqbuf);//在内核虚拟地址空间中申请reqbuf.count个连续的内存
    if(ret < 0) {
        printf("VIDIOC_REQBUFS failed (%d)\n", ret);
//        goto 	error1;
        close(fd);
    }

    struct v4l2_buffer buf;
    for (i = 0; i < reqbuf.count; i++)
    {
        buf.index = i;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        //根据buf.index询访内存为后面的映射做准备（本质就是设置buf.m.offset+=buf.length偏移）
        ret = ioctl(fd , VIDIOC_QUERYBUF, &buf);
        if(ret < 0) {
            printf("VIDIOC_QUERYBUF (%d) failed (%d)\n", i, ret);
//            goto 	error1;
            close(fd);
        }
        //映射到用户空间
        //就是将之前内核分配的视频缓冲（VIDIOC_REQBUFS）映射到用户空间，这样用户空间就可以直接读取内核扑获的视频数据
        //buf.m.offset表示要对内核中的哪个video buffer进行映射操作
        framebuf[i].length = buf.length;
        framebuf[i].start = (char *) mmap(0, buf.length, PROT_READ|PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
        if (framebuf[i].start == MAP_FAILED) {
            printf("mmap (%d) failed: %s\n", i, strerror(errno));
            ret = -1;
//            goto error1;
            close(fd);
        }

        //内存入队列
        ret = ioctl(fd , VIDIOC_QBUF, &buf);
        if (ret < 0) {
            printf("VIDIOC_QBUF (%d) failed (%d)\n", i, ret);
//            goto error2;
            for (i=0; i< 4; i++)
            {
                munmap(framebuf[i].start, framebuf[i].length);
            }
        }

        printf("Frame buffer %d: address=0x%x, length=%d\n", i,
        		(unsigned long)framebuf[i].start,
        		(unsigned int)framebuf[i].length);
    }
//--------------------------------------

    // 启动视频流
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(fd, VIDIOC_STREAMON, &type);
    if (ret < 0) {
        printf("VIDIOC_STREAMON failed (%d)\n", ret);
//        goto error2;
        for (i=0; i< 4; i++)
        {
            munmap(framebuf[i].start, framebuf[i].length);
        }
    }
#if 0
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    int x,y;
    int fd_fb;
    long int screen_size = 0;
    unsigned long  *fbp32 = NULL;
    //打开LCD设备
    fd_fb = open(FB_DEVICE, O_RDWR);
    if (fd_fb < 0)
    {
        printf("Error: cannot open framebuffer device.\n");
//        goto  error2;
        for (i=0; i< 4; i++)
        {
            munmap(framebuf[i].start, framebuf[i].length);
        }
    }
    if (ioctl(fd_fb, FBIOGET_FSCREENINFO, &finfo))
    {
        printf("Error reading fixed information.\n");
//        goto exit_3;
        close(fd_fb);
    }
    //获取LCD属性信息
    if (ioctl(fd_fb, FBIOGET_VSCREENINFO, &vinfo))
    {
        printf("Error 2 reading variable information.\n");
//        goto  exit_3;
        close(fd_fb);
    }

    //一屏的字节数
    screen_size = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
    printf("LCD:  %dx%d, %dbpp, screen_size = %ld\n",
    		vinfo.xres,
    		vinfo.yres,
    		vinfo.bits_per_pixel,
    		screen_size );

    //映射framebuffer到用户空间
    fbp32 = (unsigned long *)mmap(0, screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd_fb, 0);
    if (fbp32 == MAP_FAILED)
    {
        printf("Error: failed to map framebuffer device to memory.\n");
//        goto	exit_3;
        close(fd_fb);
    }
#endif

    printf("start camera testing...\n");
    //开始视频显示
//    while(1)
    unsigned long *picdata = (unsigned long *)malloc(fmt.fmt.pix.height*fmt.fmt.pix.width*4);
    memset(picdata,0,fmt.fmt.pix.height*fmt.fmt.pix.width*4);
    for(i=0;i<1;i++)
    {
    	memset(&buf,0,sizeof(buf));
        buf.index = 0;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
    	//内存空间出队列
       ret = ioctl(fd, VIDIOC_DQBUF, &buf);
       if (ret < 0)
       {
           printf("VIDIOC_DQBUF failed (%d)\n", ret);
           break;
       }
#if 0
       if(vinfo.bits_per_pixel == 32)
       {
    	   //yuv422 两个像素占4个字节（Y0，V，Y1，U），两个像素共用一个UV对，总字节数为W*H*2，
    	   //而RGB32则正好两个像素是一个LONG型对齐，以LONG为单位一次可计算两个像素点，之后framebuf自增4个字节地址指向一两个像素点
    	   for(y = 0; y < fmt.fmt.pix.height;  y++)
           {
               for(x = 0; x < fmt.fmt.pix.width/2; x++)
               {
                   //YUV422转换为RGB32
//                   process(fbp32 + y*vinfo.xres + x,
//                		   (U8 *)framebuf[buf.index].start + (y*fmt.fmt.pix.width+x)*2);
            	   //由于是以long为基数自增，所以一行long数为：width(long)=width(int)/2
            	   process((picdata + y*fmt.fmt.pix.width/2 + x),
            			   (U8 *)framebuf[buf.index].start + y*fmt.fmt.pix.width*2+x*4);
               }
            }

        }
#endif
        // 内存重新入队列
        ret = ioctl(fd, VIDIOC_QBUF, &buf);
        if (ret < 0)
          {
            printf("VIDIOC_QBUF failed (%d)\n", ret);
            break;
          }

    }//while(1)

    /*save image picture captured*/
    char picname[100];
    sprintf(picname,"data_%d*%d.bmp",fmt.fmt.pix.width,fmt.fmt.pix.height);
    GenBmpFile((U8 *)picdata,32,fmt.fmt.pix.width,fmt.fmt.pix.height,picname);

#if 0
    //释放fbp32资源
    munmap(fbp32, screen_size);
#endif
//    //关闭fd_fb设备
////exit_3:
//	close(fd_fb);
//
//	//释放framebuf资源
////error2:
//    for (i=0; i< 4; i++)
//    {
//        munmap(framebuf[i].start, framebuf[i].length);
//    }
//
////error1:	//关闭fd设备
//    close(fd);
    return ret;
}


