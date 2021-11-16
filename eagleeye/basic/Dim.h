#ifndef _EAGLEEYE_DIM_H_
#define _EAGLEEYE_DIM_H_
#include <algorithm>
#include <functional>  // for multiplies
#include <memory>
#include <numeric>
#include <string>
#include <vector>
namespace eagleeye
{
class Dim {
 public:
  using value_type = int64_t;

  Dim() = default;

  explicit Dim(const std::vector<value_type> &x) { ConstructFrom(x); }
  // Dim(std::initializer_list<value_type> init_list) :
  // Dim(std::vector<value_type>(init_list)) {}

  void ConstructFrom(const std::vector<value_type> &x) { data_ = x; }

  value_type operator[](int offset) const { return data_[offset]; }
  value_type &operator[](int offset) { return data_[offset]; }
  std::vector<int64_t> Vectorize() const { return data_; }

  size_t size() const { return data_.size(); }
  bool empty() const { return data_.empty(); }

  value_type production() const;

  const std::vector<value_type> &data() const { return data_; }
  value_type count(int start, int end) const;

  Dim Slice(int start, int end) const;

  Dim Flatten2D(int col) const {
    return Dim(std::vector<value_type>(
        {Slice(0, col).production(), Slice(col, size()).production()}));
  }

  friend bool operator==(const Dim &a, const Dim &b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); i++) {
      if (a[i] != b[i]) return false;
    }
    return true;
  }

  friend bool operator!=(const Dim &a, const Dim &b) {
    if (a.size() != b.size()) return true;
    for (size_t i = 0; i < a.size(); i++) {
      if (a[i] != b[i]) return true;
    }
    return false;
  }

  friend Dim operator &(const Dim& a, const Dim& b){
    std::vector<value_type> data;
    for (size_t i=0; i<a.size(); ++i){
      data.push_back(a[i] < b[i] ? a[i] : b[i]);
    }
    Dim c(data);
    return c;
  }
  
  friend Dim operator |(const Dim& a, const Dim& b){
    std::vector<value_type> data;
    for (size_t i=0; i<a.size(); ++i){
      data.push_back(a[i] > b[i] ? a[i] : b[i]);
    }
    Dim c(data);
    return c;
  }

 private:
  std::vector<value_type> data_;
};
} // namespace eagleeye
#endif