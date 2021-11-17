#include "eagleeye/basic/Dim.h"
namespace eagleeye
{
using value_type = int64_t;

value_type Dim::production() const {
  value_type res = 1;
  for (size_t i = 0; i < data_.size(); i++) {
    res *= data_[i];
  }
  return res;
}

value_type Dim::count(int start, int end) const {
  start = std::max(start, 0);
  end = std::min(end, static_cast<int>(data_.size()));
  if (end < start) {
    return 0;
  }
  value_type sum = 1;
  for (auto i = start; i < end; ++i) {
    sum *= data_[i];
  }
  return sum;
}

Dim Dim::Slice(int start, int end) const {
  start = std::max(start, 0);
  end = std::min(end, static_cast<int>(data_.size()));
  std::vector<value_type> new_dim(end - start);
  for (int i = start; i < end; i++) {
    new_dim[i - start] = data_[i];
  }
  return Dim(new_dim);
}
} // namespace eagleeye
