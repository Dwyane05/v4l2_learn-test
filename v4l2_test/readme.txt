参考：http://blog.csdn.net/wenrenhua08/article/details/40044495
http://blog.csdn.net/zhangdaxia2/article/details/72763847
LinuxTV官网capture.c文件解析：

main( int argc, char *argv[] )
{
获取命令行参数-执行选项
		|
		|
打开设备open_device()
		|		|--stat文件状态，是否是字符设备
		|		|--打开dev_name设备=全局变量fd总
		|
初始化设备init_device()
		|		|--查询设备能力容量VIDIOC_QUERYCAP
		|		|--确认是否支持单平面视频捕捉，是否支持streaming io
		|		|--判断是否支持裁剪，如果支持设置为默认配置V4L2_BUF_TYPE_VIDEO_CAPTURE
		|		|--是否强制设置format，如果强制格式为640x480，V4L2_PIX_FMT_YUYV，V4L2_FIELD_INTERLACED
		|		|--		or查询格式---驱动偏执狂
		|		|--初始化内存映射init_mmap();
		|		|		--VIDIOC_REQBUFS，使用mmap方式，由环形缓冲区，rep.count=4;设置相应类型和方式
		|		|		  判断如果获得buffer数量是否小于2；
		|		|		--calloc相应数量的用户缓冲区结构体；结构体包含：起始地址，长度；判断是否成功
		|		|		  使用VIDIOC_QUERYBUF将申请的count个v4l2_buffer放入环形缓冲区，查询
		|		|		  每个v4l2_buffer状态，将其地址和长度放入结构体数组buffers中。
		|
		|
开始捕获start_capturing（）
		|		|--循环VIDIOC_QBUF--将指定的Buffer放到输入队列中，即向Device表明这个Buffer可以存放东西
		|		|--VIDIOC_STREAMON 开始流I/O--Start or stop streaming I/O
		|
		|
进入主循环mainloop（）
		|		|--使用全局变量frame_count确定帧数，while确定采集帧数；
		|		|    for()循环中使用select两秒--读帧if (read_frame()) break;
		|		|		--读帧read_frame()  VIDIOC_DQBUF从输出队列取出buffer，判断索引号，
		|		|   		处理图像process_image（）
		|		|				--图像处理(buffers[buf.index].start, buf.bytesused)
		|		|				  暂无处理-----
		|		|			VIDIOC_QBUF--处理完图像，重新将buffer放入缓冲区	  
		|
		|
停止捕获stop_capturing
		|		|--VIDIOC_STREAMOFF关闭流
		|
		|
设备去初始化uninit_device（）
		|		|--通过ioctl申请的内存，是物理内存，无法被交换入Disk，所以一定要释放：munmap()
		|		|	使用内存是在内核空间的物理内存，不用了必须用munmap释放
		|
		|
关闭设备close_device
		
		
		
程序只是捕捉到数据，并未处理。如果需要保存成bmp格式的图片，需要将yuyv格式数据转换成rb格式，然后在保存成bmp图片
