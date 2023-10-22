#ifndef _EAGLEEYE_YUV_H_
#define _EAGLEEYE_YUV_H_
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/Array.h"

namespace eagleeye
{
// for android    
void I420_rotate_90(unsigned char* i420_data, int width, int height, unsigned char* rotated_i420_data);
void I420_rotate_180(unsigned char* i420_data, int width, int height, unsigned char* rotated_i420_data);
void I420_rotate_270(unsigned char* i420_data, int width, int height, unsigned char* rotated_i420_data);

void Android420_to_I420(const uint8_t* src_android_420_data, uint8_t* dst_i420_data, int width, int height, int degree=0);

Matrix<Array<unsigned char,3>> I420_to_RGB(unsigned char* i420_data, int width, int height, unsigned char* rgb_data_ptr=NULL);
Matrix<Array<unsigned char,3>> I420_to_BGR(unsigned char* i420_data, int width, int height, unsigned char* bgr_data_ptr=NULL);

Matrix<Array<unsigned char,3>> NV21_to_RGB(unsigned char* i420_data, int width, int height, unsigned char* rgb_data_ptr=NULL);
Matrix<Array<unsigned char,3>> NV21_to_BGR(unsigned char* i420_data, int width, int height, unsigned char* bgr_data_ptr=NULL);

Matrix<Array<unsigned char,3>> NV12_to_RGB(unsigned char* i420_data, int width, int height, unsigned char* rgb_data_ptr=NULL);
Matrix<Array<unsigned char,3>> NV12_to_BGR(unsigned char* i420_data, int width, int height, unsigned char* bgr_data_ptr=NULL);
} // namespace eagleeye

#endif