#include "eagleeye/engine/math/arm/interpolate.h"
#include <string>
#include <vector>
#include <cmath>
#include <cstring>

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
                     int align_mode) {
  int* buf = new int[w_out + h_out + w_out * 2 + h_out * 2];

  int* xofs = buf;
  int* yofs = buf + w_out;

  float* alpha = reinterpret_cast<float*>(buf + w_out + h_out);
  float* beta = reinterpret_cast<float*>(buf + w_out + h_out + w_out * 2);

  float fx = 0.0f;
  float fy = 0.0f;
  int sx = 0;
  int sy = 0;
  if (with_align) {
    scale_x = static_cast<float>(w_in - 1) / (w_out - 1);
    scale_y = static_cast<float>(h_in - 1) / (h_out - 1);
    // calculate x axis coordinate
    for (int dx = 0; dx < w_out; dx++) {
      fx = dx * scale_x;
      sx = static_cast<int>(fx);
      fx -= sx;
      xofs[dx] = sx;
      alpha[dx * 2] = 1.f - fx;
      alpha[dx * 2 + 1] = fx;
    }
    // calculate y axis coordinate
    for (int dy = 0; dy < h_out; dy++) {
      fy = dy * scale_y;
      sy = static_cast<int>(fy);
      fy -= sy;
      yofs[dy] = sy;
      beta[dy * 2] = 1.f - fy;
      beta[dy * 2 + 1] = fy;
    }
  } else {
    scale_x = static_cast<float>(w_in) / w_out;
    scale_y = static_cast<float>(h_in) / h_out;
    // calculate x axis coordinate
    for (int dx = 0; dx < w_out; dx++) {
      fx = align_mode ? scale_x * dx : scale_x * (dx + 0.5f) - 0.5f;
      fx = fx < 0 ? 0.f : fx;
      sx = static_cast<int>(fx);
      fx -= sx;
      xofs[dx] = sx;
      alpha[dx * 2] = 1.f - fx;
      alpha[dx * 2 + 1] = fx;
    }
    // calculate y axis coordinate
    for (int dy = 0; dy < h_out; dy++) {
      fy = align_mode ? scale_y * dy : scale_y * (dy + 0.5f) - 0.5f;
      fy = fy < 0 ? 0.f : fy;
      sy = static_cast<int>(fy);
      fy -= sy;
      yofs[dy] = sy;
      beta[dy * 2] = 1.f - fy;
      beta[dy * 2 + 1] = fy;
    }
  }
  float* rowsbuf0 = new float[w_out];
  float* rowsbuf1 = new float[w_out];
  float* rows0 = rowsbuf0;
  float* rows1 = rowsbuf1;
  // output w , h boundary
  int w_bound = w_out;
  int h_bound = h_out;
  if (with_align) {
    w_bound = ceil((w_in - 1) / scale_x);
    h_bound = ceil((h_in - 1) / scale_y);
  } else {
    w_bound = ceil((w_in - 0.5f) / scale_x - 0.5f);
    h_bound = ceil((h_in - 0.5f) / scale_y - 0.5f);
  }
  // h_bound loop
  for (int dy = 0; dy < h_bound; dy++) {
    int sy = yofs[dy];

    const float* s0 = src + sy * w_in;
    const float* s1 = src + (sy + 1) * w_in;

    const float* alphap = alpha;
    float* rows0p = rows0;
    float* rows1p = rows1;

    int dx = 0;
    // w_bound loop
    for (; dx + 1 < w_bound; dx += 2) {
      int sx = xofs[dx];
      int sxn = xofs[dx + 1];
      const float* s0p = s0 + sx;
      const float* s1p = s1 + sx;
      const float* s0np = s0 + sxn;
      const float* s1np = s1 + sxn;

      float32x4_t _a = vld1q_f32(alphap);
      float32x2_t _s0 = vld1_f32(s0p);
      float32x2_t _s1 = vld1_f32(s1p);
      float32x2_t _s0n = vld1_f32(s0np);
      float32x2_t _s1n = vld1_f32(s1np);

      float32x4_t _s0s0n = vcombine_f32(_s0, _s0n);
      float32x4_t _ms0 = vmulq_f32(_s0s0n, _a);
      float32x4_t _s1s1n = vcombine_f32(_s1, _s1n);
      float32x4_t _ms1 = vmulq_f32(_s1s1n, _a);

      float32x2_t _rows0 = vpadd_f32(vget_low_f32(_ms0), vget_high_f32(_ms0));
      vst1_f32(rows0p + dx, _rows0);
      float32x2_t _rows1 = vpadd_f32(vget_low_f32(_ms1), vget_high_f32(_ms1));
      vst1_f32(rows1p + dx, _rows1);

      alphap += 4;
    }
    // w_bound remain loop
    for (; dx < w_bound; dx++) {
      int sx = xofs[dx];
      const float* s0p = s0 + sx;
      const float* s1p = s1 + sx;

      float a0 = alphap[0];
      float a1 = alphap[1];
      rows0p[dx] = s0p[0] * a0 + s0p[1] * a1;
      rows1p[dx] = s1p[0] * a0 + s1p[1] * a1;

      alphap += 2;
    }

    const float buffer1[2] = {*(src + sy * w_in + w_in - 1),
                              *(src + sy * w_in + w_in - 1)};
    const float buffer2[2] = {*(src + (sy + 1) * w_in + w_in - 1),
                              *(src + (sy + 1) * w_in + w_in - 1)};
    // w_bound - w_out loop
    for (; dx + 1 < w_out; dx += 2) {
      const float* s0p = buffer1;
      const float* s1p = buffer2;
      const float* s0np = buffer1;
      const float* s1np = buffer2;

      float32x4_t _a = vld1q_f32(alphap);
      float32x2_t _s0 = vld1_f32(s0p);
      float32x2_t _s1 = vld1_f32(s1p);
      float32x2_t _s0n = vld1_f32(s0np);
      float32x2_t _s1n = vld1_f32(s1np);

      float32x4_t _s0s0n = vcombine_f32(_s0, _s0n);
      float32x4_t _ms0 = vmulq_f32(_s0s0n, _a);
      float32x4_t _s1s1n = vcombine_f32(_s1, _s1n);
      float32x4_t _ms1 = vmulq_f32(_s1s1n, _a);

      float32x2_t _rows0 = vpadd_f32(vget_low_f32(_ms0), vget_high_f32(_ms0));
      vst1_f32(rows0p + dx, _rows0);
      float32x2_t _rows1 = vpadd_f32(vget_low_f32(_ms1), vget_high_f32(_ms1));
      vst1_f32(rows1p + dx, _rows1);

      alphap += 4;
    }
    // w_bound - w_out remain loop
    for (; dx < w_out; dx++) {
      const float* s0p = buffer1;
      const float* s1p = buffer2;

      float a0 = alphap[0];
      float a1 = alphap[1];
      rows0p[dx] = s0p[0] * a0 + s0p[1] * a1;
      rows1p[dx] = s1p[0] * a0 + s1p[1] * a1;

      alphap += 2;
    }

    float b0 = beta[0];
    float b1 = beta[1];

    float* dp = dst + dy * w_out;

    int nn = w_out >> 3;
    int remain = w_out - (nn << 3);

#ifdef __aarch64__
    float32x4_t _b0 = vdupq_n_f32(b0);
    float32x4_t _b1 = vdupq_n_f32(b1);
    // calculate and store results
    for (; nn > 0; nn--) {
      float32x4_t _rows0 = vld1q_f32(rows0p);
      float32x4_t _d = vmulq_f32(_rows0, _b0);
      float32x4_t _rows1 = vld1q_f32(rows1p);
      _d = vmlaq_f32(_d, _rows1, _b1);

      float32x4_t _rows0n = vld1q_f32(rows0p + 4);
      float32x4_t _rows1n = vld1q_f32(rows1p + 4);

      float32x4_t _dn = vmulq_f32(_rows0n, _b0);
      vst1q_f32(dp, _d);
      _dn = vmlaq_f32(_dn, _rows1n, _b1);
      vst1q_f32(dp + 4, _dn);

      dp += 8;
      rows0p += 8;
      rows1p += 8;
    }

#else
    if (nn > 0) {
      asm volatile(
          "vdup.32 q0, %[b0]                   @dup b0 to q1\n"
          "vdup.32 q1, %[b1]                   @dup b1 to q0\n"
          "1:                                                      \n"
          "vld1.32 {d4-d5}, [%[rows0p]]!       @loads rows0p to q2\n"
          "vld1.32 {d6-d7}, [%[rows1p]]!       @loads rows0p to q3\n"
          "vmul.f32 q2, q2, q0                 @mul\n"
          "vmla.f32 q2, q3, q1                 @mul add\n"
          "vst1.32 {d4-d5}, [%[out]]!          @store out to q2 \n"
          "pld [%[rows0p]]                     @preload rows0p\n"

          "vld1.32 {d4-d5}, [%[rows0p]]!       @loads rows0p to q2\n"
          "vld1.32 {d6-d7}, [%[rows1p]]!       @load rows1p to q3\n"
          "vmul.f32 q2, q2, q0                 @mul\n"
          "vmla.f32 q2, q3, q1                 @mul add\n"
          "vst1.32 {d4-d5}, [%[out]]!          @store out to q2 \n"
          "pld [%[rows1p]]                     @preload rows1p\n"
          "subs %[loopc], #1                   @loop count minus #1\n"
          "bne 1b                              @jump to 1\n"
          : [rows0p] "+r"(rows0p),
            [rows1p] "+r"(rows1p),
            [out] "+r"(dp),
            [loopc] "+r"(nn)
          : [b0] "r"(b0), [b1] "r"(b1)
          : "cc", "memory", "q0", "q1", "q2", "q3");
    }
#endif
    // calculate and store remain resluts
    for (; remain; --remain) {
      *dp++ = *rows0p++ * b0 + *rows1p++ * b1;
    }
    beta += 2;
  }

  // h_bound - h_out loop
  for (int dy = h_bound; dy < h_out; dy++) {
    int sy = h_in - 1;
    const float* s0 = src + sy * w_in;
    const float* s1 = s0;
    const float* alphap = alpha;
    float* rows0p = rows0;
    float* rows1p = rows1;

    int dx = 0;
    // w_bound loop
    for (; dx + 1 < w_bound; dx += 2) {
      int sx = xofs[dx];
      int sxn = xofs[dx + 1];
      const float* s0p = s0 + sx;
      const float* s1p = s1 + sx;
      const float* s0np = s0 + sxn;
      const float* s1np = s1 + sxn;

      float32x4_t _a = vld1q_f32(alphap);
      float32x2_t _s0 = vld1_f32(s0p);
      float32x2_t _s1 = vld1_f32(s1p);
      float32x2_t _s0n = vld1_f32(s0np);
      float32x2_t _s1n = vld1_f32(s1np);

      float32x4_t _s0s0n = vcombine_f32(_s0, _s0n);
      float32x4_t _ms0 = vmulq_f32(_s0s0n, _a);
      float32x4_t _s1s1n = vcombine_f32(_s1, _s1n);
      float32x4_t _ms1 = vmulq_f32(_s1s1n, _a);

      float32x2_t _rows0 = vpadd_f32(vget_low_f32(_ms0), vget_high_f32(_ms0));
      vst1_f32(rows0p + dx, _rows0);
      float32x2_t _rows1 = vpadd_f32(vget_low_f32(_ms1), vget_high_f32(_ms1));
      vst1_f32(rows1p + dx, _rows1);

      alphap += 4;
    }
    // w_bound remain loop
    for (; dx < w_bound; dx++) {
      int sx = xofs[dx];
      const float* s0p = s0 + sx;
      float a0 = alphap[0];
      float a1 = alphap[1];
      rows0p[dx] = s0p[0] * a0 + s0p[1] * a1;
      rows1p[dx] = rows0p[dx];

      alphap += 2;
    }

    const float buffer1[2] = {*(src + sy * w_in + w_in - 1),
                              *(src + sy * w_in + w_in - 1)};
    // w_bound - w_out loop
    for (; dx + 1 < w_out; dx += 2) {
      const float* s0p = buffer1;
      const float* s1p = buffer1;
      const float* s0np = buffer1;
      const float* s1np = buffer1;

      float32x4_t _a = vld1q_f32(alphap);
      float32x2_t _s0 = vld1_f32(s0p);
      float32x2_t _s1 = vld1_f32(s1p);
      float32x2_t _s0n = vld1_f32(s0np);
      float32x2_t _s1n = vld1_f32(s1np);

      float32x4_t _s0s0n = vcombine_f32(_s0, _s0n);
      float32x4_t _ms0 = vmulq_f32(_s0s0n, _a);
      float32x4_t _s1s1n = vcombine_f32(_s1, _s1n);
      float32x4_t _ms1 = vmulq_f32(_s1s1n, _a);

      float32x2_t _rows0 = vpadd_f32(vget_low_f32(_ms0), vget_high_f32(_ms0));
      vst1_f32(rows0p + dx, _rows0);
      float32x2_t _rows1 = vpadd_f32(vget_low_f32(_ms1), vget_high_f32(_ms1));
      vst1_f32(rows1p + dx, _rows1);

      alphap += 4;
    }
    // w_bound - wout remain loop
    for (; dx < w_out; dx++) {
      const float* s0p = buffer1;
      float a0 = alphap[0];
      float a1 = alphap[1];
      rows0p[dx] = s0p[0] * a0 + s0p[1] * a1;
      rows1p[dx] = rows0p[dx];
      alphap += 2;
    }

    float b0 = beta[0];
    float b1 = beta[1];

    float* dp = dst + dy * w_out;

    int nn = w_out >> 3;
    int remain = w_out - (nn << 3);

#ifdef __aarch64__
    float32x4_t _b0 = vdupq_n_f32(b0);
    float32x4_t _b1 = vdupq_n_f32(b1);
    // calculate and store results
    for (; nn > 0; nn--) {
      float32x4_t _rows0 = vld1q_f32(rows0p);
      float32x4_t _d = vmulq_f32(_rows0, _b0);
      float32x4_t _rows1 = vld1q_f32(rows1p);
      _d = vmlaq_f32(_d, _rows1, _b1);

      float32x4_t _rows0n = vld1q_f32(rows0p + 4);
      float32x4_t _rows1n = vld1q_f32(rows1p + 4);

      float32x4_t _dn = vmulq_f32(_rows0n, _b0);
      vst1q_f32(dp, _d);
      _dn = vmlaq_f32(_dn, _rows1n, _b1);
      vst1q_f32(dp + 4, _dn);

      dp += 8;
      rows0p += 8;
      rows1p += 8;
    }

#else
    if (nn > 0) {
      asm volatile(
          "vdup.32 q0, %[b0]                   @dup b0 to q1\n"
          "vdup.32 q1, %[b1]                   @dup b1 to q0\n"
          "1:                                                      \n"
          "vld1.32 {d4-d5}, [%[rows0p]]!       @loads rows0p to q2\n"
          "vld1.32 {d6-d7}, [%[rows1p]]!       @loads rows0p to q3\n"
          "vmul.f32 q2, q2, q0                 @mul\n"
          "vmla.f32 q2, q3, q1                 @mul add\n"
          "vst1.32 {d4-d5}, [%[out]]!          @store out to q2 \n"
          "pld [%[rows0p]]                     @preload rows0p\n"

          "vld1.32 {d4-d5}, [%[rows0p]]!       @loads rows0p to q2\n"
          "vld1.32 {d6-d7}, [%[rows1p]]!       @load rows1p to q3\n"
          "vmul.f32 q2, q2, q0                 @mul\n"
          "vmla.f32 q2, q3, q1                 @mul add\n"
          "vst1.32 {d4-d5}, [%[out]]!          @store out to q2 \n"
          "pld [%[rows1p]]                     @preload rows1p\n"
          "subs %[loopc], #1                   @loop count minus #1\n"
          "bne 1b                              @jump to 1\n"
          : [rows0p] "+r"(rows0p),
            [rows1p] "+r"(rows1p),
            [out] "+r"(dp),
            [loopc] "+r"(nn)
          : [b0] "r"(b0), [b1] "r"(b1)
          : "cc", "memory", "q0", "q1", "q2", "q3");
    }
#endif
    // calculate and store remain results
    for (; remain; --remain) {
      *dp++ = *rows0p++ * b0 + *rows1p++ * b1;
    }

    beta += 2;
  }
  delete[] buf;
  delete[] rowsbuf0;
  delete[] rowsbuf1;
}

void nearest_interp(const float* src,
                    int w_in,
                    int h_in,
                    float* dst,
                    int w_out,
                    int h_out,
                    float scale_x,
                    float scale_y,
                    bool with_align) {
  float scale_w_new = (with_align)
                          ? (static_cast<float>(w_in - 1) / (w_out - 1))
                          : (static_cast<float>(w_in) / (w_out));
  float scale_h_new = (with_align)
                          ? (static_cast<float>(h_in - 1) / (h_out - 1))
                          : (static_cast<float>(h_in) / (h_out));
  if (with_align) {
    for (int h = 0; h < h_out; ++h) {
      float* dst_p = dst + h * w_out;
      int near_y = static_cast<int>(scale_h_new * h + 0.5);
      for (int w = 0; w < w_out; ++w) {
        int near_x = static_cast<int>(scale_w_new * w + 0.5);
        *dst_p++ = src[near_y * w_in + near_x];
      }
    }
  } else {
    for (int h = 0; h < h_out; ++h) {
      float* dst_p = dst + h * w_out;
      int near_y = static_cast<int>(scale_h_new * h);
      for (int w = 0; w < w_out; ++w) {
        int near_x = static_cast<int>(scale_w_new * w);
        *dst_p++ = src[near_y * w_in + near_x];
      }
    }
  }
}

#define QRBAR_SHIFTBITS    8
#define QRBAR_ROUND0(x)  (x>>QRBAR_SHIFTBITS)
#define QRBAR_ROUND1(x)  (ROUND0(x))+1
#ifndef QRBAR_CLIP
#define QRBAR_CLIP(x) ( x<0 ? 0 : (x>255 ? 255 : x) )
#endif

void bilinear_rgb_8u_3d_interp(
                    const unsigned char * pSrcImg, 
                    unsigned char * pDesImg, 
                    int srcWidth, 
                    int srcHeight, 
                    int srcOffsetX, 
                    int srcOffsetY, 
                    int srcStride, 
                    int desWidth, 
                    int desHeight){
  	int i, j;
	int nRateW, nRateH;
	int y;
	unsigned short *coord_x, *coord_y;
	unsigned char *bi_coef_x, *bi_coef_y;
	unsigned char *sub_bi_coef_x, *sub_bi_coef_y;
	unsigned char *pImg_left_top, *pImg_left_down;
	unsigned char *pImg_right_top, *pImg_right_down;
	int float_value;
	const short std_1_value = (1 << QRBAR_SHIFTBITS);
	const short std_and_value = ((1 << QRBAR_SHIFTBITS) - 1);
	int res_x0, res_x1;
	int res_xy;
	const unsigned char *pSrc, *pSrc1;

	pImg_left_top = (unsigned char *)malloc(sizeof(char)*desWidth * 3);
	pImg_left_down = (unsigned char *)malloc(sizeof(char)*desWidth * 3);
	pImg_right_top = (unsigned char *)malloc(sizeof(char)*desWidth * 3);
	pImg_right_down = (unsigned char *)malloc(sizeof(char)*desWidth * 3);

	coord_x = (unsigned short *)malloc(sizeof(short)*desWidth);
	coord_y = (unsigned short *)malloc(sizeof(short)*desHeight);
	bi_coef_x = (unsigned char *)malloc(sizeof(char)*desWidth);
	bi_coef_y = (unsigned char *)malloc(sizeof(char)*desHeight);
	sub_bi_coef_x = (unsigned char *)malloc(sizeof(char)*desWidth);
	sub_bi_coef_y = (unsigned char *)malloc(sizeof(char)*desHeight);

	if (coord_x == NULL || coord_y == NULL || bi_coef_x == NULL || bi_coef_y == NULL || sub_bi_coef_x == NULL || sub_bi_coef_y == NULL ||
		pImg_left_top == NULL || pImg_left_down == NULL || pImg_right_top == NULL || pImg_right_down == NULL)
		return;

	nRateW = (srcWidth << QRBAR_SHIFTBITS) / (desWidth);
	nRateH = (srcHeight << QRBAR_SHIFTBITS) / (desHeight);

	for (i = 0; i < desHeight; ++i)
	{
		float_value = i * nRateH;
		bi_coef_y[i] = float_value & std_and_value;
		if (!bi_coef_y[i])
			bi_coef_y[i] = 1;
		sub_bi_coef_y[i] = std_1_value - bi_coef_y[i];
		coord_y[i] = QRBAR_ROUND0(float_value);
		if (coord_y[i] > srcHeight - 2)
			coord_y[i] = srcHeight - 2;
	}

	for (i = 0; i < desWidth; ++i)
	{
		float_value = i * nRateW;
		bi_coef_x[i] = float_value & std_and_value;
		if (!bi_coef_x[i])
			bi_coef_x[i] = 1;
		sub_bi_coef_x[i] = std_1_value - bi_coef_x[i];
		coord_x[i] = QRBAR_ROUND0(float_value);
		if (coord_x[i] > srcWidth - 2)
			coord_x[i] = srcWidth - 2;
	}

	for (i = 0; i < desHeight; i++)
	{
		// pSrc = pSrcImg + coord_y[i] * srcWidth * 3;
		pSrc = pSrcImg + (coord_y[i]+srcOffsetY)*srcStride*3 + srcOffsetX*3;
		y = coord_y[i];
		for (j = 0; j < desWidth; j++)
		{
			pSrc1 = pSrc;
			pSrc1 += coord_x[j] * 3;
			memcpy(pImg_left_top + j * 3, pSrc1, 3);
			memcpy(pImg_right_top + j * 3, pSrc1 + 3, 3);
			// pSrc1 += srcWidth * 3;
			pSrc1 += srcStride*3;
			memcpy(pImg_left_down + j * 3, pSrc1, 3);
			memcpy(pImg_right_down + j * 3, pSrc1 + 3, 3);
		}

#ifdef __aarch64__
		unsigned char *img1, *img2, *img3, *img4;
		unsigned char *outimg;
		unsigned char *coef1, *coef2, *coef3, *coef4;
		unsigned char *coef_x1, *coef_x2;
		uint8x8_t value_y1, value_y2;

		value_y1 = vdup_n_u8(sub_bi_coef_y[i]);
		value_y2 = vdup_n_u8(bi_coef_y[i]);
		img1 = pImg_left_top;
		img2 = pImg_right_top;
		img3 = pImg_left_down;
		img4 = pImg_right_down;
		outimg = pDesImg + i*desWidth*3;
		coef_x1 = sub_bi_coef_x;
		coef_x2 = bi_coef_x;
		for (j = 0; j < desWidth; j += 8)
		{
			// uint8x8_t v1, v2, v3, v4;
			uint8x8_t value_x1, value_x2;
			uint8x8_t bires_x0, bires_x1;
			uint8x8x3_t bires_xy;
			uint16x8_t multi_value;

			// v1 = vld1_u8(img1);
			// v2 = vld1_u8(img2);
			// v3 = vld1_u8(img3);
			// v4 = vld1_u8(img4);

			uint8x8x3_t v1_3, v2_3, v3_3, v4_3;
			v1_3 = vld3_u8(img1);
			v2_3 = vld3_u8(img2);
			v3_3 = vld3_u8(img3);
			v4_3 = vld3_u8(img4);

			value_x1 = vld1_u8(coef_x1);
			value_x2 = vld1_u8(coef_x2);
			// round 1
			multi_value = vmull_u8(v1_3.val[0], value_x1);
			multi_value = vmlal_u8(multi_value, v2_3.val[0], value_x2);
			bires_x0 = vshrn_n_u16(multi_value, QRBAR_SHIFTBITS);
			multi_value = vmull_u8(v3_3.val[0], value_x1);
			multi_value = vmlal_u8(multi_value, v4_3.val[0], value_x2);
			bires_x1 = vshrn_n_u16(multi_value, QRBAR_SHIFTBITS);
			multi_value = vmull_u8(bires_x0, value_y1);
			multi_value = vmlal_u8(multi_value, bires_x1, value_y2);
			bires_xy.val[0] = vshrn_n_u16(multi_value, QRBAR_SHIFTBITS);

			// round 2
			multi_value = vmull_u8(v1_3.val[1], value_x1);
			multi_value = vmlal_u8(multi_value, v2_3.val[1], value_x2);
			bires_x0 = vshrn_n_u16(multi_value, QRBAR_SHIFTBITS);
			multi_value = vmull_u8(v3_3.val[1], value_x1);
			multi_value = vmlal_u8(multi_value, v4_3.val[1], value_x2);
			bires_x1 = vshrn_n_u16(multi_value, QRBAR_SHIFTBITS);
			multi_value = vmull_u8(bires_x0, value_y1);
			multi_value = vmlal_u8(multi_value, bires_x1, value_y2);
			bires_xy.val[1] = vshrn_n_u16(multi_value, QRBAR_SHIFTBITS);

			// round 3
			multi_value = vmull_u8(v1_3.val[2], value_x1);
			multi_value = vmlal_u8(multi_value, v2_3.val[2], value_x2);
			bires_x0 = vshrn_n_u16(multi_value, QRBAR_SHIFTBITS);
			multi_value = vmull_u8(v3_3.val[2], value_x1);
			multi_value = vmlal_u8(multi_value, v4_3.val[2], value_x2);
			bires_x1 = vshrn_n_u16(multi_value, QRBAR_SHIFTBITS);
			multi_value = vmull_u8(bires_x0, value_y1);
			multi_value = vmlal_u8(multi_value, bires_x1, value_y2);
			bires_xy.val[2] = vshrn_n_u16(multi_value, QRBAR_SHIFTBITS);			
			vst3_u8(outimg, bires_xy);

			outimg += 24;
			img1 += 24;
			img2 += 24;
			img3 += 24;
			img4 += 24;
			coef_x1 += 8;
			coef_x2 += 8;
		}

		for (; j < desWidth; j++)
		{
			res_x0 = (((img1[0])*sub_bi_coef_x[j] + (img2[0])*bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_x1 = (((img3[0])*sub_bi_coef_x[j] + (img4[0])*bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_xy = res_x0*sub_bi_coef_y[i] + res_x1*bi_coef_y[i];
			outimg[0] = (res_xy >> QRBAR_SHIFTBITS);
			res_x0 = (((img1[1])*sub_bi_coef_x[j] + (img2[1])*bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_x1 = (((img3[1])*sub_bi_coef_x[j] + (img4[1])*bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_xy = res_x0*sub_bi_coef_y[i] + res_x1*bi_coef_y[i];
			outimg[1] = (res_xy >> QRBAR_SHIFTBITS);
			res_x0 = (((img1[2])*sub_bi_coef_x[j] + (img2[2])*bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_x1 = (((img3[2])*sub_bi_coef_x[j] + (img4[2])*bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_xy = res_x0*sub_bi_coef_y[i] + res_x1*bi_coef_y[i];
			outimg[2] = (res_xy >> QRBAR_SHIFTBITS);

			outimg += 3;
			img1 += 3;
			img2 += 3;
			img3 += 3;
			img4 += 3;
		}
#else
		for (j = 0; j < desWidth; j++)
		{
			res_x0 = ((pImg_left_top[j * 3] * sub_bi_coef_x[j] + pImg_right_top[j * 3] * bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_x1 = ((pImg_left_down[j * 3] * sub_bi_coef_x[j] + pImg_right_down[j * 3] * bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_xy = res_x0*sub_bi_coef_y[i] + res_x1*bi_coef_y[i];
			pDesImg[(i*desWidth + j) * 3] = (res_xy >> QRBAR_SHIFTBITS);
			res_x0 = ((pImg_left_top[j * 3 + 1] * sub_bi_coef_x[j] + pImg_right_top[j * 3 + 1] * bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_x1 = ((pImg_left_down[j * 3 + 1] * sub_bi_coef_x[j] + pImg_right_down[j * 3 + 1] * bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_xy = res_x0*sub_bi_coef_y[i] + res_x1*bi_coef_y[i];
			pDesImg[(i*desWidth + j) * 3 + 1] = (res_xy >> QRBAR_SHIFTBITS);
			res_x0 = ((pImg_left_top[j * 3 + 2] * sub_bi_coef_x[j] + pImg_right_top[j * 3 + 2] * bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_x1 = ((pImg_left_down[j * 3 + 2] * sub_bi_coef_x[j] + pImg_right_down[j * 3 + 2] * bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_xy = res_x0*sub_bi_coef_y[i] + res_x1*bi_coef_y[i];
			pDesImg[(i*desWidth + j) * 3 + 2] = (res_xy >> QRBAR_SHIFTBITS);
		}
#endif
	}

	free(coord_x);
	free(coord_y);
	free(bi_coef_x);
	free(bi_coef_y);
	free(sub_bi_coef_x);
	free(sub_bi_coef_y);
	free(pImg_left_top);
	free(pImg_left_down);
	free(pImg_right_top);
	free(pImg_right_down);
}


void bilinear_gray_8u_1d_interp(
    const unsigned char * pSrcImg, 
    unsigned char * pDesImg, 
    int srcWidth, 
    int srcHeight, 
    int srcOffsetX, 
    int srcOffsetY, 
    int srcStride, 
    int desWidth, 
    int desHeight){
	int i, j;
	int nRateW, nRateH;
	int y;
	unsigned short *coord_x, *coord_y;
	unsigned char *bi_coef_x, *bi_coef_y;
	unsigned char *sub_bi_coef_x, *sub_bi_coef_y;
	unsigned char *pImg_left_top, *pImg_left_down;
	unsigned char *pImg_right_top, *pImg_right_down;
	int float_value;
	const short std_1_value = (1 << QRBAR_SHIFTBITS);
	const short std_and_value = ((1 << QRBAR_SHIFTBITS) - 1);
	int res_x0, res_x1;
	int res_xy;
	const unsigned char *pSrc, *pSrc1;

	pImg_left_top = (unsigned char *)malloc(sizeof(char)*desWidth);
	pImg_left_down = (unsigned char *)malloc(sizeof(char)*desWidth);
	pImg_right_top = (unsigned char *)malloc(sizeof(char)*desWidth);
	pImg_right_down = (unsigned char *)malloc(sizeof(char)*desWidth);

	coord_x = (unsigned short *)malloc(sizeof(short)*desWidth);
	coord_y = (unsigned short *)malloc(sizeof(short)*desHeight);
	bi_coef_x = (unsigned char *)malloc(sizeof(char)*desWidth);
	bi_coef_y = (unsigned char *)malloc(sizeof(char)*desHeight);
	sub_bi_coef_x = (unsigned char *)malloc(sizeof(char)*desWidth);
	sub_bi_coef_y = (unsigned char *)malloc(sizeof(char)*desHeight);


	if (coord_x == NULL || coord_y == NULL || bi_coef_x == NULL || bi_coef_y == NULL || sub_bi_coef_x == NULL || sub_bi_coef_y == NULL ||
		pImg_left_top == NULL || pImg_left_down == NULL || pImg_right_top == NULL || pImg_right_down == NULL)
		return;

    nRateW = (srcWidth << QRBAR_SHIFTBITS) / (desWidth);
    nRateH = (srcHeight << QRBAR_SHIFTBITS) / (desHeight);

	for (i = 0; i < desHeight; ++i)
	{
		float_value = i * nRateH;
		bi_coef_y[i] = float_value & std_and_value;
		if (!bi_coef_y[i])
			bi_coef_y[i] = 1;
		sub_bi_coef_y[i] = std_1_value - bi_coef_y[i];
		coord_y[i] = QRBAR_ROUND0(float_value);
		if (coord_y[i] > srcHeight - 2)
			coord_y[i] = srcHeight - 2;
	}

	for (i = 0; i < desWidth; ++i)
	{
		float_value = i * nRateW;
		bi_coef_x[i] = float_value & std_and_value;
		if (!bi_coef_x[i])
			bi_coef_x[i] = 1;
		sub_bi_coef_x[i] = std_1_value - bi_coef_x[i];
		coord_x[i] = QRBAR_ROUND0(float_value);
		if (coord_x[i] > srcWidth - 2)
			coord_x[i] = srcWidth - 2;
	}

	for (i = 0; i < desHeight; i++)
	{
		// pSrc = pSrcImg + coord_y[i] * srcWidth;
		pSrc = pSrcImg + (coord_y[i]+srcOffsetY)*srcStride + srcOffsetX;
		y = coord_y[i];
		for (j = 0; j < desWidth; j++)
		{
			pSrc1 = pSrc;
			pSrc1 += coord_x[j];
			pImg_left_top[j] = *pSrc1;
			pImg_right_top[j] = *(pSrc1 + 1);
			// pSrc1 += srcWidth;
			pSrc1 += srcStride;
			pImg_left_down[j] = *pSrc1;
			pImg_right_down[j] = *(pSrc1 + 1);
		}

#ifdef __aarch64__
		unsigned char *img1, *img2, *img3, *img4;
		unsigned char *outimg;
		unsigned char *coef1, *coef2, *coef3, *coef4;
		unsigned char *coef_x1, *coef_x2;
		uint8x8_t value_y1, value_y2;

		value_y1 = vdup_n_u8(sub_bi_coef_y[i]);
		value_y2 = vdup_n_u8(bi_coef_y[i]);
		img1 = pImg_left_top;
		img2 = pImg_right_top;
		img3 = pImg_left_down;
		img4 = pImg_right_down;
		outimg = pDesImg + i*desWidth;
		coef_x1 = sub_bi_coef_x;
		coef_x2 = bi_coef_x;
		for (j = 0; j < desWidth; j += 8)
		{
			uint8x8_t v1, v2, v3, v4;
			uint8x8_t value_x1, value_x2;
			uint8x8_t bires_x0, bires_x1, bires_xy;
			uint16x8_t multi_value;

			v1 = vld1_u8(img1);
			v2 = vld1_u8(img2);
			v3 = vld1_u8(img3);
			v4 = vld1_u8(img4);

			value_x1 = vld1_u8(coef_x1);
			value_x2 = vld1_u8(coef_x2);
			multi_value = vmull_u8(v1, value_x1);
			multi_value = vmlal_u8(multi_value, v2, value_x2);
			bires_x0 = vshrn_n_u16(multi_value, QRBAR_SHIFTBITS);
			multi_value = vmull_u8(v3, value_x1);
			multi_value = vmlal_u8(multi_value, v4, value_x2);
			bires_x1 = vshrn_n_u16(multi_value, QRBAR_SHIFTBITS);
			multi_value = vmull_u8(bires_x0, value_y1);
			multi_value = vmlal_u8(multi_value, bires_x1, value_y2);
			bires_xy = vshrn_n_u16(multi_value, QRBAR_SHIFTBITS);
			vst1_u8(outimg, bires_xy);

			outimg += 8;
			img1 += 8;
			img2 += 8;
			img3 += 8;
			img4 += 8;
			coef_x1 += 8;
			coef_x2 += 8;
		}
		for (; j < desWidth; j++)
		{
			res_x0 = (((*img1)*sub_bi_coef_x[j] + (*img2)*bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_x1 = (((*img3)*sub_bi_coef_x[j] + (*img4)*bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_xy = res_x0*sub_bi_coef_y[i] + res_x1*bi_coef_y[i];
			*outimg = (res_xy >> QRBAR_SHIFTBITS);
			outimg++;
			img1++;
			img2++;
			img3++;
			img4++;
		}
#else
		for (j = 0; j < desWidth; j++)
		{
			res_x0 = ((pImg_left_top[j] * sub_bi_coef_x[j] + pImg_right_top[j] * bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_x1 = ((pImg_left_down[j] * sub_bi_coef_x[j] + pImg_right_down[j] * bi_coef_x[j]) >> QRBAR_SHIFTBITS);
			res_xy = res_x0*sub_bi_coef_y[i] + res_x1*bi_coef_y[i];
			pDesImg[i*desWidth + j] = (res_xy >> QRBAR_SHIFTBITS);
		}
#endif
	}

	free(coord_x);
	free(coord_y);
	free(bi_coef_x);
	free(bi_coef_y);
	free(sub_bi_coef_x);
	free(sub_bi_coef_y);
	free(pImg_left_top);
	free(pImg_left_down);
	free(pImg_right_top);
	free(pImg_right_down);
}


void bilinear_32f_c1_interp(
  const float* pSrcImg, 
  float* pDesImg, 
  int srcWidth, 
  int srcHeight, 
  int srcOffsetX, 
  int srcOffsetY, 
  int srcStride, 
  int dstWidth, 
  int dstHeight){
	int i, j;
	float nRateW, nRateH;
	int y;
	unsigned short *coord_x, *coord_y;
	float* bi_coef_x,*bi_coef_y;
	float* sub_bi_coef_x,*sub_bi_coef_y;
	float *pImg_left_top, *pImg_left_down;
	float *pImg_right_top, *pImg_right_down;
	float float_value;
	float res_x0, res_x1;
	float res_xy;
	const float *pSrc, *pSrc1;

	pImg_left_top = (float *)malloc(sizeof(float)*dstWidth);
	pImg_left_down = (float *)malloc(sizeof(float)*dstWidth);
	pImg_right_top = (float *)malloc(sizeof(float)*dstWidth);
	pImg_right_down = (float *)malloc(sizeof(float)*dstWidth);

	coord_x = (unsigned short *)malloc(sizeof(short)*dstWidth);
	coord_y = (unsigned short *)malloc(sizeof(short)*dstHeight);
	bi_coef_x = (float *)malloc(sizeof(float)*dstWidth);
	bi_coef_y = (float *)malloc(sizeof(float)*dstHeight);
	sub_bi_coef_x = (float *)malloc(sizeof(float)*dstWidth);
	sub_bi_coef_y = (float *)malloc(sizeof(float)*dstHeight);

	if (coord_x == NULL || coord_y == NULL || bi_coef_x == NULL || bi_coef_y == NULL || sub_bi_coef_x == NULL || sub_bi_coef_y == NULL ||
		pImg_left_top == NULL || pImg_left_down == NULL || pImg_right_top == NULL || pImg_right_down == NULL)
		return;

    // nRateW = (srcWidth << QRBAR_SHIFTBITS) / (dstWidth);
    // nRateH = (srcHeight << QRBAR_SHIFTBITS) / (dstHeight);
	nRateW = float(srcWidth) / float(dstWidth);
	nRateH = float(srcHeight) / float(dstHeight);

	for (i = 0; i < dstHeight; ++i)
	{
		float_value = i * nRateH;
		coord_y[i] = (unsigned short)(float_value);
		bi_coef_y[i] = float_value - coord_y[i];
		sub_bi_coef_y[i] = 1.0f - bi_coef_y[i];
		if (coord_y[i] > srcHeight - 2)
			coord_y[i] = srcHeight - 2;
	}

	for (i = 0; i < dstWidth; ++i)
	{
		float_value = i * nRateW;
		coord_x[i] = (unsigned short)(float_value);
		bi_coef_x[i] = float_value - coord_x[i];
		sub_bi_coef_x[i] = 1.0f - bi_coef_x[i];		
		if (coord_x[i] > srcWidth - 2)
			coord_x[i] = srcWidth - 2;
	}

	for (i = 0; i < dstHeight; i++)
	{
		// pSrc = pSrcImg + coord_y[i] * srcWidth;
		pSrc = pSrcImg + (coord_y[i]+srcOffsetY)*srcStride + srcOffsetX;
		for (j = 0; j < dstWidth; j++)
		{
			pSrc1 = pSrc;
			pSrc1 += coord_x[j];
			pImg_left_top[j] = *pSrc1;
			pImg_right_top[j] = *(pSrc1 + 1);
			// pSrc1 += srcWidth;
			pSrc1 += srcStride;
			pImg_left_down[j] = *pSrc1;
			pImg_right_down[j] = *(pSrc1 + 1);
		}

#ifdef __aarch64__
		float *img1, *img2, *img3, *img4;
		float *outimg;
		float *coef_x1, *coef_x2;
		float32x4_t value_y1, value_y2;
		value_y1 = vdupq_n_f32(sub_bi_coef_y[i]);
		value_y2 = vdupq_n_f32(bi_coef_y[i]);
		img1 = pImg_left_top;
		img2 = pImg_right_top;
		img3 = pImg_left_down;
		img4 = pImg_right_down;
		outimg = pDesImg + i*dstWidth;
		coef_x1 = sub_bi_coef_x;
		coef_x2 = bi_coef_x;

		for(j=0; j<dstWidth; j += 4){
			float32x4_t v1,v2,v3,v4;
			v1 = vld1q_f32(img1);
			v2 = vld1q_f32(img2);
			v3 = vld1q_f32(img3);
			v4 = vld1q_f32(img4);

			float32x4_t value_x1, value_x2;
			value_x1 = vld1q_f32(coef_x1);
			value_x2 = vld1q_f32(coef_x2);

			float32x4_t multi_value;
			float32x4_t bires_x0, bires_x1, bires_xy;

			multi_value = vmulq_f32(v1, value_x1);
			bires_x0 = vmlaq_f32(multi_value, v2, value_x2);

			multi_value = vmulq_f32(v3, value_x1);
			bires_x1 = vmlaq_f32(multi_value, v4, value_x2);

			multi_value = vmulq_f32(bires_x0, value_y1);
			bires_xy = vmlaq_f32(multi_value, bires_x1, value_y2);

			vst1q_f32(outimg, bires_xy);

			outimg += 4;
			img1 += 4;
			img2 += 4;
			img3 += 4;
			img4 += 4;
			coef_x1 += 4;
			coef_x2 += 4;
		}

		for (; j < dstWidth; j++)
		{
			res_x0 = ((*img1)*sub_bi_coef_x[j] + (*img2)*bi_coef_x[j]);
			res_x1 = ((*img3)*sub_bi_coef_x[j] + (*img4)*bi_coef_x[j]);
			res_xy = res_x0*sub_bi_coef_y[i] + res_x1*bi_coef_y[i];
			*outimg = res_xy;
			outimg++;
			img1++;
			img2++;
			img3++;
			img4++;
		}
#else
		for (j = 0; j < dstWidth; j++){
			res_x0 = ((pImg_left_top[j])*sub_bi_coef_x[j] + (pImg_right_top[j])*bi_coef_x[j]);
			res_x1 = ((pImg_left_down[j])*sub_bi_coef_x[j] + (pImg_right_down[j])*bi_coef_x[j]);
			res_xy = res_x0*sub_bi_coef_y[i] + res_x1*bi_coef_y[i];
			pDesImg[i*dstWidth + j] = res_xy;
		}
#endif
	}

	free(coord_x);
	free(coord_y);
	free(bi_coef_x);
	free(bi_coef_y);
	free(sub_bi_coef_x);
	free(sub_bi_coef_y);
	free(pImg_left_top);
	free(pImg_left_down);
	free(pImg_right_top);
	free(pImg_right_down);
}

} /* namespace arm */
} /* namespace math */
} /* namespace eagleeye */
