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


#define CAMERA_DEVICE "/dev/video0"


#define CAPTURE_FILE "frame_yuyv_new.jpg"
#define CAPTURE_RGB_FILE "frame_rgb_new.bmp"
#define CAPTURE_show_FILE "a.bmp"


#define VIDEO_WIDTH 640
#define VIDEO_HEIGHT 480
#define VIDEO_FORMAT V4L2_PIX_FMT_YUYV
#define BUFFER_COUNT 4


typedef struct VideoBuffer {
    void   *start; //视频缓冲区的起始地址
    size_t  length;//缓冲区的长度
} VideoBuffer;


/*
*void *calloc(unsigned n,unsigned size) 功 能: 在内存的动态存储区中分配n个长度为size的连续空间，
*函数返回一个指向分配起始地址的指针；如果分配不成功，返回NULL。
*跟malloc的区别：calloc在动态分配完内存后，自动初始化该内存空间为零，而malloc不初始化，里边数据是随机的垃圾数据
*/
//位图文件头数据结构含有位图文件的类型，大小和打印格式等信息
//进行数据字节的对齐
#pragma pack(1)
typedef struct BITMAPFILEHEADER
{
  unsigned short bfType;//位图文件的类型,
  unsigned long bfSize;//位图文件的大小，以字节为单位
  unsigned short bfReserved1;//位图文件保留字，必须为0
  unsigned short bfReserved2;//同上
  unsigned long bfOffBits;//位图阵列的起始位置，以相对于位图文件   或者说是头的偏移量表示，以字节为单位
} BITMAPFILEHEADER;
#pragma pack()


typedef struct BITMAPINFOHEADER//位图信息头类型的数据结构，用于说明位图的尺寸
{
  unsigned long biSize;//位图信息头的长度，以字节为单位
  unsigned long biWidth;//位图的宽度，以像素为单位
  unsigned long biHeight;//位图的高度，以像素为单位
  unsigned short biPlanes;//目标设备的级别,必须为1
  unsigned short biBitCount;//每个像素所需的位数，必须是1(单色),4(16色),8(256色)或24(2^24色)之一
  unsigned long biCompression;//位图的压缩类型，必须是0-不压缩，1-BI_RLE8压缩类型或2-BI_RLE4压缩类型之一
  unsigned long biSizeImage;//位图大小，以字节为单位
  unsigned long biXPelsPerMeter;//位图目标设备水平分辨率，以每米像素数为单位
  unsigned long biYPelsPerMeter;//位图目标设备垂直分辨率，以每米像素数为单位
  unsigned long biClrUsed;//位图实际使用的颜色表中的颜色变址数
  unsigned long biClrImportant;//位图显示过程中被认为重要颜色的变址数
} BITMAPINFOHEADER;




VideoBuffer framebuf[BUFFER_COUNT];   //修改了错误，2012-5.21
int fd;
struct v4l2_capability cap;
struct v4l2_fmtdesc fmtdesc;
struct v4l2_format fmt;
struct v4l2_requestbuffers reqbuf;
struct v4l2_buffer buf;
unsigned char *starter;
unsigned char *newBuf;
struct BITMAPFILEHEADER bfh;
struct BITMAPINFOHEADER bih;


void create_bmp_header()
{
  bfh.bfType = (unsigned short)0x4D42;
  bfh.bfSize = (unsigned long)(14 + 40 + VIDEO_WIDTH * VIDEO_HEIGHT*3);
  bfh.bfReserved1 = 0;
  bfh.bfReserved2 = 0;
  bfh.bfOffBits = (unsigned long)(14 + 40);


  bih.biBitCount = 24;
  bih.biWidth = VIDEO_WIDTH;
  bih.biHeight = VIDEO_HEIGHT;
  bih.biSizeImage = VIDEO_WIDTH * VIDEO_HEIGHT * 3;
  bih.biClrImportant = 0;
  bih.biClrUsed = 0;
  bih.biCompression = 0;
  bih.biPlanes = 1;
  bih.biSize = 40;//sizeof(bih);
  bih.biXPelsPerMeter = 0x00000ec4;
  bih.biYPelsPerMeter = 0x00000ec4;
}










void yuyv2rgb()
{
    unsigned char YUYV[4],RGB[6];
    int j,k,i;
    unsigned int location=0;
    j=0;
    for(i=0;i < framebuf[buf.index].length;i+=4)
    {
        YUYV[0]=starter[i];//Y0
        YUYV[1]=starter[i+1];//U
        YUYV[2]=starter[i+2];//Y1
        YUYV[3]=starter[i+3];//V
        if(YUYV[0]<1)
        {
            RGB[0]=0;
            RGB[1]=0;
            RGB[2]=0;
        }
        else
        {
            RGB[0]=YUYV[0]+1.772*(YUYV[1]-128);//b
            RGB[1]=YUYV[0]-0.34413*(YUYV[1]-128)-0.71414*(YUYV[3]-128);//g
            RGB[2]=YUYV[0]+1.402*(YUYV[3]-128);//r
        }
        if(YUYV[2]<0)
        {
            RGB[3]=0;
            RGB[4]=0;
            RGB[5]=0;
        }
        else
        {
            RGB[3]=YUYV[2]+1.772*(YUYV[1]-128);//b
            RGB[4]=YUYV[2]-0.34413*(YUYV[1]-128)-0.71414*(YUYV[3]-128);//g
            RGB[5]=YUYV[2]+1.402*(YUYV[3]-128);//r


        }


        for(k=0;k<6;k++)
        {
            if(RGB[k]<0)
                RGB[k]=0;
            if(RGB[k]>255)
                RGB[k]=255;
        }


        //请记住：扫描行在位图文件中是反向存储的！
        if(j%(VIDEO_WIDTH*3)==0)//定位存储位置
        {
            location=(VIDEO_HEIGHT-j/(VIDEO_WIDTH*3))*(VIDEO_WIDTH*3);
        }
        bcopy(RGB,newBuf+location+(j%(VIDEO_WIDTH*3)),sizeof(RGB));


        j+=6;
    }
    return;
}


void move_noise()
{//双滤波器
    int i,j,k,temp[3],temp1[3];
    unsigned char BGR[13*3];
    unsigned int sq,sq1,loc,loc1;
    int h=VIDEO_HEIGHT,w=VIDEO_WIDTH;
    for(i=2;i<h-2;i++)
    {
        for(j=2;j<w-2;j++)
        {
            memcpy(BGR,newBuf+(i-1)*w*3+3*(j-1),9);
            memcpy(BGR+9,newBuf+i*w*3+3*(j-1),9);
            memcpy(BGR+18,newBuf+(i+1)*w*3+3*(j-1),9);
            memcpy(BGR+27,newBuf+(i-2)*w*3+3*j,3);
            memcpy(BGR+30,newBuf+(i+2)*w*3+3*j,3);
            memcpy(BGR+33,newBuf+i*w*3+3*(j-2),3);
            memcpy(BGR+36,newBuf+i*w*3+3*(j+2),3);


            memset(temp,0,4*3);

            for(k=0;k<9;k++)
            {
                temp[0]+=BGR[k*3];
                temp[1]+=BGR[k*3+1];
                temp[2]+=BGR[k*3+2];
            }
            temp1[0]=temp[0];
            temp1[1]=temp[1];
            temp1[2]=temp[2];
            for(k=9;k<13;k++)
            {
                temp1[0]+=BGR[k*3];
                temp1[1]+=BGR[k*3+1];
                temp1[2]+=BGR[k*3+2];
            }
            for(k=0;k<3;k++)
            {
                temp[k]/=9;
                temp1[k]/=13;
            }
            sq=0xffffffff;loc=0;
            sq1=0xffffffff;loc1=0;
            unsigned int a;
            for(k=0;k<9;k++)
            {
                a=abs(temp[0]-BGR[k*3])+abs(temp[1]-BGR[k*3+1])+abs(temp[2]-BGR[k*3+2]);
                if(a<sq)
                {
                    sq=a;
                    loc=k;
                }
            }
            for(k=0;k<13;k++)
            {
                a=abs(temp1[0]-BGR[k*3])+abs(temp1[1]-BGR[k*3+1])+abs(temp1[2]-BGR[k*3+2]);
                if(a<sq1)
                {
                    sq1=a;
                    loc1=k;
                }
            }

            newBuf[i*w*3+3*j]=(unsigned char)((BGR[3*loc]+BGR[3*loc1])/2);
            newBuf[i*w*3+3*j+1]=(unsigned char)((BGR[3*loc+1]+BGR[3*loc1+1])/2);
            newBuf[i*w*3+3*j+2]=(unsigned char)((BGR[3*loc+2]+BGR[3*loc1+2])/2);
            /*还是有些许的噪点
            temp[0]=(BGR[3*loc]+BGR[3*loc1])/2;
            temp[1]=(BGR[3*loc+1]+BGR[3*loc1+1])/2;
            temp[2]=(BGR[3*loc+2]+BGR[3*loc1+2])/2;
            sq=abs(temp[0]-BGR[loc*3])+abs(temp[1]-BGR[loc*3+1])+abs(temp[2]-BGR[loc*3+2]);
            sq1=abs(temp[0]-BGR[loc1*3])+abs(temp[1]-BGR[loc1*3+1])+abs(temp[2]-BGR[loc1*3+2]);
            if(sq1<sq) loc=loc1;
            newBuf[i*w*3+3*j]=BGR[3*loc];
            newBuf[i*w*3+3*j+1]=BGR[3*loc+1];
            newBuf[i*w*3+3*j+2]=BGR[3*loc+2];*/
        }
    }
    return;
}


void yuyv2rgb1()
{
    unsigned char YUYV[3],RGB[3];
    memset(YUYV,0,3);
    int j,k,i;
    unsigned int location=0;
    j=0;
    for(i=0;i < framebuf[buf.index].length;i+=2)
    {
        YUYV[0]=starter[i];//Y0
        if(i%4==0)
            YUYV[1]=starter[i+1];//U
        //YUYV[2]=starter[i+2];//Y1
        if(i%4==2)
            YUYV[2]=starter[i+1];//V
        if(YUYV[0]<1)
        {
            RGB[0]=0;
            RGB[1]=0;
            RGB[2]=0;
        }
        else
        {
            RGB[0]=YUYV[0]+1.772*(YUYV[1]-128);//b
            RGB[1]=YUYV[0]-0.34413*(YUYV[1]-128)-0.71414*(YUYV[2]-128);//g
            RGB[2]=YUYV[0]+1.402*(YUYV[2]-128);//r
        }


        for(k=0;k<3;k++)
        {
            if(RGB[k]<0)
                RGB[k]=0;
            if(RGB[k]>255)
                RGB[k]=255;
        }


        //请记住：扫描行在位图文件中是反向存储的！
        if(j%(VIDEO_WIDTH*3)==0)//定位存储位置
        {
            location=(VIDEO_HEIGHT-j/(VIDEO_WIDTH*3))*(VIDEO_WIDTH*3);
        }
        bcopy(RGB,newBuf+location+(j%(VIDEO_WIDTH*3)),sizeof(RGB));


        j+=3;
    }
    return;
}


void store_yuyv()
{
    FILE *fp = fopen(CAPTURE_FILE, "wb");
    if (fp < 0) {
        printf("open frame data file failed\n");
        return;
    }
    fwrite(framebuf[buf.index].start, 1, buf.length, fp);
    fclose(fp);
    printf("Capture one frame saved in %s\n", CAPTURE_FILE);
    return;
}




void store_bmp(int n_len)
{
    FILE *fp1 = fopen(CAPTURE_RGB_FILE, "wb");
    if (fp1 < 0) {
        printf("open frame data file failed\n");
        return;
    }
    fwrite(&bfh,sizeof(bfh),1,fp1);
    fwrite(&bih,sizeof(bih),1,fp1);
    fwrite(newBuf, 1, n_len, fp1);
    fclose(fp1);
    printf("Change one frame saved in %s\n", CAPTURE_RGB_FILE);
    return;
}


int main()
{
    int i, ret;



//     Get frame
/*
控制命令VIDIOC_DQBUF
功能：从视频缓冲区的输出队列中取得一个已经保存有一帧视频数据的视频缓冲区
参数说明：参数类型为V4L2缓冲区数据结构类型struct v4l2_buffer;
返回值说明： 执行成功时，函数返回值为 0;函数执行成功后，相应的内核视频缓冲区中保存有当前拍摄到的视频数据，应用程序可以通过访问用户空间来读取该视频数据（前面已经通过调用函数 mmap做了用户空间和内核空间的内存映射).


说明: VIDIOC_DQBUF命令结果, 使从队列删除的缓冲帧信息传给了此buf
V4L2_buffer结构体的作用就相当于申请的缓冲帧的代理，找缓冲帧的都要先问问它，通过它来联系缓冲帧，起了中间桥梁的作用
*/
    ret = ioctl(fd, VIDIOC_DQBUF, &buf);//VIDIOC_DQBUF命令结果, 使从队列删除的缓冲帧信息传给了此buf
    if (ret < 0) {
        printf("VIDIOC_DQBUF failed (%d)\n", ret);
        return ret;
    }


    // Process the frame 此时我们需要进行数据格式的改变
    store_yuyv();


    //对采集的数据进行转变，变换成RGB24模式，然后进行存储
/*
（1）开辟出来一段内存区域来存放转换后的数据
（2）循环读取buf内存段的内容，进行转换，转换后放入到新开辟的内存区域中
（3）将新开辟出来的内存区的内容读到文件中
*/
    printf("********************************************\n");
    int n_len;
    n_len=framebuf[buf.index].length*3/2;
    newBuf= (unsigned char*)calloc((unsigned int)n_len,sizeof(unsigned char));

    if(!newBuf)
    {
        printf("cannot assign the memory !\n");
        exit(0);
    }


    printf("the information about the new buffer:\n start Address:0x%x,length=%d\n\n",(unsigned int)newBuf,n_len);


    printf("----------------------------------\n");

    //YUYV to RGB
    starter=(unsigned char *)framebuf[buf.index].start;
    yuyv2rgb();//还是这个采集的图片的效果比较好
    move_noise();
    //yuyv2rgb1();
    //设置bmp文件的头和bmp文件的一些信息
    create_bmp_header();

    store_bmp(n_len);


    // Re-queen buffer
    ret = ioctl(fd, VIDIOC_QBUF, &buf);
    if (ret < 0) {
        printf("VIDIOC_QBUF failed (%d)\n", ret);
        return ret;
    }
    printf("re-queen buffer end\n");
    // Release the resource
/*
表头文件 #include<unistd.h>
        #include<sys/mman.h>
        定义函数 int munmap(void *start,size_t length);
        函数说明 munmap()用来取消参数start所指的映射内存起始地址，参数length则是欲取消的内存大小。当进程结束或利用exec相关函数来执行其他程序时，映射内存会自动解除，但关闭对应的文件描述词时不会解除映射
        返回值 如果解除映射成功则返回0，否则返回－1
*/
    for (i=0; i< 4; i++)
    {

        munmap(framebuf[i].start, framebuf[i].length);
    }
    //free(starter);
printf("free starter end\n");
    //free(newBuf);
printf("free newBuf end\n");
    close(fd);



    printf("Camera test Done.\n");
    return 0;
}
