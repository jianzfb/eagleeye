#ifndef _EAGLEEYE_POOLING_H_
#define _EAGLEEYE_POOLING_H_


#include <algorithm>
#include <string>
#include <vector>

namespace eagleeye {
namespace math {
namespace arm {
// !pooling fp32 Op
void pooling_basic(const float* din,
                   float* dout,
                   int num,
                   int chout,
                   int hout,
                   int wout,
                   int chin,
                   int hin,
                   int win,
                   const std::vector<int>& ksize,
                   const std::vector<int>& strides,
                   const std::vector<int>& paddings,
                   bool global_pooling,
                   bool exclusive,
                   bool adaptive,
                   bool ceil_mode,
                   bool use_quantizer,
                   const std::string& pooling_type);

void pooling_global_max(const float* din,
                        float* dout,
                        int num,
                        int chout,
                        int hout,
                        int wout,
                        int chin,
                        int hin,
                        int win);

void pooling_global_avg(const float* din,
                        float* dout,
                        int num,
                        int chout,
                        int hout,
                        int wout,
                        int chin,
                        int hin,
                        int win);

void pooling1x1s2p0_max(const float* din,
                        float* dout,
                        int num,
                        int chout,
                        int hout,
                        int wout,
                        int chin,
                        int hin,
                        int win,
                        int pad_bottom,
                        int pad_right);

void pooling2x2s2p0_max(const float* din,
                        float* dout,
                        int num,
                        int chout,
                        int hout,
                        int wout,
                        int chin,
                        int hin,
                        int win,
                        int pad_bottom,
                        int pad_right);

void pooling2x2s2p0_avg(const float* din,
                        float* dout,
                        int num,
                        int chout,
                        int hout,
                        int wout,
                        int chin,
                        int hin,
                        int win,
                        bool exclusive,
                        int pad_bottom,
                        int pad_right);

void pooling2x2s2p1_max(const float* din,
                        float* dout,
                        int num,
                        int chout,
                        int hout,
                        int wout,
                        int chin,
                        int hin,
                        int win,
                        int pad_bottom,
                        int pad_right);

void pooling2x2s2p1_avg(const float* din,
                        float* dout,
                        int num,
                        int chout,
                        int hout,
                        int wout,
                        int chin,
                        int hin,
                        int win,
                        bool exclusive,
                        int pad_bottom,
                        int pad_right);

void pooling3x3s1p1_max(const float* din,
                        float* dout,
                        int num,
                        int chout,
                        int hout,
                        int wout,
                        int chin,
                        int hin,
                        int win,
                        int pad_bottom,
                        int pad_right);

void pooling3x3s1p1_avg(const float* din,
                        float* dout,
                        int num,
                        int chout,
                        int hout,
                        int wout,
                        int chin,
                        int hin,
                        int win,
                        bool exclusive,
                        int pad_bottom,
                        int pad_right);

void pooling3x3s2p1_max(const float* din,
                        float* dout,
                        int num,
                        int chout,
                        int hout,
                        int wout,
                        int chin,
                        int hin,
                        int win,
                        int pad_bottom,
                        int pad_right);

void pooling3x3s1p0_max(const float* din,
                        float* dout,
                        int num,
                        int chout,
                        int hout,
                        int wout,
                        int chin,
                        int hin,
                        int win,
                        int pad_bottom,
                        int pad_right);

void pooling3x3s1p0_avg(const float* din,
                        float* dout,
                        int num,
                        int chout,
                        int hout,
                        int wout,
                        int chin,
                        int hin,
                        int win,
                        bool exclusive,
                        int pad_bottom,
                        int pad_right);

void pooling3x3s2p1_avg(const float* din,
                        float* dout,
                        int num,
                        int chout,
                        int hout,
                        int wout,
                        int chin,
                        int hin,
                        int win,
                        bool exclusive,
                        int pad_bottom,
                        int pad_right);

void pooling3x3s2p0_max(const float* din,
                        float* dout,
                        int num,
                        int chout,
                        int hout,
                        int wout,
                        int chin,
                        int hin,
                        int win,
                        int pad_bottom,
                        int pad_right);

void pooling3x3s2p0_avg(const float* din,
                        float* dout,
                        int num,
                        int chout,
                        int hout,
                        int wout,
                        int chin,
                        int hin,
                        int win,
                        bool exclusive,
                        int pad_bottom,
                        int pad_right);

}  // namespace arm
}  // namespace math
}  // namespace paddle
#endif
