#ifndef _EAGLEEYE_QUEUE_H_
#define _EAGLEEYE_QUEUE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include <atomic>
#include <deque>
#include <mutex>

namespace eagleeye{
namespace dataflow {

template <class T>
class Queue {
public:  
  using value_type = T;
  using size_type = std::size_t;
  using reference = T &;
  using const_reference = T const &;

  using mutex = std::mutex;

  Queue(){}
  ~Queue(){}
  
  void push (T const & val) {
    std::lock_guard<mutex> lock(mtx_);
    data_.push_back(val);    
  }

  bool try_pop (T & val) noexcept {
    std::lock_guard<mutex> lock(mtx_);
    if (data_.size() == 0) { return false; }

    val = data_.front();
    data_.pop_front();
    return true;
  }

  std::size_t size () const noexcept {
    return data_.size();
  }
private:
  std::deque<T> data_;
  mutex mtx_;
};

}

}
#endif
