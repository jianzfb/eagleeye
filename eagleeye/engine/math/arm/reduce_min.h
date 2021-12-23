#ifndef _EAGLEEYE_REDUCE_MIN_H_
#define _EAGLEEYE_REDUCE_MIN_H_

namespace eagleeye {
namespace math {
namespace arm {

template <typename T>
void reduce_min_n(const T* src,
                  T* dst,
                  int num_in,
                  int channel_in,
                  int height_in,
                  int width_in);

template <typename T>
void reduce_min_c(const T* src,
                  T* dst,
                  int num_in,
                  int channel_in,
                  int height_in,
                  int width_in);

template <typename T>
void reduce_min_all_of_three(
    const T* src, T* dst, int first_in, int second_in, int third_in);

template <typename T>
void reduce_min_first_of_three(
    const T* src, T* dst, int first_in, int second_in, int third_in);

template <typename T>
void reduce_min_second_of_three(
    const T* src, T* dst, int first_in, int second_in, int third_in);

template <typename T>
void reduce_min_third_of_three(
    const T* src, T* dst, int first_in, int second_in, int third_in);

template <typename T>
void reduce_min_h(const T* src,
                  T* dst,
                  int num_in,
                  int channel_in,
                  int height_in,
                  int width_in);

template <typename T>
void reduce_min_w(const T* src,
                  T* dst,
                  int num_in,
                  int channel_in,
                  int height_in,
                  int width_in);

template <typename T>
void reduce_min_nc(const T* src,
                   T* dst,
                   int num_in,
                   int channel_in,
                   int height_in,
                   int width_in);

template <typename T>
void reduce_min_ch(const T* src,
                   T* dst,
                   int num_in,
                   int channel_in,
                   int height_in,
                   int width_in);

template <typename T>
void reduce_min_hw(const T* src,
                   T* dst,
                   int num_in,
                   int channel_in,
                   int height_in,
                   int width_in);

template <typename T>
void reduce_min_all(const T* src,
                    T* dst,
                    int num_in,
                    int channel_in,
                    int height_in,
                    int width_in);

}  // namespace math
}  // namespace arm
}  // namespace eagleeye

#endif
