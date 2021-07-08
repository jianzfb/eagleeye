#include "eagleeye/common/EagleeyeYUV.h"
#include "libyuv.h"

namespace eagleeye
{
/**
 * int I420Rotate(const uint8_t* src_y,
               int src_stride_y,
               const uint8_t* src_u,
               int src_stride_u,
               const uint8_t* src_v,
               int src_stride_v,
               uint8_t* dst_y,
               int dst_stride_y,
               uint8_t* dst_u,
               int dst_stride_u,
               uint8_t* dst_v,
               int dst_stride_v,
               int width,
               int height,
               enum RotationMode mode);
 */     
void eagleeye_I420_rotate_90(unsigned char* i420_data, int width, int height, unsigned char* rotated_i420_data){
    uint8_t* i420_data_y = (uint8_t*)i420_data;
    uint8_t* i420_data_u = i420_data_y + width * height;
    uint8_t* i420_data_v = i420_data_u + (int)(width * height * 0.25);

    uint8_t* rotated_i420_data_y = (uint8_t*)rotated_i420_data;
    uint8_t* rotated_i420_data_u = rotated_i420_data_y + width * height;
    uint8_t* rotated_i420_data_v = rotated_i420_data_u + (int)(width * height * 0.25);

    libyuv::I420Rotate(
            i420_data_y, width,
            i420_data_u, (width >> 1),
            i420_data_v, (width >> 1),
            rotated_i420_data_y, height,
            rotated_i420_data_u, (height >> 1),
            rotated_i420_data_v, (height >> 1),
            width, height, libyuv::kRotate90);
}

void eagleeye_I420_rotate_180(unsigned char* i420_data, int width, int height, unsigned char* rotated_i420_data){
    uint8_t* i420_data_y = (uint8_t*)i420_data;
    uint8_t* i420_data_u = i420_data_y + width * height;
    uint8_t* i420_data_v = i420_data_u + (int)(width * height * 0.25);

    uint8_t* rotated_i420_data_y = (uint8_t*)rotated_i420_data;
    uint8_t* rotated_i420_data_u = rotated_i420_data_y + width * height;
    uint8_t* rotated_i420_data_v = rotated_i420_data_u + (int)(width * height * 0.25);

    libyuv::I420Rotate(
            i420_data_y, width,
            i420_data_u, (width >> 1),
            i420_data_v, (width >> 1),
            rotated_i420_data_y, width,
            rotated_i420_data_u, (width >> 1),
            rotated_i420_data_v, (width >> 1),
            width, height, libyuv::kRotate180);
}

void eagleeye_I420_rotate_270(unsigned char* i420_data, int width, int height, unsigned char* rotated_i420_data){
    uint8_t* i420_data_y = (uint8_t*)i420_data;
    uint8_t* i420_data_u = i420_data_y + width * height;
    uint8_t* i420_data_v = i420_data_u + (int)(width * height * 0.25);

    uint8_t* rotated_i420_data_y = (uint8_t*)rotated_i420_data;
    uint8_t* rotated_i420_data_u = rotated_i420_data_y + width * height;
    uint8_t* rotated_i420_data_v = rotated_i420_data_u + (int)(width * height * 0.25);

    libyuv::I420Rotate(
            i420_data_y, width,
            i420_data_u, (width >> 1),
            i420_data_v, (width >> 1),
            rotated_i420_data_y, height,
            rotated_i420_data_u, (height >> 1),
            rotated_i420_data_v, (height >> 1),
            width, height, libyuv::kRotate270);
}


Matrix<Array<unsigned char,3>> eagleeye_I420_to_RGB(unsigned char* i420_data, int width, int height){
    uint8_t* i420_data_y = (uint8_t*)i420_data;
    uint8_t* i420_data_u = i420_data_y + width * height;
    uint8_t* i420_data_v = i420_data_u + (int)(width * height * 0.25);

    Matrix<Array<unsigned char,3>> rgb_data(height, width);
    unsigned char* rgb_data_ptr = (unsigned char*)rgb_data.dataptr();
    libyuv::I420ToRAW(i420_data_y, width, 
                        i420_data_u, (width>>1),
                        i420_data_v, (width>>1),
                        rgb_data_ptr, width*3,
                        width,
                        height);

    return rgb_data;
}

Matrix<Array<unsigned char,3>> eagleeye_I420_to_BGR(unsigned char* i420_data, int width, int height){
    uint8_t* i420_data_y = (uint8_t*)i420_data;
    uint8_t* i420_data_u = i420_data_y + width * height;
    uint8_t* i420_data_v = i420_data_u + (int)(width * height * 0.25);

    Matrix<Array<unsigned char,3>> bgr_data(height, width);
    unsigned char* bgr_data_ptr = (unsigned char*)bgr_data.dataptr();
    libyuv::I420ToRGB24(i420_data_y, width, 
                        i420_data_u, (width>>1),
                        i420_data_v, (width>>1),
                        bgr_data_ptr, width*3,
                        width,
                        height);

    return bgr_data;
}

Matrix<Array<unsigned char,3>> eagleeye_NV21_to_RGB(unsigned char* nv21_data, int width, int height){
    uint8_t* nv21_data_y = (uint8_t*)nv21_data;
    uint8_t* nv21_data_vu = nv21_data_y + width * height;

    Matrix<Array<unsigned char,3>> rgb_data(height, width);
    unsigned char* rgb_data_ptr = (unsigned char*)rgb_data.dataptr();
    libyuv::NV21ToRAW(nv21_data_y,
            width,
            nv21_data_vu,
            width,
            rgb_data_ptr,
            width*3,
            width,
            height);

    return rgb_data;
}

Matrix<Array<unsigned char,3>> eagleeye_NV21_to_BGR(unsigned char* nv21_data, int width, int height){
    uint8_t* nv21_data_y = (uint8_t*)nv21_data;
    uint8_t* nv21_data_vu = nv21_data_y + width * height;

    Matrix<Array<unsigned char,3>> bgr_data(height, width);
    unsigned char* bgr_data_ptr = (unsigned char*)bgr_data.dataptr();
    libyuv::NV21ToRGB24(nv21_data_y,
            width,
            nv21_data_vu,
            width,
            bgr_data_ptr,
            width*3,
            width,
            height);

    return bgr_data;
}

Matrix<Array<unsigned char,3>> eagleeye_NV12_to_RGB(unsigned char* nv12_data, int width, int height){
    uint8_t* nv12_data_y = (uint8_t*)nv12_data;
    uint8_t* nv12_data_vu = nv12_data_y + width * height;

    Matrix<Array<unsigned char,3>> rgb_data(height, width);
    unsigned char* rgb_data_ptr = (unsigned char*)rgb_data.dataptr();
    libyuv::NV12ToRAW(nv12_data_y,
            width,
            nv12_data_vu,
            width,
            rgb_data_ptr,
            width*3,
            width,
            height);

    return rgb_data;
}

Matrix<Array<unsigned char,3>> eagleeye_NV12_to_BGR(unsigned char* nv12_data, int width, int height){
    uint8_t* nv12_data_y = (uint8_t*)nv12_data;
    uint8_t* nv12_data_vu = nv12_data_y + width * height;

    Matrix<Array<unsigned char,3>> bgr_data(height, width);
    unsigned char* bgr_data_ptr = (unsigned char*)bgr_data.dataptr();
    libyuv::NV12ToRGB24(nv12_data_y,
            width,
            nv12_data_vu,
            width,
            bgr_data_ptr,
            width*3,
            width,
            height);

    return bgr_data;
}
} // namespace eagleeye
