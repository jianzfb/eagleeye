#ifndef _EAGLEEYE_UTILOP_H_
#define _EAGLEEYE_UTILOP_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/Tensor.h"
#include <vector>

namespace eagleeye{
// Tensor<float> resize(Tensor<float> tensor, int after_r, int after_c, InterpMethod interp_method=BILINEAR_INTERPOLATION);

template<typename T>
Tensor<T> concat(std::vector<Tensor<T>> tensors, int axis=0);
}

#include "eagleeye/tensorop/tensorop.hpp"
#endif