#ifndef _EAGLEEYE_SPINLOCK_H_
#define _EAGLEEYE_SPINLOCK_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include <atomic>
#include <thread>

namespace eagleeye{
class spinlock {
public:
  void lock () {
    while (f_.test_and_set(std::memory_order_acquire)) {
//       std::this_thread::yield();
    }
  }

  bool try_lock () {
    return f_.test_and_set(std::memory_order_acquire);
  }

  void unlock () {
    f_.clear(std::memory_order_release);
  }
private:
  std::atomic_flag f_ = ATOMIC_FLAG_INIT;  
};

inline void SpinWait(const std::atomic<int> &variable,
                     const int value,
                     const int64_t spin_wait_max_time = -1) {
  auto start_time = std::chrono::high_resolution_clock::now();
  for (size_t k = 1; variable.load(std::memory_order_acquire) == value; ++k) {
    if (spin_wait_max_time > 0 && k % 1000 == 0) {
      auto end_time = std::chrono::high_resolution_clock::now();
      int64_t elapse =
          std::chrono::duration_cast<std::chrono::nanoseconds>(
              end_time - start_time).count();
      if (elapse > spin_wait_max_time) {
        break;
      }
    }
  }
}

inline void SpinWaitUntil(const std::atomic<int> &variable,
                          const int value,
                          const int64_t spin_wait_max_time = -1) {
  auto start_time = std::chrono::high_resolution_clock::now();
  for (size_t k = 1; variable.load(std::memory_order_acquire) != value; ++k) {
    if (spin_wait_max_time > 0 && k % 1000 == 0) {
      auto end_time = std::chrono::high_resolution_clock::now();
      int64_t elapse =
          std::chrono::duration_cast<std::chrono::nanoseconds>(
              end_time - start_time).count();
      if (elapse > spin_wait_max_time) {
        break;
      }
    }
  }
}
}
#endif

