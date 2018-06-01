/*
 * bmp.cpp
 *
 *  Created on: Dec 23, 2016
 *      Author: xiao
 */
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "bmp.h"

//BITMAPFILEHEADER filehead;
//BITMAPINFOHEADER infohead;
//RGBQUAD plate[256];	//256色调色板

#if 0
void read_img(char *file,IMAGE *SRC)
{
	FILE *fpi;
	fpi = fopen(file, "rb");
	if (fpi == NULL)
	{
		printf("open bmp file Error!!! ");
		exit(1);
	}

	WORD bfType;
	fread(&bfType, 1, sizeof(WORD), fpi);
	if (0x4d42 != bfType){
		printf("the file is not a bmp file!");
		exit(1);
	}

	fread(&filehead, 1, sizeof(tagBITMAPFILEHEADER), fpi);
	fread(&infohead, 1, sizeof(tagBITMAPINFOHEADER), fpi);
	//acquire height and width
	SRC->height = infohead.biHeight;
	SRC->width = infohead.biWidth;
	//printf("%d",infohead.biWidth);
	DWORD bitcolor;
	//DWORD lineByte = (infohead.biWidth*infohead.bmp_type + 31) / 32 * 4;
	DWORD lineByte = (infohead.biWidth*infohead.bmp_type / 8 + 3) / 4 * 4;//与上式等价 =3*width
	printf("lineByte = %d", lineByte);
	printf("bmp_type = %d", infohead.bmp_type);
	printf("biHeight = %d", infohead.biHeight);
	if (infohead.bmp_type == 1){ bitcolor = 2; printf("不能读取退出"); exit(-1); }
	if (infohead.bmp_type == 4){ bitcolor = 16; printf("不能读取退出"); exit(-1); }


	////读取调色板
	//for (unsigned int nCounti = 0; nCounti<infohead.biClrUsed; nCounti++)
	//{
	//	fread((char *)&(plate[nCounti].rgbBlue), 1, sizeof(BYTE), fpi);
	//	fread((char *)&(plate[nCounti].rgbGreen), 1, sizeof(BYTE), fpi);
	//	fread((char *)&(plate[nCounti].rgbRed), 1, sizeof(BYTE), fpi);
	//	fread((char *)&(plate[nCounti].rgbReserved), 1, sizeof(BYTE), fpi);
	//}

	RGBQUAD *bitmap;
	//8bit single channel image
	if (infohead.bmp_type == 8)
	{
		BYTE *temp = (BYTE*)malloc(infohead.biHeight * lineByte*sizeof(BYTE));
		memset(temp, 0x00, infohead.biHeight * lineByte * sizeof(BYTE));

		bitcolor = 256;//2^(8)
		bitmap = ( RGBQUAD *)calloc(bitcolor, sizeof(RGBQUAD));
		SRC->image = (unsigned char *)malloc(sizeof(unsigned char)*(lineByte*infohead.biHeight));
		memset(SRC->image, 0x00, sizeof(BYTE)*lineByte*infohead.biHeight);

		if (SRC->image == NULL) { fprintf(stderr, "\n Allocation error for temp in read_bmp() \n"); }

		fseek(fpi, 0x36, SEEK_SET);
		fread(bitmap, sizeof(RGBQUAD), bitcolor, fpi);
		fseek(fpi, filehead.header_length, SEEK_SET);
		fread(temp, lineByte *infohead.biHeight, 1, fpi);
		if (temp == NULL)printf("\n读取失败\n");

		for (int i = 0; i < infohead.biHeight; i++)
		{
			for (int j = 0; j < infohead.biWidth; j++)
			{
				SRC->image[i*infohead.biWidth + j] =
					(BYTE)(0.299*bitmap[temp[i*lineByte + j]].rgbBlue +
					0.578*bitmap[temp[i*lineByte + j]].rgbGreen +
					0.114*bitmap[temp[i*lineByte + j]].rgbRed);
			}
		}
		free(temp);
		temp = NULL;
	}

	//24bit color image
	if (infohead.bmp_type == 24)
	{

		BYTE *temp = (BYTE *)malloc(infohead.biHeight * lineByte * sizeof(BYTE));
		if (temp == NULL)exit(-1);

		SRC->image = (unsigned char *)malloc(lineByte * infohead.biHeight *sizeof(unsigned char));
		if (SRC->image == NULL) fprintf(stderr, "\n Allocation error for temp in read_bmp() \n");

		fseek(fpi, filehead.header_length, SEEK_SET);
		fread(temp, sizeof(unsigned char), lineByte * infohead.biHeight, fpi);

		for (int i = 0; i < infohead.biHeight; i++)
		{
			DWORD l = 0;
			for (int j = 0; j < infohead.biWidth * 3; j += 3)
			{
				l = (infohead.biHeight - i - 1)* infohead.biWidth * 3 + j;//图像是翻转过来的，因此这里需要这样做
				SRC->image[l + 2] = *(temp + i*lineByte + j + 2);//r
				SRC->image[l + 1] = *(temp + i*lineByte + j + 1);//g
				SRC->image[l] = *(temp + i*lineByte + j);//b

			}
		}

		free(temp);
		temp = NULL;
	}

	fclose(fpi);

}
#endif

bmp::bmp()
{
	imageWidth = 640;
	imageHeight = 480;
	imageBitCount = 8;
}

bmp::bmp(unsigned int width, unsigned int height)
{
	imageWidth = width;
	imageHeight = height;
	imageBitCount = 8;
}

void bmp::get_size(unsigned int *width, unsigned int *height)
{
	*width = imageWidth;
	*height = imageHeight;
}

void bmp::set_size(unsigned int width, unsigned int height)
{
	imageWidth = width;
	imageHeight = height;
}

int bmp::save_black_white(char *ImageName, unsigned char *image)
{
	int colorTablesize = 0;
	imageBitCount = 8;
	if (!ImageName || !image) {
		printf("bmp save param error\n");
		return -1;
	}

	//颜色表大小，以字节为单位
	//灰度图像颜色表为1024字节，彩色图像颜色表大小为0
	if (imageBitCount == 8)		//8bit图像--bmp_type
		colorTablesize = 1024;
	if (imageBitCount == 24)	//24bit彩色图像--bmp_type
		colorTablesize = 0;

	//待存储图像数据每行字节数为4的倍数
	int lineByte = (imageWidth * imageBitCount / 8 + 3) / 4 * 4;

	//以二进制写的方式打开文件
	FILE *fpw = fopen(ImageName, "wb");
	if (fpw == 0) {
		printf("bmp open file error\n");
		return -1;
	}

	//申请位图文件头结构变量，填写文件头信息
	BITMAPFILEHEADER fileHead;
	WORD bfType = 0x4D42;//bmp类型
	fwrite(&bfType, 1, sizeof(WORD), fpw);

	//bfSize是图像文件4个组成部分之和
	fileHead.bfSize = sizeof(BITMAPFILEHEADER) \
			+ sizeof(BITMAPINFOHEADER) + colorTablesize + lineByte * imageHeight;
	fileHead.bfReserved1 = 0;
	fileHead.bfReserved2 = 0;

	//bfOffBits是图像文件前3个部分所需空间之和
	fileHead.header_length = 54 + colorTablesize;

	//写文件头进文件
	fwrite(&fileHead, sizeof(BITMAPFILEHEADER), 1, fpw);

	//申请位图信息头结构变量，填写信息头信息
	BITMAPINFOHEADER head;
	head.bmp_type = imageBitCount;
	head.biClrImportant = 0;
	head.biClrUsed = 0;
	head.biCompression = 0;
	head.biHeight = imageHeight;
	head.biPlanes = 1;
	head.biSize = 40;
	head.biSizeImage = lineByte*imageHeight;
	head.biWidth = imageWidth;
	head.biXPelsPerMeter = 0;
	head.biYPelsPerMeter = 0;

	//写位图信息头进内存
	fwrite(&head, sizeof(BITMAPINFOHEADER), 1, fpw);

	RGBQUAD *pColorTable = (RGBQUAD *)malloc(sizeof(RGBQUAD)* 256);
	for (int i = 0; i < 256; i++)
	{
		pColorTable[i].rgbBlue = static_cast<BYTE>(i);
		pColorTable[i].rgbGreen = static_cast<BYTE>(i);
		pColorTable[i].rgbRed = static_cast<BYTE>(i);
		pColorTable[i].rgbReserved = static_cast<BYTE>(255);
	}

	//如果灰度图像，有颜色表，写入文件
	if (imageBitCount == 8)
	{
		fwrite(pColorTable, sizeof(RGBQUAD), 256, fpw);

		//图像是翻转过来的，也就是说数据是反向存储的，需要先整体反转，再每行反转
		correct_store(image, lineByte*imageHeight );
		for( int k = 0; k < imageHeight; k++ ){
			correct_store(image+(k*lineByte), lineByte );
		}

		//写位图数据进文件
		fwrite(image, imageHeight*lineByte, 1, fpw);

		if( pColorTable != NULL ){
			free(pColorTable);
		}
	} else if (imageBitCount == 24) {
		BYTE *temp = (BYTE *) malloc(head.biHeight * lineByte * sizeof(BYTE));

		for (int i = 0; i < head.biHeight; i++)
		{
			DWORD l = 0;
			for (int j = 0; j < head.biWidth * 3; j += 3)
			{
				l = (head.biHeight - i - 1)* head.biWidth * 3 + j;//图像是翻转过来的，因此这里需要这样做
				temp[l + 2] = *(image + i*lineByte + j + 2);	//r
				temp[l + 1] = *(image + i*lineByte + j + 1);	//g
				temp[l] = *(image + i*lineByte + j);	//b
			}
		}

		//fwrite(temp, imageHeight*lineByte, 1, fpw);//等价于下面
		for (int y = 0; y < imageHeight; y++)
		{
			for (int x = 0; x < lineByte; x+=3)//lineByte = 3*width
			{
				fwrite(&temp[y*lineByte + x], 1, sizeof(BYTE), fpw);
				fwrite(&temp[y*lineByte + x + 1], 1, sizeof(BYTE), fpw);
				fwrite(&temp[y*lineByte + x + 2], 1, sizeof(BYTE), fpw);

			}
		}

		if( temp != NULL ){
			free(temp);
		}
	}

	//关闭文件
	fclose(fpw);
	return 0;
}

/*
 *去噪算法
*/
void bmp::move_noise(unsigned char * newBuf){//双滤波器
	int i,j,k,temp[3],temp1[3];
	unsigned char BGR[13*3];
	unsigned int sq,sq1,loc,loc1;
	int h=imageHeight,w=imageWidth;

	for(i=2;i<h-2;i++){
		for(j=2;j<w-2;j++){
			memcpy(BGR,newBuf+(i-1)*w*3+3*(j-1),9);
			memcpy(BGR+9,newBuf+i*w*3+3*(j-1),9);
			memcpy(BGR+18,newBuf+(i+1)*w*3+3*(j-1),9);
			memcpy(BGR+27,newBuf+(i-2)*w*3+3*j,3);
			memcpy(BGR+30,newBuf+(i+2)*w*3+3*j,3);
			memcpy(BGR+33,newBuf+i*w*3+3*(j-2),3);
			memcpy(BGR+36,newBuf+i*w*3+3*(j+2),3);

			memset(temp,0,4*3);
			for(k=0;k<9;k++){
				temp[0]+=BGR[k*3];
				temp[1]+=BGR[k*3+1];
				temp[2]+=BGR[k*3+2];
			}

			temp1[0]=temp[0];
			temp1[1]=temp[1];
			temp1[2]=temp[2];

			for(k=9;k<13;k++){
				temp1[0]+=BGR[k*3];
				temp1[1]+=BGR[k*3+1];
				temp1[2]+=BGR[k*3+2];
			}

			for(k=0;k<3;k++){
				temp[k]/=9;
				temp1[k]/=13;
			}

			sq=0xffffffff;loc=0;
			sq1=0xffffffff;loc1=0;
			unsigned int a;

			for(k=0;k<9;k++){
				a=abs(temp[0]-BGR[k*3])+abs(temp[1]-BGR[k*3+1])+abs(temp[2]-BGR[k*3+2]);
				if(a<sq){
					sq=a;
					loc=k;
				}
			}
			for(k=0;k<13;k++){
				a=abs(temp1[0]-BGR[k*3])+abs(temp1[1]-BGR[k*3+1])+abs(temp1[2]-BGR[k*3+2]);
				if(a<sq1){
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
	printf("----move noise successfully---- !\n");
		return;
}

void bmp::yuv422_2_rgb( unsigned char *newBuf, unsigned char *starter, int yuv_len )
{
    unsigned char YUV[4],RGB[6];
    int i,j,k=0;
    unsigned int location = 0;
    for(i = 0;i < yuv_len; i+=4)
    {
        YUV[0] = starter[i];        // y
        YUV[1] = starter[i+1];      // u
        YUV[2] = starter[i+2];      // y
        YUV[3] = starter[i+3];      // v
        if(YUV[0] < 0){
            RGB[0]=0;
            RGB[1]=0;
            RGB[2]=0;
        }else{
            RGB[0] = YUV[0] + 1.772*(YUV[1]-128);       // b
            RGB[1] = YUV[0] - 0.34414*(YUV[1]-128) - 0.71414*(YUV[3]-128);      // g
            RGB[2] = YUV[0 ]+ 1.402*(YUV[3]-128);           // r
        }
        if(YUV[2] < 0)
        {
            RGB[3]=0;
            RGB[4]=0;
            RGB[5]=0;
        }else{
            RGB[3] = YUV[2] + 1.772*(YUV[1]-128);       // b
            RGB[4] = YUV[2] - 0.34414*(YUV[1]-128) - 0.71414*(YUV[3]-128);      // g
            RGB[5] = YUV[2] + 1.402*(YUV[3]-128) ;          // r
        }

        for(j = 0; j < 6; j++){
            if(RGB[j] < 0)
                RGB[j] = 0;
            if(RGB[j] > 255)
                RGB[j] = 255;
        }
        //请记住：扫描行在位图文件中是反向存储的！
        if(k%(imageWidth*3)==0)//定位存储位置
        {
            location=(imageHeight-k/(imageWidth*3))*(imageWidth*3);
        }
        bcopy(RGB,newBuf+location+(k%(imageWidth*3)),sizeof(RGB));
        k+=6;
    }
    return ;
}

int bmp::save_color(char *ImageName, unsigned char *image)
{
	int colorTablesize = 0;
	imageBitCount = 24;
	if (!ImageName || !image) {
		printf("bmp save param error\n");
		return -1;
	}

	//颜色表大小，以字节为单位
	//灰度图像颜色表为1024字节，彩色图像颜色表大小为0
	if (imageBitCount == 8)		//8bit图像--bmp_type
		colorTablesize = 1024;
	if (imageBitCount == 24)	//24bit彩色图像--bmp_type
		colorTablesize = 0;

	//待存储图像数据每行字节数为4的倍数
	int lineByte = (imageWidth * imageBitCount / 8 + 3) / 4 * 4;

	//以二进制写的方式打开文件
	FILE *fpw = fopen(ImageName, "wb");
	if (fpw == 0) {
		printf("bmp open file error\n");
		return -1;
	}

	//申请位图文件头结构变量，填写文件头信息
	BITMAPFILEHEADER fileHead;
	WORD bfType = 0x4D42;//bmp类型
	fwrite(&bfType, 1, sizeof(WORD), fpw);

	//bfSize是图像文件4个组成部分之和
	fileHead.bfSize = sizeof(BITMAPFILEHEADER) \
			+ sizeof(BITMAPINFOHEADER) + colorTablesize + lineByte * imageHeight;
	fileHead.bfReserved1 = 0;
	fileHead.bfReserved2 = 0;

	//bfOffBits是图像文件前3个部分所需空间之和
	fileHead.header_length = 54 + colorTablesize;

	//写文件头进文件
	fwrite(&fileHead, sizeof(BITMAPFILEHEADER), 1, fpw);

	//申请位图信息头结构变量，填写信息头信息
	BITMAPINFOHEADER head;
	head.bmp_type = imageBitCount;
	head.biClrImportant = 0;
	head.biClrUsed = 0;
	head.biCompression = 0;
	head.biHeight = imageHeight;
	head.biPlanes = 1;
	head.biSize = 40;
	head.biSizeImage = lineByte*imageHeight;
	head.biWidth = imageWidth;
	head.biXPelsPerMeter = 0;
	head.biYPelsPerMeter = 0;

	//写位图信息头进内存
	fwrite(&head, sizeof(BITMAPINFOHEADER), 1, fpw);

	RGBQUAD *pColorTable = (RGBQUAD *)malloc(sizeof(RGBQUAD)* 256);
	for (int i = 0; i < 256; i++)
	{
		pColorTable[i].rgbBlue = static_cast<BYTE>(i);
		pColorTable[i].rgbGreen = static_cast<BYTE>(i);
		pColorTable[i].rgbRed = static_cast<BYTE>(i);
		pColorTable[i].rgbReserved = static_cast<BYTE>(255);
	}

	//如果灰度图像，有颜色表，写入文件
	if (imageBitCount == 8)
	{
		fwrite(pColorTable, sizeof(RGBQUAD), 256, fpw);
		//写位图数据进文件
		fwrite(image, imageHeight*lineByte, 1, fpw);

		if( pColorTable != NULL ){
			free(pColorTable);
		}
	} else if (imageBitCount == 24) {
		BYTE *temp = (BYTE *) malloc(head.biHeight * lineByte * sizeof(BYTE));
#if 1

		yuv422_2_rgb(temp, image, imageWidth*imageHeight*2);	//imageWidth*imageHeight*2,表示图像的字节数yuyv
//		move_noise( temp );
//		fwrite(newBuf, 1, buf.length*3/2, file1);
		fwrite(temp, imageHeight*lineByte, 1, fpw);//等价于下面
#else
		for (int i = 0; i < head.biHeight; i++)
		{
			DWORD l = 0;
			for (int j = 0; j < head.biWidth * 3; j += 3)
			{
				l = (head.biHeight - i - 1)* head.biWidth * 3 + j;//图像是翻转过来的，因此这里需要这样做
				temp[l + 2] = *(image + i*lineByte + j + 2);	//r
				temp[l + 1] = *(image + i*lineByte + j + 1);	//g
				temp[l] = *(image + i*lineByte + j);	//b
			}
		}

		//fwrite(temp, imageHeight*lineByte, 1, fpw);//等价于下面
		for (int y = 0; y < imageHeight; y++)
		{
			for (int x = 0; x < lineByte; x+=3)//lineByte = 3*width
			{
				fwrite(&temp[y*lineByte + x], 1, sizeof(BYTE), fpw);
				fwrite(&temp[y*lineByte + x + 1], 1, sizeof(BYTE), fpw);
				fwrite(&temp[y*lineByte + x + 2], 1, sizeof(BYTE), fpw);

			}
		}
#endif
		if( temp != NULL ){
			free(temp);
		}
	}

	//关闭文件
	fclose(fpw);
	return 0;
}

void bmp::correct_store( unsigned char *correct_buf, int len )
{


    unsigned char *start = correct_buf;
    unsigned char *end = correct_buf + len - 1;
//    unsigned char *end = correct_buf + imageWidth*imageHeight - 1;
    unsigned char ch;

    if (correct_buf != NULL)
    {
        while (start < end)
        {
            ch = *start;
            *start++ = *end;
            *end-- = ch;
        }
    }
    return;
}
