/*
 * Camera.cpp
 *
 *  Created on: 5 Jul 2016
 *      Author: xiao
 */
#include "camera.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <getopt.h>	//getopt_long()
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>	//for videodev2.h


//#if 0
//#define DBG()	fprintf(stdout, "**>%s L%d\n", __func__, __LINE__)
//#else
//#define DBG()	do {} while(0)
//#endif

#define DBG_EN  1
#if (DBG_EN == 1)
#define printf_dbg(x,arg...) printf("[Camera_debug]"x,##arg)
#define printf_dbg_full(x,arg...) printf("[Camera_debug] %s line[%d]"x,, __func__, __LINE__,##arg)
#else
#define printf_dbg(x,arg...)
#define printf_dbg_full(x,arg...)
#endif

#define printf_info(x,arg...) printf("[Camera_info]"x,##arg)
#define printf_warn(x,arg...) printf("[Camera_warn]"x,##arg)
#define printf_err(x,arg...) printf("[Camera_err]"x,##arg)


#define CLEAR(x)	memset(&(x), 0, sizeof(x))

const char *io_method_str[3] = {
	"IO_METHOD_READ",
	"IO_METHOD_MMAP",
	"IO_METHOD_USERPTR",
};

Camera::Camera(int iwidth, int iheight)
{
	width = iwidth;
	height = iheight;
	param_reset();
}

Camera::Camera()
{
	width = 0;
	height = 0;
	param_reset();
}

void Camera::param_reset()
{
	fd = -1;
	n_buffers = 0;
	streaming = false;
	io  = IO_METHOD_MMAP;

	for(int i = 0; i < BUFFER_MAXNUM; i++)
	{
		buffers[i].start = NULL;
		buffers[i].length = 0;
	}
}

Camera::~Camera()
{
//	close_dev();
}

int Camera::xioctl(int fd, int request, void *arg)
{
	int r;

	do {
		r = ioctl(fd, request, arg);
	} while (-1 == r && EINTR == errno);

	return r;
}


int Camera::open_dev(char const *dev_name, bool block)
{
	struct stat st;

	if (-1 == stat(dev_name, &st)) {
		printf_err("Cannot identify '%s':%d, %s\n",\
				dev_name, errno, strerror(errno));
		return -1;
	}

	if (!S_ISCHR(st.st_mode)) {
		printf_err("%s is not char device\n", dev_name);
		return -1;
	}

	if(block == true) {
	    fd = open(dev_name, O_RDWR);
	} else {
	    fd = open(dev_name, O_RDWR | O_NONBLOCK);
	}
    if (fd < 0) {
        printf_err("Open Device %s failed\n", dev_name);
        return -1;
    }

    printf_dbg("Open Camera Device %s OK, fd -> %d\n", dev_name, fd);

//	return 0;

#if 1
	// 获取驱动信息
	struct v4l2_capability cap;
	int ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
	if (ret < 0) {
		printf_err("VIDIOC_QUERYCAP failed (%d)\n", ret);
		return -1;
	}

	printf_dbg("Driver Name: %s\nCard Name: %s\nBus info: %s\nDriver Version: %u.%u.%u\n", \
			cap.driver, cap.card, cap.bus_info, \
			(cap.version >> 16) & 0XFF, \
			(cap.version >> 8) & 0XFF,  \
			cap.version & 0XFF);


	if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		printf_err("Device Is not a video capture device\n");
		goto END_ERROR;
	}

	if ((cap.capabilities & V4L2_CAP_STREAMING) == 0) {
		printf_err("Capture device does not support streaming i/o\n");
		goto END_ERROR;
	}

	return 0;

END_ERROR:
	if (fd != -1) {
		close(fd);
		fd = -1;
	}
	return -1;
#endif
}

void Camera::query_cap(struct v4l2_capability *cap)
{
	if (cap->capabilities & V4L2_CAP_VIDEO_CAPTURE)
		printf_info("Video capture device\n");

	if (cap->capabilities & V4L2_CAP_READWRITE)
		printf_info("Read/Write systemcalls\n");

	if (cap->capabilities & V4L2_CAP_STREAMING)
		printf_info("Streaming I/O ioctls\n");
}

int Camera::close_dev()
{
	if(streaming) {
		printf_err("Camera is streaming\n");
		return -1;
	}

	if(fd < 0) {
		printf_warn("Camera is not opened, fd -> %d\n", fd);
		return 0;
	}

	close(fd);
	fd = -1;
	return 0;
}

void Camera::enum_format(void)
{
	int ret = 0;

	for(int i = 0;  ; i++)
	{
		struct v4l2_fmtdesc fmtdes;
		memset(&fmtdes, 0, sizeof(fmtdes));
		fmtdes.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		fmtdes.index = i;
		ret = ioctl(fd, VIDIOC_ENUM_FMT, &fmtdes);
		if(ret < 0) {
			printf_warn("enum fmt error\n");
			break;
		}

		printf_info("{ pixelformat : '%c%c%c%c', description : '%s' }\n", \
			fmtdes.pixelformat & 0xFF, \
			(fmtdes.pixelformat >> 8) & 0xFF, \
			(fmtdes.pixelformat >> 16) & 0xFF, \
			(fmtdes.pixelformat >> 24) & 0xFF,  \
			fmtdes.description);
	}
}

void Camera::dump_format(struct v4l2_format *fmt)
{
	if (fmt->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
		printf_info("width=%d, height=%d\n", \
				fmt->fmt.pix.width, fmt->fmt.pix.height);

		printf_info("pixelformat=%s, field=%d\n", \
				(char *)&fmt->fmt.pix.pixelformat, fmt->fmt.pix.field);

		printf_info("bytesperline=%d, sizeimage=%d\n", \
				fmt->fmt.pix.bytesperline, fmt->fmt.pix.sizeimage);

		printf_info("colorspace=%d\n", fmt->fmt.pix.colorspace);
	}
}

//unsigned int Camera::set_format(struct display_info *disp)
unsigned int Camera::set_format(unsigned int video_width, unsigned int video_height)
{
	struct v4l2_format fmt;
	unsigned int min;
	int ret = -1;

	if(streaming) {
		return -1;
	}

	CLEAR(fmt);

	fmt.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = video_width;
	fmt.fmt.pix.height      = video_height;
	fmt.fmt.pix.pixelformat	= V4L2_PIX_FMT_YUYV;
//	fmt.fmt.pix.pixelformat	= V4L2_PIX_FMT_YUV420;
	fmt.fmt.pix.field	= V4L2_FIELD_NONE;	//V4L2_FIELD_INTERLACED;
//	fmt.fmt.pix.width	= disp->width;
//	fmt.fmt.pix.height	= disp->height;
//	fmt.fmt.pix.priv	= disp->fmt.fmt_priv;
//	fmt.fmt.pix.pixelformat	= disp->fmt.fourcc;

	if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt)) {
	    printf_err("==>%s L%d: set error  !!!!!!!!!!!!! \n",__func__, __LINE__);
	    printf_err("Set format failed (%d)\n", ret);
	   	return ret;
     }

    xioctl(fd, VIDIOC_G_FMT, &fmt);

//	if((disp->width != fmt.fmt.pix.width) ||
//			(disp->height != fmt.fmt.pix.height)) {
//		disp->width = fmt.fmt.pix.width;
//		disp->height = fmt.fmt.pix.height;
//	}
	if((video_width != fmt.fmt.pix.width) || \
			(video_height != fmt.fmt.pix.height)) {
		width = fmt.fmt.pix.width;
		height = fmt.fmt.pix.height;
	}
	//	printf_dbg("Set format OK. width:%d, height:%d\n",width, height);


	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;

	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;

#if DBG_EN
	dump_format(&fmt);
#endif

	printf_dbg("==>%s L%d: sizeimage=0x%x\n fmt.fmt.pix.width = 0x%x\n fmt.fmt.pix.height = 0x%x\n", \
			__func__, __LINE__, fmt.fmt.pix.sizeimage,fmt.fmt.pix.width, fmt.fmt.pix.height);


	return fmt.fmt.pix.sizeimage;
}

void Camera::get_format(int *video_width, int *video_height)
{
	if(!video_width || !video_height)
	{
		return;
	}

	*video_width = width;
	*video_height = height ;
}

int Camera::get_bufsize()
{
	return 	width * height ;
}

bool Camera::is_streaming()
{
	return streaming;
}

int Camera::init_mmap(int buffer_num)
{
	struct v4l2_requestbuffers req;
	int ret = -1;

	if(streaming) {
		return -1;
	}

	if(BUFFER_MAXNUM < buffer_num) {
		printf_err("Request too large number of buffer\n");
		return -EINVAL;
	}

	CLEAR(req);
//	req.count	= BUFFER_MAXNUM;
	req.count = buffer_num;
	req.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory	= V4L2_MEMORY_MMAP;

    //请求分配内存,采取方式为MMAP映射
	if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
//			fprintf(stderr, "%s does not support memory mapping\n", dev_name);
			exit(EXIT_FAILURE);
		} else {
			printf_err("VIDIOC_REQBUFS failed (%d)\n", ret);
			return ret;
		}
	}

	if (req.count < 1) {
//		fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);
		exit(EXIT_FAILURE);
	}

	printf_dbg("requested %d buffer\n", req.count);
//    buf_count =  req.count;

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		struct v4l2_buffer buf;

		CLEAR(buf);
		buf.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory	= V4L2_MEMORY_MMAP;
		buf.index	= n_buffers;
		if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf)) {
			printf_err("VIDIOC_QUERYBUF (%d) failed (%d)\n", n_buffers, ret);
	        return -1;
	   }

		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start = (char *) mmap(NULL, buf.length, \
				PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

		if (MAP_FAILED == buffers[n_buffers].start) {
			printf_err("mmap (%d) failed: %s\n", n_buffers, strerror(errno));
			return -1;
		}

        printf_dbg("Queue Frame buffer no.%d: address = 0x%p, length = %d\n",
        		buf.index, buffers[n_buffers].start, buffers[n_buffers].length);
	}

    return 0;
}


int Camera::exit_mmap()
{
	int ret;

    if(streaming)
    	return -1;

	for (unsigned int i = 0; i < n_buffers; i++)
	{
		struct v4l2_buffer buf;
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		ret = ioctl(fd, VIDIOC_DQBUF, &buf);
		if (ret < 0) {
			printf_err("%s DQBUF number buffer fail!\n", __func__);
			continue;
		}
       printf_dbg("Dqueue Frame buffer no.%d: address = %p, length = %d\n",
    		   buf.index, buffers[buf.index].start, buffers[buf.index].length);
	}

	for (unsigned int i = 0; i < n_buffers; i++)
	{
		if(buffers[i].start != NULL) {
			ret = munmap(buffers[i].start, buffers[i].length );
			if(ret < 0) {
				printf_err("mumap address:%p error(%d)", buffers[i].start, ret);
			} else {
				buffers[i].start = NULL;
				buffers[i].length = 0;
			}
		}

		printf_dbg("Munmap Frame buffer no.%d: address = %p, length = %d\n",
				i, buffers[i].start, buffers[i].length);
	}

	n_buffers = 0;
    return 0;
}

#if 1
void Camera::init_userp(unsigned int buffer_size)
{

}
#else
void Camera::init_userp(unsigned int buffer_size)
{
	struct v4l2_requestbuffers req;
	unsigned int page_size = 0;

	page_size = getpagesize();
	buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);

	CLEAR(req);

	req.count	= 1;
	req.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory	= V4L2_MEMORY_USERPTR;

	if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
//			fprintf(stderr, "%s does not support user pointer i/o\n", dev_name);
			exit(EXIT_FAILURE);
		} else
			errno_exit("VIDIOC_REQBUFS");
	}

//	buffers = calloc(req.count, sizeof(*buffers));
//	if (!buffers) {
//		fprintf(stderr, "Out of memory\n");
//		exit(EXIT_FAILURE);
//	}

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		buffers[n_buffers].length = buffer_size;
		buffers[n_buffers].start = (unsigned char *)JZMalloc(128, buffer_size);

		printf("==>%s L%d: the buffers[%d].start = %x\n", __func__, __LINE__ , n_buffers,buffers[n_buffers].start);
		if (!buffers[n_buffers].start) {
			fprintf(stderr, "Out of memory\n");
			exit(EXIT_FAILURE);
		}
	}
}
#endif

void Camera::init_read(unsigned int buffer_size)
{
#if 0
	buffers = calloc(1, sizeof(*buffers));

	if (!buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	buffers[0].length = buffer_size;
	buffers[0].start = malloc(buffer_size);

	if (!buffers[0].start) {
		fprintf(stderr, "Out of memory2\n");
		exit(EXIT_FAILURE);
	}
#endif
}

//int Camera::init_device(struct display_info *disp)
int Camera::init_device()
{
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
//	unsigned int frmsize;

//	if (disp->userptr == 1)
//		io = IO_METHOD_USERPTR;		// user pointer I/O
//	else
//		io = IO_METHOD_MMAP;		// map device memory to userspace

	printf_dbg("==>%s L%d: io method: %s\n", __func__, __LINE__, io_method_str[io]);

	if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			printf_err("Camera is not V4L2 device\n");
			return -1;
		} else {
			printf_err("VIDIOC_QUERYCAP error'n");
			return -1;
		}
	}

#if DBG_EN
	query_cap(&cap);
#endif
	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		printf_err("Camera is not capture device\n");
		return -1;
	}

	switch (io) {
	case IO_METHOD_READ:
#if 0
		if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
			fprintf(stderr, "%s doesn't support read i/o\n", dev_name);
			exit(EXIT_FAILURE);
		}
#endif
		break;
	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
			printf_err("Camera doesn't support streaming i/o\n");
			return -1;
		}
		break;
	}

	/* select video input, video standard and tune here. */

	CLEAR(cropcap);

	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect;	/* reset to default */

		if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
			switch(errno) {
			case EINVAL:
				/* Cropping not supported */
				break;
			default:
				/* Errors ignored */
				break;
			}
		}
	} else {
		/* Errors ignored */
	}

//	frmsize = set_format(width, height);

	switch (io) {
	case IO_METHOD_MMAP:
		init_mmap(1);
		break;
#if 0
	case IO_METHOD_READ:
		init_read(frmsize);
		break;

	case IO_METHOD_USERPTR:
		init_userp(frmsize);
		break;
#endif
	default:
		printf_err("init buffer type fail\n");
		break;
	}

	return 0;
}


void Camera::uninit_device(void)
{
	unsigned int i;

	printf_dbg("current device fd -> %d\n", fd);
	switch(io) {
	case IO_METHOD_MMAP:
		for (i=0; i < n_buffers; ++i)
		{
			if (-1 == munmap(buffers[i].start, buffers[i].length)) {
				printf_err("munmap fail\n");
			}
		}

		break;
#if 0
	case IO_METHOD_READ:
		free(buffers[0].start);
		break;

	case IO_METHOD_USERPTR:
		for (i=0; i<n_buffers; ++i)
		{
//			printf("uninit_device i = %d \n  n_buffers = %d buffers[%d].start = 0x%x\n", i , n_buffers, i, buffers[i].start);
//	                jz47_free_alloc_mem();
		}
		break;
#endif
	default:
		printf_err("uninit buffer type fail\n");
		break;
	}

//	free(buffers);
}


int  Camera::start_capturing(void)
{
	unsigned int i;
	int ret =-1;
	enum v4l2_buf_type type;

	switch(io) {
	case IO_METHOD_READ:
		/* nothing to do */
		break;

	case IO_METHOD_MMAP:
		for (i=0; i < n_buffers; ++i) {
			struct v4l2_buffer buf;

			CLEAR(buf);

			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index = i;

			if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
				printf_err("%s VIDIOC_QBUF fail\n", __func__);
			}

		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl(fd, VIDIOC_STREAMON, &type)) {
			printf_err("VIDIOC_STREAMON failed (%d)\n", ret);
			return ret;
		}

		streaming = true;
		break;

	case IO_METHOD_USERPTR:
		for (i=0; i < n_buffers; ++i) {
			struct v4l2_buffer buf;

			CLEAR(buf);

			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_USERPTR;
			buf.index = i;
			buf.m.userptr = (unsigned long)buffers[i].start;
			buf.length = buffers[i].length;

//			memset((void *)buf.m.userptr, 0xff, buf.length);	//force OS to allocate physical memory
//			if (-1 == xioctl(fd, VIDIOC_CIM_S_MEM, &buf))		//by ylyuan
//				errno_exit("VIDIOC_CIM_S_MEM");

			if (-1 == xioctl(fd, VIDIOC_QBUF, &buf)){
				printf_err("VIDIOC_QBUF (%d) failed (%d)\n", i, -1);
				return -1;
			}

		}

		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (-1 == xioctl(fd, VIDIOC_STREAMON, &type)){
			printf_err("VIDIOC_STREAMON failed (%d)\n", -1);
			return -1;
		}

		streaming = true;
		break;
	}
	return 0;
}

int Camera::stop_capturing(void)
{
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	int ret = -1;

	printf_dbg("stream on!\n");

	switch (io) {
	case IO_METHOD_READ:
		/* nothing to do */
		break;
	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type)) {
			   printf_err("VIDIOC_STREAMOFF failed (%d)\n", ret);
			   return ret;
		}

		streaming = false;
		break;
	}

	return 0;
}


int Camera::WaitCamerReady(unsigned int second)
{
	fd_set fds;
	struct timeval tv;
	int r;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	/* Timeout */
	tv.tv_sec  = second;
	tv.tv_usec = 0;

	r = select(fd + 1, &fds, NULL, NULL, &tv);
	if (r == -1)
	{
		printf_err("select err\n");
		return -1;
	}
	else if (r == 0)
	{
		printf_err("select timeout\n");
		return -1;
	}

	return 0;
}

int Camera::CaptureGetFrame(int *index)
{
	int ret = -1;
	struct v4l2_buffer buf;

	switch (io) {
#if 0
	case IO_METHOD_READ:
		if (-1 == read(fd, buffers[0].start, buffers[0].length)) {
			switch (errno) {
			case EAGAIN:
				return 0;
			case EIO:
			default:
				errno_exit("read");
			}
		}
//		*frame = (void *)buffers[0].start;
		break;
#endif
	case IO_METHOD_MMAP:

		ret = WaitCamerReady(2);
		if (ret != 0) {
			printf_err("wait time out\n");
			return ret;
		}

		CLEAR(buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;

		if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
			case EAGAIN:
				return 0;
			case EIO:
			default:
				*index = -1;
				printf_err("%s: VIDIOC_DQBUF Failed\n",__func__);
				return ret;
			}
		}

		printf_dbg("get data, width:%d, height:%d\n",width, height);

		*index = buf.index;

		assert(buf.index < n_buffers);

//      if(frame_handler) {
//	    	frame_handler((void *)buffers[buf.index].start);
//		}
//
//		if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
//			errno_exit("VIDIOC_QBUF");

		break;
#if 0
	case IO_METHOD_USERPTR:
		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;

		if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
			case EAGAIN:
				return 0;
			case EIO:
			default:
				errno_exit("VIDIOC_DQBUF");
			}
		}
	        int page_size = getpagesize();
	        int buf_size = (buf.length  + page_size - 1) & ~(page_size - 1);
		for (i=0; i<n_buffers; ++i) {
			if (buf.m.userptr == (unsigned long)buffers[i].start
			 && buf_size == buffers[i].length){
				break;
			}
		}

//		*frame = (void *)buf.m.userptr;

                if(frame_handler) {
	                    frame_handler((void *)buffers[buf.index].start);
		}

		if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
			errno_exit("VIDIOC_QBUF");

		break;
#endif
	default:
		printf_err("error io\n");
		break;
	}

	return 0;
}

void Camera::CaptureReleaseFrame(int index)
{
	int ret = -1;
	struct v4l2_buffer buf;

	if(index < 0)
		return;

	memset(&buf, 0, sizeof(v4l2_buffer));
	buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
	buf.index = index;

	printf_dbg("ReleaseFrame, buf.index: %d\n", buf.index);

    ret = ioctl(fd, VIDIOC_QBUF, &buf);
    if (ret != 0)
	{
    	printf_err("%s: VIDIOC_QBUF Failed: index = %d, ret = %d, %s\n", \
    			__func__, \
				buf.index,  \
				ret, strerror(errno));
    }
}

int Camera::get_imagedata(unsigned char *buffer, int * data_len)
{
	int ret = -1;
	int index;

	if(!buffer || !data_len)
		return -EINVAL;

	if(!streaming) {
		printf_err("Camera is not streaming\n");
		return -EBUSY;
	}
    CaptureGetFrame(&index);
    CaptureReleaseFrame(index);

	ret = CaptureGetFrame(&index);
	if(ret < 0) {
		printf_err("get frame fail\n");
		return ret;
	}

	*data_len = 0;
	char *imp = (char *) buffers[index].start;
	int z = 0;
	for (unsigned int y = 0; y < height; y++)
	{
		unsigned int offset = y * width * 2;
		if(offset >= buffers[index].length) {
			printf_err("get image data out of mem\n");
			break;
		}

//		printf_dbg("y:%d ,add: %p offset %d\n",  y, imp, offset);

		z = 0;
		int off = (*data_len);
		for (unsigned int x = 0; x < width; x++)
		{
		
//			printf_dbg("x:%d y:%d offset %d\n", x, y, offset);
			buffer[ off + x] = imp[offset + z];
			(*data_len)++;
			z+=2;
		}
	}

	CaptureReleaseFrame(index);

	printf_dbg("data len : %d\n",*data_len);

	return 0;
}

int Camera::set_input(unsigned int input_num)
{

	if(streaming) {
		return -1;
	}

	printf_dbg("Set input to %d\n", input_num);

	if (-1 == ioctl (fd, VIDIOC_S_INPUT, &input_num)) {	//设置输入index
		printf_err("VIDIOC_S_INPUT error!, %d\n", input_num);
		return -1;
	}
#if 0
	struct v4l2_input input;
	memset(&input, 0, sizeof(input));
	input.index = input_num;
	if (-1 == ioctl(fd, VIDIOC_ENUMINPUT, &input)) {
	    printf_err("Can not enum input device\n");
	    return -1;
	} else {
		if (input.type != V4L2_INPUT_TYPE_CAMERA) {
			printf_err("input[%d] type is not V4L2_INPUT_TYPE_CAMERA!\n",input_num);
			return -1;
		}
	}
#endif

	return 0;
}

#if 0
int Camera::get_input()
{
	//测试输入输出
	int index = -1;

	if (-1 == ioctl(fd, VIDIOC_G_INPUT, &index)) {
	    printf_err("Can not get input device\n");
	    return -1;
	}

	return index;
}
#endif

#ifdef CAMERA_CTRLS
int Camera::v4l2setCaptureParams(bool mTakingPicture)
{
	int ret = -1;
	struct v4l2_streamparm params;

	// set capture mode
	memset(&params, 0 ,sizeof(params));
  	params.parm.capture.timeperframe.numerator = 1;
	params.parm.capture.timeperframe.denominator = 30;
	params.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (mTakingPicture)	{
		printf_dbg("Setting video capture image mode\n");
		params.parm.capture.capturemode = V4L2_MODE_IMAGE;
	} else {
		printf_dbg("Setting video capture video mode\n");
		params.parm.capture.capturemode = V4L2_MODE_VIDEO;
	}

	ret = ioctl(fd, VIDIOC_S_PARM, &params);
	if (ret < 0) {
		printf_err("v4l2setCaptureParams failed! %d\n",ret);
		return -1;
	}

	printf_dbg("v4l2setCaptureParams ok\n");

	return 0;
}
#endif
