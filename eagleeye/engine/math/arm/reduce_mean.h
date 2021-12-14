#ifndef _EAGLEEYE_REDUCE_MEAN_H_
#define _EAGLEEYE_REDUCE_MEAN_H_

namespace eagleeye {
namespace math {
namespace arm {

template <typename T>
void reduce_mean_n(const T* src,
                   T* dst,
                   int num_in,
                   int channel_in,
                   int height_in,
                   int width_in);

template <typename T>
void reduce_mean_c(const T* src,
                   T* dst,
                   int num_in,
                   int channel_in,
                   int height_in,
                   int width_in);

template <typename T>
void reduce_mean_h(const T* src,
                   T* dst,
                   int num_in,
                   int channel_in,
                   int height_in,
                   int width_in);

template <typename T>
void reduce_mean_w(const T* src,
                   T* dst,
                   int num_in,
                   int channel_in,
                   int height_in,
                   int width_in);

template <typename T>
void reduce_mean_nc(const T* src,
                    T* dst,
                    int num_in,
                    int channel_in,
                    int height_in,
                    int width_in);

template <typename T>
void reduce_mean_ch(const T* src,
                    T* dst,
                    int num_in,
                    int channel_in,
                    int height_in,
                    int width_in);

template <typename T>
void reduce_mean_hw(const T* src,
                    T* dst,
                    int num_in,
                    int channel_in,
                    int height_in,
                    int width_in);

template <typename T>
void reduce_mean_all(const T* src,
                     T* dst,
                     int num_in,
                     int channel_in,
                     int height_in,
                     int width_in);

template <typename T>
void mean_grad(const T* out_grad, T* in_grad, int size);

}  // namespace math
}  // namespace arm
}  // namespace paddle

#endif