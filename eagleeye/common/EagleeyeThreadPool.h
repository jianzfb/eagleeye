#ifndef _EAGLEEYE_THREADPOOL_H_
#define _EAGLEEYE_THREADPOOL_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include <functional>
#include <condition_variable>  // NOLINT(build/c++11)
#include <mutex>  // NOLINT(build/c++11)
#include <thread>  // NOLINT(build/c++11)
#include <vector>
#include <atomic>
#include "eagleeye/basic/count_down_latch.h"

namespace eagleeye{
class ThreadPool {
 public:
  ThreadPool(const int thread_count,
             const CPUAffinityPolicy affinity_policy);
  ~ThreadPool();

  void Init();

  void Run(const std::function<void(const int64_t)> &func,
           const int64_t iterations);

  int getThreadsNum();

  void Compute1D(const std::function<void(int64_t /* start */,
                                          int64_t /* end */,
                                          int64_t /* step */)> &func,
                 int64_t start,
                 int64_t end,
                 int64_t step,
                 int64_t tile_size = 0,
                 int cost_per_item = -1);

  void Compute2D(const std::function<void(int64_t /* start */,
                                          int64_t /* end */,
                                          int64_t /* step */,
                                          int64_t /* start */,
                                          int64_t /* end */,
                                          int64_t /* step */)> &func,
                 int64_t start0,
                 int64_t end0,
                 int64_t step0,
                 int64_t start1,
                 int64_t end1,
                 int64_t step1,
                 int64_t tile_size0 = 0,
                 int64_t tile_size1 = 0,
                 int cost_per_item = -1);

  void Compute3D(const std::function<void(int64_t /* start */,
                                          int64_t /* end */,
                                          int64_t /* step */,
                                          int64_t /* start */,
                                          int64_t /* end */,
                                          int64_t /* step */,
                                          int64_t /* start */,
                                          int64_t /* end */,
                                          int64_t /* step */)> &func,
                 int64_t start0,
                 int64_t end0,
                 int64_t step0,
                 int64_t start1,
                 int64_t end1,
                 int64_t step1,
                 int64_t start2,
                 int64_t end2,
                 int64_t step2,
                 int64_t tile_size0 = 0,
                 int64_t tile_size1 = 0,
                 int64_t tile_size2 = 0,
                 int cost_per_item = -1);

 private:
  void Destroy();
  void ThreadLoop(size_t tid);
  void ThreadRun(size_t tid);

  std::atomic<int> event_;
  CountDownLatch count_down_latch_;

  std::mutex event_mutex_;
  std::condition_variable event_cond_;
  std::mutex run_mutex_;

  struct ThreadInfo {
    std::atomic<int64_t> range_start;
    std::atomic<int64_t> range_end;
    std::atomic<int64_t> range_len;
    uintptr_t func;
    std::vector<size_t> cpu_cores;
  };
  std::vector<ThreadInfo> thread_infos_;
  std::vector<std::thread> threads_;
  std::vector<float> cpu_max_freqs_;

  int64_t default_tile_count_;
};    
}
#endif