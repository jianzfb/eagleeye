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

  Queue ()
  : data_(segment_size()) {
    first_ = 0;
    last_ = 0;
  }

  void push (T const & val) {
    if (size() < capacity() - 1) {
      data_[last_] = val;
    } else {
      std::lock_guard<std::mutex> lk(lock_);
      if (size() == capacity() - 1) {
        extend(segment_size());
      }
      data_[last_] = val;
    }
    last_ = (last_ + 1) % capacity();
  }

  void push (T && val) {
    if (size() < capacity() - 1) {
      data_[last_] = std::move(val);
    } else {
      std::lock_guard<std::mutex> lk(lock_);
      if (size() == capacity() - 1) {
        extend(segment_size());
      }
      data_[last_] = std::move(val);
    }
    last_ = (last_ + 1) % capacity();
  }

  bool try_pop (T & val) noexcept {
    if (!lock_.try_lock()) {
      return false;
    }
    if (size() == 0) {
      lock_.unlock();
      return false;
    }

    val = std::move(data_[(first_++) % capacity()]);
    lock_.unlock();
    return true;
  }

  std::size_t size () const noexcept {
    return (last_ + capacity() - first_) % capacity();
  }

  std::size_t capacity () const noexcept {
    return data_.size();
  }
private:
  std::deque<T> data_;
  std::atomic_size_t first_;
  std::atomic_size_t last_;
  std::mutex lock_;

  void extend (std::size_t size) {
    data_.insert(data_.begin() + last_, size, T());
    if (first_ > last_) {
      first_ += size;
    }
  }

  std::size_t segment_size () const noexcept {
    return 64;
  }
};

}

}
#endif
