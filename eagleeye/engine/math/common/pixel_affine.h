#ifndef _EAGLEEYE_PIXEL_AFFINE_
#define _EAGLEEYE_PIXEL_AFFINE_

namespace eagleeye{
namespace math{
void warpaffine_bilinear_c4(const unsigned char* src, int srcw, int srch, unsigned char* dst, int w, int h, const float* tm, int type, unsigned int v);
void warpaffine_bilinear_c3(const unsigned char* src, int srcw, int srch, unsigned char* dst, int w, int h, const float* tm, int type, unsigned int v);
void warpaffine_bilinear_c2(const unsigned char* src, int srcw, int srch, unsigned char* dst, int w, int h, const float* tm, int type, unsigned int v);
void warpaffine_bilinear_c1(const unsigned char* src, int srcw, int srch, unsigned char* dst, int w, int h, const float* tm, int type, unsigned int v);

void warpaffine_bilinear_c1(const unsigned char* src, int srcw, int srch, int srcstride, unsigned char* dst, int w, int h, int stride, const float* tm, int type, unsigned int v);
void warpaffine_bilinear_c2(const unsigned char* src, int srcw, int srch, int srcstride, unsigned char* dst, int w, int h, int stride, const float* tm, int type, unsigned int v);
void warpaffine_bilinear_c3(const unsigned char* src, int srcw, int srch, int srcstride, unsigned char* dst, int w, int h, int stride, const float* tm, int type, unsigned int v);
void warpaffine_bilinear_c4(const unsigned char* src, int srcw, int srch, int srcstride, unsigned char* dst, int w, int h, int stride, const float* tm, int type, unsigned int v);
void warpaffine_bilinear_yuv420sp(const unsigned char* src, int srcw, int srch, unsigned char* dst, int w, int h, const float* tm, int type, unsigned int v);

void get_rotation_matrix(float angle, float scale, float dx, float dy, float* tm);
void get_affine_transform(const float* points_from, const float* points_to, int num_point, float* tm);
void invert_affine_transform(const float* tm, float* tm_inv);
} // namespace math

} // namespace eagleeye

#endif