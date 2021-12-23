#ifndef _EAGLEEYE_INTERPOLATE_H_
#define _EAGLEEYE_INTERPOLATE_H_
#include <string>
#include <vector>
#include <arm_neon.h>

namespace eagleeye {
namespace math {
namespace arm {

void bilinear_interp(const float* src,
                     int w_in,
                     int h_in,
                     float* dst,
                     int w_out,
                     int h_out,
                     float scale_x,
                     float scale_y,
                     bool with_align,
                     int align_mode);

void nearest_interp(const float* src,
                    int w_in,
                    int h_in,
                    float* dst,
                    int w_out,
                    int h_out,
                    float scale_x,
                    float scale_y,
                    bool with_align);

void bilinear_rgb_8u_3d_interp(
                    const unsigned char * pSrcImg, 
                    unsigned char * pDesImg, 
                    int srcWidth, 
                    int srcHeight, 
                    int srcOffsetX, 
                    int srcOffsetY, 
                    int srcStride, 
                    int desWidth, 
                    int desHeight);

void bilinear_gray_8u_1d_interp(
                    const unsigned char * pSrcImg, 
                    unsigned char * pDesImg, 
                    int srcWidth, 
                    int srcHeight, 
                    int srcOffsetX, 
                    int srcOffsetY, 
                    int srcStride, 
                    int desWidth, 
                    int desHeight);

void bilinear_32f_c1_interp(
                    const float* pSrcImg, 
                    float* pDesImg, 
                    int srcWidth, 
                    int srcHeight, 
                    int srcOffsetX, 
                    int srcOffsetY, 
                    int srcStride, 
                    int dstWidth, 
                    int dstHeight);
// void interpolate(lite::Tensor* X,
//                  lite::Tensor* OutSize,
//                  std::vector<const lite::Tensor*> SizeTensor,
//                  lite::Tensor* Scale,
//                  lite::Tensor* Out,
//                  int out_height,
//                  int out_width,
//                  float scale,
//                  bool with_align,
//                  int align_mode,
//                  std::string interpolate_type);

} /* namespace math */
} /* namespace arm */
} /* namespace paddle */

#endif