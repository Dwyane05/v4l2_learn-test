#ifndef YOKOSCANER_H
#define YOKOSCANER_H
#include <zbar.h>
#include "code.h"
#include <string>
using namespace std;
using namespace zbar;

class YokoScanner
{
public:
     YokoScanner();
    ~YokoScanner();
    
    int decode();
    char result[500];
    int getResultLen();
    void set_data(unsigned char* data,int width,int height);
private:
    zbar_image_scanner_t *scanner;
    unsigned char* data;
    int width;
    int height;
	
    int resultLen;

    int dm_enable;
    int qr_enable;
    int ean_enable;
	int i25_enable;
	int code39_enable;
	int code128_enable;
	int pdf417_enable;
};

#endif //MATIMAGE_H
