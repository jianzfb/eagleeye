#include "eagleeye/engine/math/arm/clip.h"
#include <algorithm>
#include <limits>
#include <memory>
#include <arm_neon.h>

namespace eagleeye {
namespace math {
namespace arm {

void clip_fp32(
    const float* input, int64_t num, float min, float max, float* output) {
  const float* din_ptr = input;
  float* dout_ptr = output;
  int64_t cnt = num >> 4;
  int remain = num % 16;
  float32x4_t max_val = vdupq_n_f32(max);
  float32x4_t min_val = vdupq_n_f32(min);
  for (int64_t n = 0; n < cnt; n++) {
    float32x4_t tmp0 =
        vminq_f32(vmaxq_f32(vld1q_f32(din_ptr), min_val), max_val);
    float32x4_t tmp1 =
        vminq_f32(vmaxq_f32(vld1q_f32(din_ptr + 4), min_val), max_val);
    float32x4_t tmp2 =
        vminq_f32(vmaxq_f32(vld1q_f32(din_ptr + 8), min_val), max_val);
    float32x4_t tmp3 =
        vminq_f32(vmaxq_f32(vld1q_f32(din_ptr + 12), min_val), max_val);
    vst1q_f32(dout_ptr, tmp0);
    vst1q_f32(dout_ptr + 4, tmp1);
    vst1q_f32(dout_ptr + 8, tmp2);
    vst1q_f32(dout_ptr + 12, tmp3);
    din_ptr += 16;
    dout_ptr += 16;
  }
  for (int i = 0; i < remain; i++) {
    float tmp = din_ptr[i] > min ? din_ptr[i] : min;
    dout_ptr[i] = tmp < max ? tmp : max;
  }
}

}  // namespace math
}  // namespace arm
}  // namespace paddle
