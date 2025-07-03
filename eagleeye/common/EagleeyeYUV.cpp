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
void I420_rotate_90(unsigned char* i420_data, int width, int height, unsigned char* rotated_i420_data){
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

void I420_rotate_180(unsigned char* i420_data, int width, int height, unsigned char* rotated_i420_data){
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

void I420_rotate_270(unsigned char* i420_data, int width, int height, unsigned char* rotated_i420_data){
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


Matrix<Array<unsigned char,3>> I420_to_RGB(unsigned char* i420_data, int width, int height, unsigned char* rgb_data_ptr){
    uint8_t* i420_data_y = (uint8_t*)i420_data;
    uint8_t* i420_data_u = i420_data_y + width * height;
    uint8_t* i420_data_v = i420_data_u + (int)(width * height * 0.25);

    Matrix<Array<unsigned char,3>> rgb_data;
    if(rgb_data_ptr != NULL){
        rgb_data = Matrix<Array<unsigned char,3>>(height, width, rgb_data_ptr);
    }
    else{
        rgb_data = Matrix<Array<unsigned char,3>>(height, width);
        rgb_data_ptr = (unsigned char*)rgb_data.dataptr();
    }

    libyuv::I420ToRAW(i420_data_y, width, 
                        i420_data_u, (width>>1),
                        i420_data_v, (width>>1),
                        rgb_data_ptr, width*3,
                        width,
                        height);

    return rgb_data;
}

Matrix<Array<unsigned char,3>> I420_to_BGR(unsigned char* i420_data, int width, int height, unsigned char* bgr_data_ptr){
    uint8_t* i420_data_y = (uint8_t*)i420_data;
    uint8_t* i420_data_u = i420_data_y + width * height;
    uint8_t* i420_data_v = i420_data_u + (int)(width * height * 0.25);

    Matrix<Array<unsigned char,3>> bgr_data;
    if(bgr_data_ptr != NULL){
        bgr_data = Matrix<Array<unsigned char,3>>(height, width, bgr_data_ptr);
    }
    else{
        bgr_data = Matrix<Array<unsigned char,3>>(height, width);
        bgr_data_ptr = (unsigned char*)bgr_data.dataptr();
    }

    libyuv::I420ToRGB24(i420_data_y, width, 
                        i420_data_u, (width>>1),
                        i420_data_v, (width>>1),
                        bgr_data_ptr, width*3,
                        width,
                        height);

    return bgr_data;
}

Matrix<Array<unsigned char,3>> NV21_to_RGB(unsigned char* nv21_data, int width, int height, unsigned char* rgb_data_ptr){
    uint8_t* nv21_data_y = (uint8_t*)nv21_data;
    uint8_t* nv21_data_vu = nv21_data_y + width * height;

    Matrix<Array<unsigned char,3>> rgb_data;
    if(rgb_data_ptr != NULL){
        rgb_data = Matrix<Array<unsigned char,3>>(height, width, rgb_data_ptr);
    }
    else{
        rgb_data = Matrix<Array<unsigned char,3>>(height, width);
        rgb_data_ptr = (unsigned char*)rgb_data.dataptr();
    }

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

Matrix<Array<unsigned char,3>> NV21_to_BGR(unsigned char* nv21_data, int width, int height, unsigned char* bgr_data_ptr){
    uint8_t* nv21_data_y = (uint8_t*)nv21_data;
    uint8_t* nv21_data_vu = nv21_data_y + width * height;

    Matrix<Array<unsigned char,3>> bgr_data;
    if(bgr_data_ptr != NULL){
        bgr_data = Matrix<Array<unsigned char,3>>(height, width, bgr_data_ptr);
    }
    else{
        bgr_data = Matrix<Array<unsigned char,3>>(height, width);
        bgr_data_ptr = (unsigned char*)bgr_data.dataptr();
    }

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

Matrix<Array<unsigned char,3>> NV12_to_RGB(unsigned char* nv12_data, int width, int height, unsigned char* rgb_data_ptr){
    uint8_t* nv12_data_y = (uint8_t*)nv12_data;
    uint8_t* nv12_data_vu = nv12_data_y + width * height;

    Matrix<Array<unsigned char,3>> rgb_data;
    if(rgb_data_ptr != NULL){
        rgb_data = Matrix<Array<unsigned char,3>>(height, width, rgb_data_ptr);
    }
    else{
        rgb_data = Matrix<Array<unsigned char,3>>(height, width);
        rgb_data_ptr = (unsigned char*)rgb_data.dataptr();
    }

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

Matrix<Array<unsigned char,3>> NV12_to_BGR(unsigned char* nv12_data, int width, int height, unsigned char* bgr_data_ptr){
    uint8_t* nv12_data_y = (uint8_t*)nv12_data;
    uint8_t* nv12_data_vu = nv12_data_y + width * height;

    Matrix<Array<unsigned char,3>> bgr_data;
    if(bgr_data_ptr != NULL){
        bgr_data = Matrix<Array<unsigned char,3>>(height, width, bgr_data_ptr);
    }
    else{
        bgr_data = Matrix<Array<unsigned char,3>>(height, width);
        bgr_data_ptr = (unsigned char*)bgr_data.dataptr();
    }

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

void Android420_to_I420_Rotate(const uint8_t* src_android_420_data, uint8_t* dst_i420_data, int width, int height, int degree){
    int src_i420_y_size = width * height;
    const uint8_t *src_i420_y_data = src_android_420_data;
    const uint8_t *src_i420_u_data = src_android_420_data + src_i420_y_size;

    const int y_plane_length = width * height;
    const int uv_plane_length = y_plane_length / 4;

    uint8_t* dst_i420_data_y = dst_i420_data;
    uint8_t* dst_i420_data_u = dst_i420_data + y_plane_length;
    uint8_t* dst_i420_data_v = dst_i420_data + y_plane_length + uv_plane_length;

    int base_dst_stride_dimension = width;
    if (90 == degree || 270 == degree) base_dst_stride_dimension = height;

    libyuv::RotationMode rm = libyuv::kRotate0;
    if(degree == 90){
        rm = libyuv::kRotate90;
    }
    else if(degree == 180){
        rm = libyuv::kRotate180;
    }
    else if(degree == 270){
        rm = libyuv::kRotate270;
    }
    libyuv::Android420ToI420Rotate(
        src_i420_y_data, width,
        src_i420_u_data, width,
        src_i420_u_data+1, width,
        2,
        dst_i420_data_y, base_dst_stride_dimension, 
        dst_i420_data_u, (base_dst_stride_dimension>>1),
        dst_i420_data_v, (base_dst_stride_dimension>>1),
        width,
        height,
        rm
    );
}
} // namespace eagleeye
