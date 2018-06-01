/*
 * camera.h
 *
 *  Created on: 5 Jul 2016
 *      Author: xiao
 */

#ifndef CAMERA_H_
#define CAMERA_H_

#include <linux/videodev2.h>
//#include "preview_display.h"

#define BUFFER_MAXNUM 	3

typedef struct VideoBuffer {
    void   *start; 	//视频缓冲区的起始地址
    int  length;	//缓冲区的长度
} VideoBuffer;


typedef enum {
	IO_METHOD_READ = 0,
	IO_METHOD_MMAP,
	IO_METHOD_USERPTR,
} io_method;


class Camera {
public:
	Camera();
	Camera(int iwidth, int iheight);
	~Camera();

	void param_reset();

	int open_dev(char const *dev_name, bool block);
	int close_dev();

	unsigned int set_format(unsigned int video_width, unsigned int video_height);
//	unsigned int set_format(struct display_info *disp);

	int start_capturing(void);
	int stop_capturing(void);

	void get_format(int *video_width, int *video_height);
	void enum_format(void);
	bool is_streaming();
	int get_bufsize();

	int set_input(unsigned int input_num);
	//	int get_input();
public:
//	int init_device(struct display_info *disp);
	int init_device();
	void uninit_device(void);
private:
	void dump_format(struct v4l2_format *fmt);
	void query_cap(struct v4l2_capability *cap);
	int init_mmap(int buffer_num);
	int exit_mmap();

	void init_userp(unsigned int buffer_size);
	void init_read(unsigned int buffer_size);


public:
	int get_imagedata(unsigned char *buffer, int * data_len);
private:
	int CaptureGetFrame(int *index);
	void CaptureReleaseFrame(int index);
	int WaitCamerReady(unsigned int second);



private:
	int fd;
	unsigned int width;
	unsigned int height;

	bool streaming;
    VideoBuffer buffers[BUFFER_MAXNUM];

	io_method    io;//      = IO_METHOD_MMAP;
	unsigned int n_buffers;

	int xioctl(int fd, int request, void *arg);




//#define CAMERA_CTRLS
#ifdef CAMERA_CTRLS
	struct Ctrl {
		int            minimum;   /* Note signedness */
		int            maximum;
		int            step;
		int            default_value;
	};

	int v4l2setCaptureParams(bool);


	int query_contrast(struct Ctrl *ctrl);
	int set_contrast(int value);
	int get_contrast(int *value);

	int query_exposure(struct Ctrl *ctrl);
	int set_exposure(int value);
	int get_exposure(int *value);

	int set_exposure_auto(int value);
	int get_exposure_auto(int *value);

	int query_EdgeEnch(struct Ctrl *ctrl);
	int get_EdgeEnch(int *value);
	int set_EdgeEnch(int value);
#endif

};

#endif /* CAMERA_H_ */
