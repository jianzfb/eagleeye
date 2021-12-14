#ifndef _EAGLEEYE_REDUCE_SUM_H_
#define _EAGLEEYE_REDUCE_SUM_H_

namespace eagleeye {
namespace math {
namespace arm {

template <typename T>
void reduce_sum_n(const T* src,
                  T* dst,
                  int num_in,
                  int channel_in,
                  int height_in,
                  int width_in);

template <typename T>
void reduce_sum_c(const T* src,
                  T* dst,
                  int num_in,
                  int channel_in,
                  int height_in,
                  int width_in);

template <typename T>
void reduce_sum_h(const T* src,
                  T* dst,
                  int num_in,
                  int channel_in,
                  int height_in,
                  int width_in);

template <typename T>
void reduce_sum_w(const T* src,
                  T* dst,
                  int num_in,
                  int channel_in,
                  int height_in,
                  int width_in);

template <typename T>
void reduce_sum_nc(const T* src,
                   T* dst,
                   int num_in,
                   int channel_in,
                   int height_in,
                   int width_in);

template <typename T>
void reduce_sum_ch(const T* src,
                   T* dst,
                   int num_in,
                   int channel_in,
                   int height_in,
                   int width_in);

template <typename T>
void reduce_sum_hw(const T* src,
                   T* dst,
                   int num_in,
                   int channel_in,
                   int height_in,
                   int width_in);

template <typename T>
void reduce_sum_all(const T* src, T* dst, int all_size);

}  // namespace math
}  // namespace arm
}  // namespace paddle

#endif
