#include "eagleeye/engine/math/common/argmax.h"
#include <algorithm>
#include <functional>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

namespace eagleeye {
namespace math {
template <typename InType, typename OutType>
void argmax_func(const Tensor& input,
                 const int axis,
                 Tensor& output) {
  auto input_ddim = input.dims();
  auto output_ddim = output.dims();

  const int size = input_ddim[axis];
  const int in_channel = input_ddim.count(axis, input_ddim.size());
  const int out_channel = output_ddim.count(axis, output_ddim.size());
  const int in_stride = input_ddim.count(axis + 1, input_ddim.size());
  const int out_stride = input_ddim.count(0, axis);

  const InType *input_data = input.cpu<InType>();
  OutType* output_data = output.cpu<OutType>();

  for (int n = 0; n < out_stride; n++) {
    for (int k = 0; k < in_stride; k++) {
      const InType *in_ptr = input_data + n * in_channel + k;
      std::vector<std::pair<InType, OutType>> vec;
      vec.resize(size);
      for (int i = 0; i < size; i++) {
        vec[i] = std::make_pair(in_ptr[i * in_stride], i);
      }
      // sort
      std::partial_sort(vec.begin(),
                        vec.begin() + 1,
                        vec.end(),
                        std::greater<std::pair<InType, OutType>>());

      // out
      OutType *out_ptr = output_data + n * out_channel + k;
      *out_ptr = vec[0].second;
    }
  }
}

template void argmax_func<float, int32_t>(const Tensor& input,
                                          const int axis,
                                          Tensor& output);
template void argmax_func<float, int64_t>(const Tensor& input,
                                          const int axis,
                                          Tensor& output);
}  // namespace math
}  // namespace eagleeye
