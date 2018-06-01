#include "YokoScanner.h"
#include <string.h>

using namespace std;
#define DBG_EN		0
#if(DBG_EN == 1)
#define printf_dbg(x,arg...) printf("[Yokoscan_debug]"x,##arg)
#else
#define printf_dbg(x,arg...)
#endif

#define printf_info(x,arg...) printf("[Yokoscan_info]"x,##arg)
#define printf_warn(x,arg...) printf("[Yokoscan_warn]"x,##arg)
#define printf_err(x,arg...) printf("[Yokoscan_err]"x,##arg)


YokoScanner::YokoScanner()
{
   scanner = zbar_image_scanner_create();
}

YokoScanner::~YokoScanner()
{
}

void YokoScanner::set_data(unsigned char* data_,int width_,int height_){
     data=data_;
     width=width_;
     height=height_;
}


int YokoScanner::decode()
{
        zbar_image_t *image = zbar_image_create();
        zbar_image_set_size(image, width, height);
        zbar_image_set_data(image, data, width * height);
      
	int n = zbar_scan_image(scanner, image);
	resultLen=0;
        
        if(n>0){
            resultLen = get_result_len(scanner);
            memcpy(result,get_results(scanner),resultLen); 
        }
        free(image);
        return n;
}



int YokoScanner::getResultLen()
{
    return resultLen;
}
