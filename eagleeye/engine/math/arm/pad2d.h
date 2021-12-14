#ifndef _EAGLEEYE_PAD2D_H_
#define _EAGLEEYE_PAD2D_H_

#include <algorithm>
#include <string>
#include <vector>

namespace eagleeye {
namespace math {
namespace arm {

void pad_constant(const float* din,
                  float* dout,
                  int n,
                  int c,
                  int h,
                  int w,
                  const int pad_top,
                  const int pad_bottom,
                  const int pad_left,
                  const int pad_right,
                  const float pad_value);
void pad_edge(const float* din,
              float* dout,
              int n,
              int c,
              int h,
              int w,
              const int pad_top,
              const int pad_bottom,
              const int pad_left,
              const int pad_right,
              const float pad_value);
void pad_reflect(const float* din,
                 float* dout,
                 int n,
                 int c,
                 int h,
                 int w,
                 const int pad_top,
                 const int pad_bottom,
                 const int pad_left,
                 const int pad_right,
                 const float pad_value);
}  // namespace math
}  // namespace arm
}  // namespace paddle

#endif