#ifndef _EAGLEEYE_MATH_ARGMAX_
#define _EAGLEEYE_MATH_ARGMAX_
#include <algorithm>
#include <string>
#include <vector>
#include "eagleeye/basic/Tensor.h"


namespace eagleeye {
namespace math {
template <typename InType, typename OutType>
void argmax_func(const Tensor& input,
                 const int axis,
                 Tensor& output);
}  // namespace math
}  // namespace eagleeye

#endif