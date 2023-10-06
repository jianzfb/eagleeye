#ifndef _EAGLEEYE_YUV_H_
#define _EAGLEEYE_YUV_H_
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/Array.h"

namespace eagleeye
{
// for android    
void eagleeye_I420_rotate_90(unsigned char* i420_data, int width, int height, unsigned char* rotated_i420_data);
void eagleeye_I420_rotate_180(unsigned char* i420_data, int width, int height, unsigned char* rotated_i420_data);
void eagleeye_I420_rotate_270(unsigned char* i420_data, int width, int height, unsigned char* rotated_i420_data);

Matrix<Array<unsigned char,3>> eagleeye_I420_to_RGB(unsigned char* i420_data, int width, int height, unsigned char* rgb_data_ptr=NULL);
Matrix<Array<unsigned char,3>> eagleeye_I420_to_BGR(unsigned char* i420_data, int width, int height, unsigned char* bgr_data_ptr=NULL);

Matrix<Array<unsigned char,3>> eagleeye_NV21_to_RGB(unsigned char* i420_data, int width, int height, unsigned char* rgb_data_ptr=NULL);
Matrix<Array<unsigned char,3>> eagleeye_NV21_to_BGR(unsigned char* i420_data, int width, int height, unsigned char* bgr_data_ptr=NULL);

Matrix<Array<unsigned char,3>> eagleeye_NV12_to_RGB(unsigned char* i420_data, int width, int height, unsigned char* rgb_data_ptr=NULL);
Matrix<Array<unsigned char,3>> eagleeye_NV12_to_BGR(unsigned char* i420_data, int width, int height, unsigned char* bgr_data_ptr=NULL);
} // namespace eagleeye

#endif