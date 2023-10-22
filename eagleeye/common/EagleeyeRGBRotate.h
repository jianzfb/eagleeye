#ifndef _EAGLEEYE_RGB_ROTATE_H_
#define _EAGLEEYE_RGB_ROTATE_H_
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/Array.h"

namespace eagleeye{
void bgr_rotate_hwc(const unsigned char* src, unsigned char* dst, int w_in, int h_in, int angle);
void rgb_to_bgr_convert(unsigned char* rgb_data, int width, int height, unsigned char* bgr_data);
}
#endif