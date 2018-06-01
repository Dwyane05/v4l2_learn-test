#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>  
#include <errno.h>
#include <getopt.h>
#include "Camera.h"
#include "bmp.h"
#include "ov7725_regs.h"

using namespace std;

#include <iostream>
#include <string>
#include <sstream>
#include <unistd.h>
using namespace std;

//OpenCV 头文件
#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
using namespace cv;

#define DBG_EN		1
#if(DBG_EN == 1)
#define printf_dbg(x,arg...) printf("[main_debug]"x,##arg)
#else
#define printf_dbg(x,arg...)
#endif

#define printf_info(x,arg...) printf("[main_info]"x,##arg)
#define printf_warn(x,arg...) printf("[main_warn]"x,##arg)
#define printf_err(x,arg...) printf("[main_err]"x,##arg)

#define CAMERA_DEVICE "/dev/video0"

#define VIDEO_HEIGHT (480)
#define VIDEO_WIDTH (640)


void set_register_values( bool high );
bool camera_start();
void camera_end();

/* 毫秒级 延时 */
void msleep(int ms)
{
    struct timeval delay;
    delay.tv_sec = 0;
    delay.tv_usec = ms * 1000;
    select(0, NULL, NULL, NULL, &delay);
}

Camera yoko_camera(VIDEO_WIDTH, VIDEO_HEIGHT);

int main(int argc, char *argv[])
{
	int ret;
//	int enum_fmt = 0;
	char *buf = NULL;
	char *y_buf = NULL;

	bool espo_high = true;

	if( !camera_start() ){
		printf_err( "camera start failed\n" );
		exit(-1);
	}

	buf = (char *) malloc(VIDEO_HEIGHT * VIDEO_WIDTH*2);
	if(!buf) {
		camera_end();
		exit(-1);
	}

	y_buf = (char *) malloc(VIDEO_HEIGHT * VIDEO_WIDTH);
	if(!y_buf) {
		camera_end();
		exit(-1);
	}
	Mat result = Mat::zeros( VIDEO_HEIGHT, VIDEO_WIDTH,  CV_8UC1 );
/********************************** init OK!********************/
	for(int i = 1; i <= 5 ; i++)
	{
		int len = 0;
		int y_len = 0;
		Mat frame1 = Mat::zeros( VIDEO_HEIGHT, VIDEO_WIDTH,  CV_8UC1 );
		ret = yoko_camera.get_imagedata((unsigned char*) buf, &len);
		if(ret < 0 ) {
			printf_err("Get image data fail\n");
			continue;
		}

		ret = yoko_camera.get_y_component((unsigned char*) buf,(unsigned char*)y_buf, &y_len );
		if(ret < 0 ) {
			printf_err("Get image data fail\n");
			continue;
		}
		printf_dbg("get y_data len :%d\n",y_len);
		if( 5==i ){
			frame1.data = (unsigned char*) y_buf;
			result = frame1.clone();
			string name = "video_h";
			char st[25];
			sprintf(st, "%d", i );
			string num = st;
			name.append( num );
			name.append(".png");
//			imwrite( name, frame1 );
		}
	}
	camera_end();
	camera_start();
	Rect roi( 0, 0, 640, 340);
	Mat result_roi = result(roi);
	Mat zero_roi = Mat::zeros( 340, 640, CV_8UC1 );
	zero_roi.copyTo(result_roi);
//	imwrite( "zero_roi.png", result );
	for(int i = 1; i <= 5 ; i++)
	{
		int len = 0;
		int y_len = 0;
		Mat frame2 = Mat::zeros( VIDEO_HEIGHT, VIDEO_WIDTH,  CV_8UC1 );

		espo_high = false;
		set_register_values( espo_high );

		ret = yoko_camera.get_imagedata((unsigned char*) buf, &len);
		if(ret < 0 ) {
			printf_err("Get image data fail\n");
			continue;
		}

		ret = yoko_camera.get_y_component((unsigned char*) buf,(unsigned char*)y_buf, &y_len );
		if(ret < 0 ) {
			printf_err("Get image data fail\n");
			continue;
		}
		printf_dbg("get y_data len :%d\n",y_len);
		if( 5==i ){
			frame2.data = (unsigned char*) y_buf;
			Mat frame2_roi = frame2( roi );
			frame2_roi.copyTo( result_roi );
			string name = "video_l";
			char st[25];
			sprintf(st, "%d", i );
			string num = st;
			name.append( num );
			name.append(".png");
//			imwrite( name, frame2 );
			imwrite( "result.png", result );
		}
	}


	free(buf);
	free(y_buf);

	return 0;
}

bool camera_start( )
{
	int ret;
	/*CAMERA*/
	printf_dbg("init camera\n");
	ret = yoko_camera.open_dev(CAMERA_DEVICE, true);
	if(ret < 0) {
		printf_err("open camera device %s fail\n",CAMERA_DEVICE);
		return -1;
	}
	printf_dbg("\n\nyokocam open camera ok\n\n");

//	yoko_camera.enum_format();
//	printf("\n\nyokocam enum support format ok\n\n");

	ret = yoko_camera.set_format(VIDEO_WIDTH, VIDEO_HEIGHT);
	if(ret < 0) {
		printf_err("Set format fail\n");
		camera_end();
		return false;
	}
	printf("\n\nyokocam set format ok\n\n");


	ret = yoko_camera.init_device();
	if(ret < 0) {
		printf_err("init device %s fail\n",CAMERA_DEVICE);
		camera_end();
		return false;
	}
	printf("\n\nyokocam init camera ok\n\n");

	ret = yoko_camera.start_capturing();
	if(ret < 0) {
		printf_err("camera streamon fail\n");
		yoko_camera.uninit_device();
		camera_end();
		return false;
	}
	printf_info("Image capture init ok!\n");
	return true;
}

void camera_end()
{
	yoko_camera.stop_capturing();
	yoko_camera.uninit_device();
	yoko_camera.close_dev();
	printf_dbg( "camera end\n" );
}
void set_register_values( bool high )
{
	int value_aew, value_aeb, value_vpt;
	if( high ){
		value_aew = 0x96;
		value_aeb = 0x10;
		value_vpt = 0xA1;
	}else{
		value_aew = 0x25;
		value_aeb = 0x15;
		value_vpt = 0x31;
	}


	//打开自动增益(AGC)、自动白平衡(AWB)、自动曝光(AEC)
	yoko_camera.set_REGISTER(COM8,			0xff);  //所有辅助功能全开，0xfd会变绿，0xff正常图像
	yoko_camera.set_REGISTER(AEW,			value_aew); //AEC启动的高值0x50 控制自动曝光强弱 0x20
	yoko_camera.set_REGISTER(AEB,			value_aeb); //AEC启动的低值  这两个值改低，使得摄像头适应低曝光情况	0x30 控制自动曝光强弱 0x15
	yoko_camera.set_REGISTER(VPT,			value_vpt); //快速模式工作区 前一位(5)*16=80 后一位(2)*16=32；
	//也就是高值和aec启动高值相等，后面一位改低也是为了适应低曝光0x52 0x21

	yoko_camera.set_REGISTER(COM9,			0x4A); //32倍数快速AGC，调整曝光

	yoko_camera.set_REGISTER(SDE,           0x07);	//使能控制图片亮暗的寄存器
	//	yoko_camera.set_REGISTER(SDE,           0x20);	//(0xa6, 0x02)黑白

/**************************************手册参数******************************************************/

	//下面的寄存器可以设置简单的白平衡
	yoko_camera.set_REGISTER(AWB_CTRL1,    	0x5d);		// Simple AWB

//	//lignt mode
	/**************************
	 * Auto		(COM8, 		0xff); //AWB on (COM5, 		0x65);	(ADVFL, 	0x00);	(ADVFH, 	0x00);
	 * Sunny 	(0x13, 0xfd); //AWB off (0x01, 0x5a);(0x02, 0x5c);(0x0e, 0x65);(0x2d, 0x00);(0x2e, 0x00);
	 * Cloudy	(0x13, 0xfd); //AWB off (0x01, 0x58);(0x02, 0x60);(0x0e, 0x65);(0x2d, 0x00);(0x2e, 0x00);
	 * Office	(COM8, 		0xfd); //AWB off (BLUE, 0x84);(RED, 0x4c);(COM5, 0x65);(ADVFL,0x00);(ADVFH,0x00);
	 * Home		(0x13, 0xfd); //AWB off (0x01, 0x96);(0x02, 0x40);(0x0e, 0x65);(0x2d, 0x00);(0x2e, 0x00);
	 * Night	0x13, 0xff); //AWB on  (0x0e, 0xe5);
	 * *******************************/
////	Auto
//	yoko_camera.set_REGISTER(COM8, 		0xff); //AWB on
	yoko_camera.set_REGISTER(COM5, 		0x65);
	yoko_camera.set_REGISTER(ADVFL, 	0x00);
	yoko_camera.set_REGISTER(ADVFH, 	0x00);


	/***********Color Saturation颜色饱和度
	Saturation + 4		--0x80
	Saturation + 3		--0x70
	Saturation + 2		--0x60
	Saturation + 1		--0x50
	Saturation + 0		--0x40
	Saturation + -1		--0x30
	Saturation + -2		--0x20
	Saturation + -3		--0x10
	Saturation + -4		--0x00
	 * ********************/
	yoko_camera.set_REGISTER(USAT, 		0x50);
	yoko_camera.set_REGISTER(VSAT, 		0x50);

	/**********下面两个寄存器可以控制亮度等级
	Brightness +4		0x48	0x06
	Brightness +3		0x38	0x06
	Brightness +2		0x28	0x06
	Brightness +1		0x18	0x06
	Brightness 0		0x08	0x06
	Brightness -1		0x08	0x0e
	Brightness -2		0x18	0x0e
	Brightness -3		0x28	0x0e
	Brightness -4		0x38	0x0e
	***/
	yoko_camera.set_REGISTER(BRIGHTNESS,    0x28);	//定义自动曝光时图片的亮度增益，此值越大，图片越亮
	yoko_camera.set_REGISTER(SIGN_BIT,    	0x0e);	 //定义BRIGHT   的高位是正数，还是负数，如果为0x0e,则图片亮度是负的；0-255，图片越来越亮；0 ~ -127，图片越来越亮

	/***********Contrast 对比度
	Contrast +4			0x30
	Contrast +3			0x2c
	Contrast +2			0x28
	Contrast +1			0x24
	Contrast 0			0x20
	Contrast -1			0x1c
	Contrast -2			0x18
	Contrast -3			0x14
	Contrast -4			0x10
	*****************/
	yoko_camera.set_REGISTER(CONTRAST, 0x30);

	/*****UV 自动调整****/
	yoko_camera.set_REGISTER(UVADJ0,        0x81);		//(0x9e, 0x81)

	/*******************Special effects
	 * Normal		(0xa6, 0x06);(0x60, 0x80);(0x61, 0x80);
	 * B&W			(0xa6, 0x26);(0x60, 0x80);(0x61, 0x80);
	 * Bluish		(0xa6, 0x1e);(0x60, 0xa0);(0x61, 0x40);
	 * Sepia		(0xa6, 0x1e);(0x60, 0x40);(0x61, 0xa0);
	 * Redish		(0xa6, 0x1e);(0x60, 0x80);(0x61, 0xc0);
	 * Greenish		(0xa6, 0x1e);(0x60, 0x60);(0x61, 0x60);
	 * Negative		(0xa6, 0x46);
	 * ********************/
//	 yoko_camera.set_REGISTER(0xa6, 	0x06);
//	 yoko_camera.set_REGISTER(0x60, 	0x80);
//	 yoko_camera.set_REGISTER(0x61, 	0x80);

	 /******************边缘强化处理
	  * (0x90, 0x05);(0x92, 0x03);(0x93, 0x00);(0x91, 0x01)
	  * *****/
	 yoko_camera.set_REGISTER(EDGE1,         0x05);		//(0x90, 0x05)
	 yoko_camera.set_REGISTER(EDGE2,         0x03);		//(0x92, 0x03)
	 yoko_camera.set_REGISTER(EDGE3,         0x00);		//(0x93, 0x00)
	 yoko_camera.set_REGISTER(DNSOFF,        0x01);		//(0x91, 0x01)

	 /************色彩矩阵还原
	  * (0x94, 0xb0)；(0x95, 0x9d)；(0x96, 0x13)；(0x97, 0x16)；(0x98, 0x7b)；(0x99, 0x91)
	  * ****************/
//	yoko_camera.set_REGISTER(MTX1,          0xB0);		//(0x94, 0xb0)
//	yoko_camera.set_REGISTER(MTX2,          0x9D);		//(0x95, 0x9d)
//	yoko_camera.set_REGISTER(MTX3,          0x13);		//(0x96, 0x13)
//	yoko_camera.set_REGISTER(MTX4,          0x16);		//(0x97, 0x16)
//	yoko_camera.set_REGISTER(MTX5,          0x7B);		//(0x98, 0x7b)
//	yoko_camera.set_REGISTER(MTX6,          0x91);		//(0x99, 0x91)

	 /************使能矩阵控制 默认值
	  * (0x9a, 0x1e)；
	  * **********/
//	yoko_camera.set_REGISTER(MTX_CTRL,      0x1E);		//(0x9a, 0x1e)

	/*****************gama参数设置 其中寄存器0x7e~0x8c均是对gama曲线的设定，设定配置手册默认值。*********************/
	 //Gamma
	yoko_camera.set_REGISTER(GAM1,          0x0C);		//(0x7e, 0x0c)
	yoko_camera.set_REGISTER(GAM2,          0x16);		//(0x7f, 0x16)
	yoko_camera.set_REGISTER(GAM3,          0x2A);		//(0x80, 0x2a)
	yoko_camera.set_REGISTER(GAM4,          0x4E);		//(0x81, 0x4e)
	yoko_camera.set_REGISTER(GAM5,          0x61);		//(0x82, 0x61);
	yoko_camera.set_REGISTER(GAM6,          0x6F);		//(0x83, 0x6f)
	yoko_camera.set_REGISTER(GAM7,          0x7B);		//(0x84, 0x7b);
	yoko_camera.set_REGISTER(GAM8,          0x86);		//(0x85, 0x86);
	yoko_camera.set_REGISTER(GAM9,          0x8E);		//(0x86, 0x8e);
	yoko_camera.set_REGISTER(GAM10,         0x97);		//(0x87, 0x97);
	yoko_camera.set_REGISTER(GAM11,         0xA4);		//(0x88, 0xa4);
	yoko_camera.set_REGISTER(GAM12,         0xAF);		//(0x89, 0xaf);
	yoko_camera.set_REGISTER(GAM13,         0xC5);		//(0x8a, 0xc5);
	yoko_camera.set_REGISTER(GAM14,         0xD7);		//(0x8b, 0xd7);
	yoko_camera.set_REGISTER(GAM15,         0xE8);		//(0x8c, 0xe8);

	yoko_camera.set_REGISTER(SLOP,          0x20);		//(0x8d, 0x20);

	/***********透镜矫正
	 * (0x4a, 0x10);(0x49, 0x10);(0x47, 0x08);(0x4b, 0x14);(0x4c, 0x17);(0x46, 0x05);
	***************/
	// Lens Correction, should be tuned with real camera module
	yoko_camera.set_REGISTER(LC_RADI,       0x00);		//(0x4a, 0x10);
	yoko_camera.set_REGISTER(LC_COEF,       0x78);		//(0x49, 0x10);
//	yoko_camera.set_REGISTER(LC_XC,			0x08);		//(0x47, 0x08);
	yoko_camera.set_REGISTER(LC_XC,			0x00);
	yoko_camera.set_REGISTER(LC_YC,			0x00);
	yoko_camera.set_REGISTER(LC_COEFB,      0x78);		//(0x4b, 0x14);
	yoko_camera.set_REGISTER(LC_COEFR,      0x78);		//(0x4c, 0x17);
	yoko_camera.set_REGISTER(LC_CTR,        0x05); 	 	// (0x46, 0x05);

	yoko_camera.set_REGISTER(COM5,        0x65); 	 	// (0x0e, 0x05);
	printf_dbg( "Set register value end\n" );
}
