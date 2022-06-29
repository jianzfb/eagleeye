#ifndef _EAGLEEYE_MATH_COMPARE_COMPUTE_
#define _EAGLEEYE_MATH_COMPARE_COMPUTE_
#include <algorithm>
#include <string>
#include <vector>
#include <math.h>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <vector>
#include "eagleeye/basic/Tensor.h"

namespace eagleeye {
namespace math {
#define COMPARE_FUNCTOR(name, op)                                           \
  template <typename T>                                                     \
  struct _##name##Functor {                                                 \
    using TYPE = T;                                                         \
    inline bool operator()(const T &a, const T &b) const { return a op b; } \
  };

COMPARE_FUNCTOR(Equal, ==);
COMPARE_FUNCTOR(NotEqual, !=);
COMPARE_FUNCTOR(LessThan, <);
COMPARE_FUNCTOR(LessEqual, <=);
COMPARE_FUNCTOR(GreaterThan, >);
COMPARE_FUNCTOR(GreaterEqual, >=);

template <>
struct _EqualFunctor<float> {
  using TYPE = float;
  inline bool operator()(const float &a, const float &b) const {
    // It is safe to cast a and b to double.
    return fabs(static_cast<double>(a - b)) < 1e-8;
  }
};

template <>
struct _NotEqualFunctor<float> {
  using TYPE = float;
  inline bool operator()(const float &a, const float &b) const {
    return !_EqualFunctor<float>()(a, b);
  }
};

template <typename PType, typename CompareFunctor>
void CompareComputeRun(const Tensor& X, const Tensor& Y, Tensor& Out, int64_t broadcast_axis=-1);
}  // namespace math
}  // namespace eagleeye

#include "eagleeye/engine/math/common/compare_compute.hpp"
#endif