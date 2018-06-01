/*
 * main.cpp
 *
 *  Created on: Oct 24, 2017
 *      Author: cui
 */


#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <linux/input.h>

#include "camera/Camera.h"
#include "scanner/YokoScanner.h"

#define DBG_EN		1
#if(DBG_EN == 1)
#define printf_dbg(x,arg...) printf("[Main_debug]"x,##arg)
#else
#define printf_dbg(x,arg...)
#endif

#define printf_info(x,arg...) printf("[Main_info]"x,##arg)
#define printf_warn(x,arg...) printf("[Main_warn]"x,##arg)
#define printf_err(x,arg...) printf("[Main_err]"x,##arg)

#define CAMERA_DEVICE "/dev/video0"

#if 1
#define VIDEO_HEIGHT (480)
#define VIDEO_WIDTH (640)
#else
#define VIDEO_HEIGHT (600)
#define VIDEO_WIDTH (800)
#endif

/* 毫秒级 延时 */
void msleep(int ms)
{
    struct timeval delay;
    delay.tv_sec = 0;
    delay.tv_usec = ms * 1000;
    select(0, NULL, NULL, NULL, &delay);
}

int main(int argc, char *argv[])
{
	int ret;
	int transLen;
	int len;
	char * img_data = NULL;
	char QR_code_result[500];

	YokoScanner yoko_scan;
	Camera yoko_camera(640, 480);

	/*CAMERA*/
	printf_dbg("init camera\n");
	ret = yoko_camera.open_dev(CAMERA_DEVICE, false);
	if(ret < 0) {
		printf_err("open camera deviceL%s fail\n",CAMERA_DEVICE);
		return -1;
	}
	ret = yoko_camera.set_input(0);
	if(ret < 0) {
		printf_err("Set Camera input fail\n");
		goto camera_exit;
	}
	ret = yoko_camera.set_format(VIDEO_WIDTH, VIDEO_HEIGHT);
	if(ret < 0) {
		printf_err("Set format fail\n");
		goto camera_exit;
	}
	printf_dbg("\n\nyokocam set format ok\n\n");

	ret = yoko_camera.init_device();
	if(ret < 0) {
		printf_err("init device %s fail\n",CAMERA_DEVICE);
		goto camera_exit;
	}
	printf_dbg("\n\nyokocam init camera ok\n\n");

	ret = yoko_camera.start_capturing();
	if(ret < 0) {
		printf_err("camera streamon fail\n");
		goto camera_exit;
	}

	printf_info("Image capture ok!\n");

	img_data =(char *) malloc (VIDEO_WIDTH * VIDEO_HEIGHT);
	if(!img_data) {
		printf_err("malloc image buffer fail\n");
		goto camera_exit;
	}


//	yoko_scan.set_code_enable(YOKO_CODE_QR, 1);

/********************************** init OK!********************/
	while(1)
	{
//		gettimeofday(&start,NULL);
		ret = yoko_camera.get_imagedata((unsigned char*) img_data, &len);
		if(ret < 0 ) {
			printf_err("Get image data fail\n");
			continue;
		}
//		printf_dbg("get data len :%d\n",len);

		yoko_scan.set_data((unsigned char *)img_data, VIDEO_WIDTH, VIDEO_HEIGHT);

		int ret = yoko_scan.decode();
		if(ret != 0 ) {
			transLen = yoko_scan.getResultLen();
			if(transLen > 0) {
				strncpy( QR_code_result, yoko_scan.result, transLen);
				QR_code_result[transLen] = NULL;
				printf_dbg("QR code result:%s\n", QR_code_result);

				msleep(100);    //350
				continue;
			}
			printf_warn("Code len is 0\n");
		}
	}

camera_exit:
	yoko_camera.stop_capturing();
	yoko_camera.uninit_device();
	yoko_camera.close_dev();


	return 0;
}


