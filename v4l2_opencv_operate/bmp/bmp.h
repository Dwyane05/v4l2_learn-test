/*
 * bmp.h
 *
 *  Created on: Dec 23, 2016
 *      Author: xiao
 */

#ifndef BMP_H_
#define BMP_H_
#pragma once
#pragma pack(push, 1)//把原来对齐方式设置压栈，并设新的对齐方式设置为一个字节对齐
#pragma pack(pop)

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef int BOOL;
typedef unsigned char uchar;

//位图文件头定义;
//其中不包含文件类型信息（由于结构体的内存结构决定，
//要是加了的话将不能正确读取文件信息）
typedef struct  tagBITMAPFILEHEADER{
	//WORD  bfType;//文件类型，必须是0x424D，即字符“BM”
	DWORD bfSize;			//文件大小
	WORD  bfReserved1;		//保留字
	WORD  bfReserved2;		//保留字
	DWORD header_length;	//从文件头到实际位图数据的偏移字节数
} BITMAPFILEHEADER;

//位图信息头定义
typedef struct tagBITMAPINFOHEADER{
	DWORD  biSize;			//信息头大小
	BOOL   biWidth;			//图像宽度
	BOOL   biHeight;		//图像高度
	WORD   biPlanes;		//位平面数，必须为1
	WORD   bmp_type;		//每像素位数
	DWORD  biCompression; 		//压缩类型
	DWORD  biSizeImage; 		//压缩图像大小字节数
	BOOL   biXPelsPerMeter; 	//水平分辨率
	BOOL   biYPelsPerMeter; 	//垂直分辨率
	DWORD  biClrUsed; 			//位图实际用到的色彩数
	DWORD  biClrImportant; 		//本位图中重要的色彩数
} BITMAPINFOHEADER;

//调色板定义
typedef struct tagRGBQUAD{
	BYTE rgbBlue; 		//该颜色的蓝色分量
	BYTE rgbGreen; 		//该颜色的绿色分量
	BYTE rgbRed; 		//该颜色的红色分量
	BYTE rgbReserved; 	//保留值
} RGBQUAD;


class bmp {
public:
	bmp();
	bmp(unsigned int width, unsigned int height);
	void set_size(unsigned int width, unsigned int height);
	void get_size(unsigned int *width, unsigned int *height);
	int save_black_white(char *ImageName, unsigned char *image);
	int save_color(char *ImageName, unsigned char *image);

	void move_noise(unsigned char * newBuf);
	void yuv422_2_rgb( unsigned char *newBuf, unsigned char *starter, int yuv_len );

	void correct_store( unsigned char *correct_buf, int len );

private:
	int imageWidth;
	int imageHeight;
	int imageBitCount;
};
#endif /* BMP_H_ */
