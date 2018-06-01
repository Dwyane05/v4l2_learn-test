#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>  
#include <errno.h>
#include <getopt.h>
#include "Camera.h"
#include "bmp.h"

using namespace std;

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

char CAPTURE_FILE[64] = {0};

static struct option long_options[] = {
	{ "preview", 0, 0, 'P'}, /* 0 */
	{ "capture", 0, 0, 'C'},
	{ "testing", 0, 0, 'T'},
	{ "scale", 0, 0, 'S'},
	{ "verbose", 0, 0, 'V'},
	{ "use_ipu", 0, 0, 'i'},
	{ "preview_width", 1, 0, 'w'},
	{ "preview_height", 1, 0, 'h'}, /* 10 */
	{ "preview_bpp", 1, 0, 'b' },
	{ "capture_width", 1, 0, 'x'},
	{ "capture_height", 1, 0, 'y'},
	{ "capture_bpp", 1, 0, 'z' },
	{ "save_raw", 0, 0, 's'},
	{ "display_raw", 0, 0, 'd'}, /* 15 */
	{ "file", 1, 0, 'f'},
	{ "planar", 0, 0, 'p'},
	{ "only_y", 0, 0, 'g'},	// grey
	{ "format", 1, 0, 't'},
	{ "cim_id", 1, 0, 'I'},
	{ "packing", 1, 0, 'k'},
	{ "macro_block", 0, 0, 'M'},
	{ "virtual_memory", 0, 0, 'v'},
	{"save picture and preview to fb", 0, 0, 'l'},
	{0, 0, 0, 0}
};

static char optstring[] = "hf:n:s";

void usage() {
	printf("\nUsage:\n");
	printf("\n./ingenic_imgcapture [-f filename] [-n imagecount]\n");
	printf("\n./ingenic_imgcapture -s (enum support format)\n");

	printf("\n\nExamples:\n");
	printf("\t./ingenic_imgcapture -f file , save image to file\n");
	printf("\t./ingenic_imgcapture -n 3 , save 3 image file\n");
}

int main(int argc, char *argv[])
{
	int ret;
	int img_cnt = 1;
	int numtest = 0;
	int enum_fmt = 0;
	bool filename =false;
	char *buf = NULL;
	char *y_buf = NULL;
	char file[64] = {0};

	bmp image(VIDEO_WIDTH, VIDEO_HEIGHT);

	Camera yoko_camera(VIDEO_WIDTH, VIDEO_HEIGHT);

	do {
		int c = 0;
		c = getopt_long(argc, argv, optstring, long_options, NULL);
		if (c == -1)
			break;

		switch (c)
		{
			case 'f':
				sprintf(file,  "%s",   optarg);
				filename = true;
				break;

			case 'n':
				img_cnt = atoi(optarg);
				printf_info("save %d images\n", img_cnt);
				break;
			
			case 's':
				enum_fmt = 1;
				break;

			case 'h':
				usage();
				return 0;

			default:
				usage();
				return -1;
		}
	} while (1);


	/*CAMERA*/
	printf_dbg("init camera\n");
	ret = yoko_camera.open_dev(CAMERA_DEVICE, true);
	if(ret < 0) {
		printf_err("open camera device %s fail\n",CAMERA_DEVICE);
		return -1;
	}

	printf_dbg("\n\nyokocam open camera ok\n\n");
	
	yoko_camera.enum_format();
	
	printf("\n\nyokocam enum support format ok\n\n");
	if(enum_fmt == 1) {
		yoko_camera.close_dev();
		return 0;
	}

	ret = yoko_camera.set_format(VIDEO_WIDTH, VIDEO_HEIGHT);
	if(ret < 0) {
		printf_err("Set format fail\n");
		goto camera_exit;
	}

	printf("\n\nyokocam set format ok\n\n");


	ret = yoko_camera.init_device();
	if(ret < 0) {
		printf_err("init device %s fail\n",CAMERA_DEVICE);
		goto camera_exit;
	}

	printf("\n\nyokocam init camera ok\n\n");

	ret = yoko_camera.start_capturing();
	if(ret < 0) {
		printf_err("camera streamon fail\n");
		yoko_camera.uninit_device();
		goto camera_exit;
	}

	printf_info("Image capture init ok!\n");

	buf = (char *) malloc(VIDEO_HEIGHT * VIDEO_WIDTH*2);
	if(!buf) {
		goto camera_exit;
	}

	y_buf = (char *) malloc(VIDEO_HEIGHT * VIDEO_WIDTH);
	if(!y_buf) {
		goto camera_exit;
	}

/********************************** init OK!********************/
	for(int i = 1; i <= img_cnt ; i++)
	{
		int len = 0;
		int y_len = 0;

		ret = yoko_camera.get_imagedata((unsigned char*) buf, &len);
		if(ret < 0 ) {
			printf_err("Get image data fail\n");
			continue;
		}
		printf_dbg("get data len :%d\n",len);

		if(!filename) {
			sprintf(CAPTURE_FILE,  "data-color-%d.bmp",   numtest);
			printf("write image name: %s\n", CAPTURE_FILE);
			image.save_color(CAPTURE_FILE, (unsigned char*) buf);
		} else {
			sprintf(CAPTURE_FILE, "%s-color-%d.bmp", file, numtest);
			printf("write image name: %s\n", CAPTURE_FILE);
			image.save_color(CAPTURE_FILE, (unsigned char*) buf);
		}

		ret = yoko_camera.get_y_component((unsigned char*) buf,(unsigned char*)y_buf, &y_len );
		if(ret < 0 ) {
			printf_err("Get image data fail\n");
			continue;
		}
		printf_dbg("get y_data len :%d\n",y_len);

		if(!filename) {
			sprintf(CAPTURE_FILE,  "data-y-%d.bmp",   numtest);
			printf("write image name: %s\n", CAPTURE_FILE);
			image.save_black_white(CAPTURE_FILE, (unsigned char*) y_buf);
		} else {
			sprintf(CAPTURE_FILE, "%s-y-%d.bmp", file, numtest);
			printf("write image name: %s\n", CAPTURE_FILE);
			image.save_black_white(CAPTURE_FILE, (unsigned char*) y_buf);
		}

		numtest++;
	}

	free(buf);
	free(y_buf);
camera_exit:
	yoko_camera.stop_capturing();
	yoko_camera.uninit_device();
	yoko_camera.close_dev();

	return 0;
}
