#ifndef _EAGLEEYE_TYPE_TRANS_
#define _EAGLEEYE_TYPE_TRANS_
#include <stdint.h>
#include <vector>

namespace eagleeye {
namespace math {
namespace arm {

template <typename dtype>
void int32_to_dtype(const int* din,
                    dtype* dout,
                    const float* scale,
                    int axis_size,
                    int64_t outer_size,
                    int64_t inner_size);

void fp32_to_int8(const float* din,
                  int8_t* dout,
                  const float* scale,
                  int axis_size,
                  int64_t outer_size,
                  int64_t inner_size);
void fp32_to_uint8(const float* din,
                  uint8_t* dout,
                  const float* scale,
                  int axis_size,
                  int64_t outer_size,
                  int64_t inner_size);
                  
void int8_to_fp32(const int8_t* in,
                  float* out,
                  const float* scale,
                  int axis_size,
                  int64_t outer_size,
                  int64_t inner_size);

}  // namespace math
}  // namespace arm
}  // namespace paddle

#endif