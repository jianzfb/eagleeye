#ifndef _EAGLEEYE_REDUCE_MAX_H_
#define _EAGLEEYE_REDUCE_MAX_H_


namespace eagleeye {
namespace math {
namespace arm {

template <typename T>
void reduce_n(const T* src,
              T* dst,
              int num_in,
              int channel_in,
              int height_in,
              int width_in);

template <typename T>
void reduce_c(const T* src,
              T* dst,
              int num_in,
              int channel_in,
              int height_in,
              int width_in);

template <typename T>
void reduce_all_of_three(
    const T* src, T* dst, int first_in, int second_in, int third_in);

template <typename T>
void reduce_first_of_three(
    const T* src, T* dst, int first_in, int second_in, int third_in);

template <typename T>
void reduce_second_of_three(
    const T* src, T* dst, int first_in, int second_in, int third_in);

template <typename T>
void reduce_third_of_three(
    const T* src, T* dst, int first_in, int second_in, int third_in);

template <typename T>
void reduce_h(const T* src,
              T* dst,
              int num_in,
              int channel_in,
              int height_in,
              int width_in);

template <typename T>
void reduce_w(const T* src,
              T* dst,
              int num_in,
              int channel_in,
              int height_in,
              int width_in);

template <typename T>
void reduce_nc(const T* src,
               T* dst,
               int num_in,
               int channel_in,
               int height_in,
               int width_in);

template <typename T>
void reduce_ch(const T* src,
               T* dst,
               int num_in,
               int channel_in,
               int height_in,
               int width_in);

template <typename T>
void reduce_hw(const T* src,
               T* dst,
               int num_in,
               int channel_in,
               int height_in,
               int width_in);

template <typename T>
void reduce_all(const T* src,
                T* dst,
                int num_in,
                int channel_in,
                int height_in,
                int width_in);

}  // namespace math
}  // namespace arm
}  // namespace paddle

#endif