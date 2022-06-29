#include <algorithm>
#include <numeric>
#include "eagleeye/engine/thread_pool.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/runtime/cpu/cpu_runtime.h"
#include "eagleeye/basic/Math.h"
#include "eagleeye/port/env.h"

namespace eagleeye {
constexpr int kThreadPoolSpinWaitTime = 2000000;  // ns
constexpr int kTileCountPerThread = 2;
constexpr int kMaxCostUsingSingleThread = 100;
constexpr int kMinCpuCoresForPerformance = 3;
constexpr int kMaxCpuCoresForPerformance = 5;

enum {
  kThreadPoolNone = 0,
  kThreadPoolInit = 1,
  kThreadPoolRun = 2,
  kThreadPoolShutdown = 4,
  kThreadPoolEventMask = 0x7fffffff
};
class EagleeyeCPU;
ThreadPool::ThreadPool(const int thread_count_hint, const CPUAffinityPolicy policy)
    : event_(kThreadPoolNone),
      count_down_latch_(kThreadPoolSpinWaitTime) {
  int thread_count = thread_count_hint;

  // TODO: 对于线程池的亲和性设置存在问题
  if (port::Env::Default()->GetCPUMaxFreq(&cpu_max_freqs_)
      != EagleeyeError::EAGLEEYE_NO_ERROR) {
    EAGLEEYE_LOGE("Fail to get cpu max frequencies");
  }

  std::vector<size_t> cores_to_use;
  port::Env::Default()->GetCPUDevice()->getRuntime()->GetCPUCoresToUse(cpu_max_freqs_, policy, &thread_count, &cores_to_use);

  if (!cores_to_use.empty()) {
    if (port::Env::Default()->SchedSetAffinity(cores_to_use)
        != EagleeyeError::EAGLEEYE_NO_ERROR) {
      EAGLEEYE_LOGE("Failed to sched_set_affinity");
    }
  }

  default_tile_count_ = thread_count;
  if (thread_count > 1) {
    default_tile_count_ = thread_count * kTileCountPerThread;
  }
  EAGLEEYE_CHECK(default_tile_count_ > 0, "default tile count should > 0");

  threads_ = std::vector<std::thread>(static_cast<size_t>(thread_count));
  thread_infos_ = std::vector<ThreadInfo>(static_cast<size_t>(thread_count));
  for (auto &thread_info : thread_infos_) {
    thread_info.cpu_cores = cores_to_use;
  }
}

ThreadPool::~ThreadPool() {
  // Clear affinity of main thread
  if (!cpu_max_freqs_.empty()) {
    std::vector<size_t> cores(cpu_max_freqs_.size());
    for (size_t i = 0; i < cores.size(); ++i) {
      cores[i] = i;
    }
    port::Env::Default()->SchedSetAffinity(cores);
  }

  Destroy();
}

int ThreadPool::getThreadsNum(){
  return this->threads_.size();
}

void ThreadPool::Init() {
  EAGLEEYE_LOGD("Init thread pool");
  if (threads_.size() <= 1) {
    return;
  }
  count_down_latch_.Reset(static_cast<int>(threads_.size() - 1));
  event_ = kThreadPoolInit;
  for (size_t i = 1; i < threads_.size(); ++i) {
    threads_[i] = std::thread(&ThreadPool::ThreadLoop, this, i);
  }
  count_down_latch_.Wait();

  EAGLEEYE_LOGD("Thread Pool has %d threads",threads_.size());
}

void ThreadPool::Run(const std::function<void(const int64_t)> &func,
                     const int64_t iterations) {
  const size_t thread_count = threads_.size();
  const int64_t iters_per_thread = iterations / thread_count;
  const int64_t remainder = iterations % thread_count;
  int64_t iters_offset = 0;

  std::unique_lock<std::mutex> run_lock(run_mutex_);

  for (size_t i = 0; i < thread_count; ++i) {
    int64_t range_len =
        iters_per_thread + (static_cast<int64_t>(i) < remainder);
    thread_infos_[i].range_start = iters_offset;
    thread_infos_[i].range_len = range_len;
    thread_infos_[i].range_end = iters_offset + range_len;
    thread_infos_[i].func = reinterpret_cast<uintptr_t>(&func);
    iters_offset = thread_infos_[i].range_end;
  }

  count_down_latch_.Reset(static_cast<int>(thread_count - 1));
  {
    std::unique_lock<std::mutex> m(event_mutex_);
    event_.store(kThreadPoolRun | ~(event_ | kThreadPoolEventMask),
                 std::memory_order::memory_order_release);
    event_cond_.notify_all();
  }

  ThreadRun(0);
  count_down_latch_.Wait();
}


void ThreadPool::Run(const std::pair<std::function<void(const int64_t)>, int64_t> &func){
  this->Run(func.first, func.second);
}

void ThreadPool::Destroy() {
  EAGLEEYE_LOGD("Destroy thread pool.");
  if (threads_.size() <= 1) {
    return;
  }

  std::unique_lock<std::mutex> run_lock(run_mutex_);

  count_down_latch_.Wait();
  {
    std::unique_lock<std::mutex> m(event_mutex_);
    event_.store(kThreadPoolShutdown, std::memory_order::memory_order_release);
    event_cond_.notify_all();
  }

  for (size_t i = 1; i < threads_.size(); ++i) {
    if (threads_[i].joinable()) {
      threads_[i].join();
    } else {
        EAGLEEYE_LOGD("Thread: %d not joinable",threads_[i].get_id());
    }
  }
}

// Event is executed synchronously.
void ThreadPool::ThreadLoop(size_t tid) {
  if (!thread_infos_[tid].cpu_cores.empty()) {
    if (port::Env::Default()->SchedSetAffinity(thread_infos_[tid].cpu_cores)
        != EagleeyeError::EAGLEEYE_NO_ERROR) {
      EAGLEEYE_LOGE("Failed to sched set affinity for tid: %d",tid);
    }
  }

  int last_event = kThreadPoolNone;

  for (;;) {
    SpinWait(event_, last_event, kThreadPoolSpinWaitTime);
    if (event_.load(std::memory_order::memory_order_acquire) == last_event) {
      std::unique_lock<std::mutex> m(event_mutex_);
      while (event_ == last_event) {
        event_cond_.wait(m);
      }
    }

    int event = event_.load(std::memory_order::memory_order_acquire);
    switch (event & kThreadPoolEventMask) {
      case kThreadPoolInit: {
        count_down_latch_.CountDown();
        break;
      }

      case kThreadPoolRun: {
        ThreadRun(tid);
        count_down_latch_.CountDown();
        break;
      }

      case kThreadPoolShutdown: return;
      default: break;
    }

    last_event = event;
  }
}

void ThreadPool::ThreadRun(size_t tid) {
  ThreadInfo &thread_info = thread_infos_[tid];
  uintptr_t func_ptr = thread_info.func;
  const std::function<void(int64_t)> *func =
      reinterpret_cast<const std::function<void(int64_t)> *>(func_ptr);
  // do own work
  int64_t range_len;
  while ((range_len = thread_info.range_len) > 0) {
    if (thread_info.range_len.compare_exchange_strong(range_len,
                                                      range_len - 1)) {
      func->operator()(thread_info.range_start++);
    }
  }

  // steal other threads' work
  size_t thread_count = threads_.size();
  for (size_t t = (tid + 1) % thread_count; t != tid;
       t = (t + 1) % thread_count) {
    ThreadInfo &other_thread_info = thread_infos_[t];
    uintptr_t other_func_ptr = other_thread_info.func;
    const std::function<void(int64_t)> *other_func =
        reinterpret_cast<const std::function<void(int64_t)> *>(
            other_func_ptr);
    while ((range_len = other_thread_info.range_len) > 0) {
      if (other_thread_info.range_len.compare_exchange_strong(range_len,
                                                              range_len
                                                                  - 1)) {
        int64_t tail = other_thread_info.range_end--;
        other_func->operator()(tail - 1);
      }
    }
  }
}

void ThreadPool::Compute1D(const std::function<void(int64_t,
                                                    int64_t,
                                                    int64_t)> &func,
                           const int64_t start,
                           const int64_t end,
                           const int64_t step,
                           int64_t tile_size,
                           const int cost_per_item) {
  if (start >= end) {
    return;
  }

  const int64_t items = 1 + (end - start - 1) / step;
  if (threads_.size() <= 1 || (cost_per_item >= 0
      && items * cost_per_item < kMaxCostUsingSingleThread)) {
    func(start, end, step);
    return;
  }

  if (tile_size == 0) {
    tile_size = std::max(static_cast<int64_t>(1), items / default_tile_count_);
  }

  const int64_t step_tile_size = step * tile_size;
  const int64_t tile_count = RoundUpDiv(items, tile_size);
  Run([=](int64_t tile_idx) {
    const int64_t tile_start = start + tile_idx * step_tile_size;
    const int64_t tile_end = std::min(end, tile_start + step_tile_size);
    func(tile_start, tile_end, step);
  }, tile_count);
}

void ThreadPool::Compute2D(const std::function<void(const int64_t,
                                                    const int64_t,
                                                    const int64_t,
                                                    const int64_t,
                                                    const int64_t,
                                                    const int64_t)> &func,
                           const int64_t start0,
                           const int64_t end0,
                           const int64_t step0,
                           const int64_t start1,
                           const int64_t end1,
                           const int64_t step1,
                           int64_t tile_size0,
                           int64_t tile_size1,
                           const int cost_per_item) {
  if (start0 >= end0 || start1 >= end1) {
    return;
  }

  const int64_t items0 = 1 + (end0 - start0 - 1) / step0;
  const int64_t items1 = 1 + (end1 - start1 - 1) / step1;

  // std::cout<<threads_.size()<<std::endl;
  if (threads_.size() <= 1 || (cost_per_item >= 0
      && items0 * items1 * cost_per_item < kMaxCostUsingSingleThread)) {
    func(start0, end0, step0, start1, end1, step1);
    return;
  }

  if (tile_size0 == 0 || tile_size1 == 0) {
    if (items0 >= default_tile_count_) {
      tile_size0 = items0 / default_tile_count_;
      tile_size1 = items1;
    } else {
      tile_size0 = 1;
      tile_size1 = std::max(static_cast<int64_t>(1),
                            items1 * items0 / default_tile_count_);
    }
  }

  const int64_t step_tile_size0 = step0 * tile_size0;
  const int64_t step_tile_size1 = step1 * tile_size1;
  const int64_t tile_count0 = RoundUpDiv(items0, tile_size0);
  const int64_t tile_count1 = RoundUpDiv(items1, tile_size1);

  Run([=](int64_t tile_idx) {
    const int64_t tile_idx0 = tile_idx / tile_count1;
    const int64_t tile_idx1 = tile_idx - tile_idx0 * tile_count1;
    const int64_t tile_start0 = start0 + tile_idx0 * step_tile_size0;
    const int64_t tile_end0 = std::min(end0, tile_start0 + step_tile_size0);
    const int64_t tile_start1 = start1 + tile_idx1 * step_tile_size1;
    const int64_t tile_end1 = std::min(end1, tile_start1 + step_tile_size1);
    func(tile_start0, tile_end0, step0, tile_start1, tile_end1, step1);
  }, tile_count0 * tile_count1);
}

void ThreadPool::Compute3D(const std::function<void(const int64_t,
                                                    const int64_t,
                                                    const int64_t,
                                                    const int64_t,
                                                    const int64_t,
                                                    const int64_t,
                                                    const int64_t,
                                                    const int64_t,
                                                    const int64_t)> &func,
                           const int64_t start0,
                           const int64_t end0,
                           const int64_t step0,
                           const int64_t start1,
                           const int64_t end1,
                           const int64_t step1,
                           const int64_t start2,
                           const int64_t end2,
                           const int64_t step2,
                           int64_t tile_size0,
                           int64_t tile_size1,
                           int64_t tile_size2,
                           const int cost_per_item) {
  if (start0 >= end0 || start1 >= end1 || start2 >= end1) {
    return;
  }

  const int64_t items0 = 1 + (end0 - start0 - 1) / step0;
  const int64_t items1 = 1 + (end1 - start1 - 1) / step1;
  const int64_t items2 = 1 + (end2 - start2 - 1) / step2;
  if (threads_.size() <= 1 || (cost_per_item >= 0
      && items0 * items1 * items2 * cost_per_item
          < kMaxCostUsingSingleThread)) {
    func(start0, end0, step0, start1, end1, step1, start2, end2, step2);
    return;
  }

  if (tile_size0 == 0 || tile_size1 == 0 || tile_size2 == 0) {
    if (items0 >= default_tile_count_) {
      tile_size0 = items0 / default_tile_count_;
      tile_size1 = items1;
      tile_size2 = items2;
    } else {
      tile_size0 = 1;
      const int64_t items01 = items1 * items0;
      if (items01 >= default_tile_count_) {
        tile_size1 = items01 / default_tile_count_;
        tile_size2 = items2;
      } else {
        tile_size1 = 1;
        tile_size2 = std::max(static_cast<int64_t>(1),
                              items01 * items2 / default_tile_count_);
      }
    }
  }

  const int64_t step_tile_size0 = step0 * tile_size0;
  const int64_t step_tile_size1 = step1 * tile_size1;
  const int64_t step_tile_size2 = step2 * tile_size2;
  const int64_t tile_count0 = RoundUpDiv(items0, tile_size0);
  const int64_t tile_count1 = RoundUpDiv(items1, tile_size1);
  const int64_t tile_count2 = RoundUpDiv(items2, tile_size2);
  const int64_t tile_count12 = tile_count1 * tile_count2;

  Run([=](int64_t tile_idx) {
    const int64_t tile_idx0 = tile_idx / tile_count12;
    const int64_t tile_idx12 = tile_idx - tile_idx0 * tile_count12;
    const int64_t tile_idx1 = tile_idx12 / tile_count2;
    const int64_t tile_idx2 = tile_idx12 - tile_idx1 * tile_count2;
    const int64_t tile_start0 = start0 + tile_idx0 * step_tile_size0;
    const int64_t tile_end0 = std::min(end0, tile_start0 + step_tile_size0);
    const int64_t tile_start1 = start1 + tile_idx1 * step_tile_size1;
    const int64_t tile_end1 = std::min(end1, tile_start1 + step_tile_size1);
    const int64_t tile_start2 = start2 + tile_idx2 * step_tile_size2;
    const int64_t tile_end2 = std::min(end2, tile_start2 + step_tile_size2);
    func(tile_start0,
         tile_end0,
         step0,
         tile_start1,
         tile_end1,
         step1,
         tile_start2,
         tile_end2,
         step2);
  }, tile_count0 * tile_count12);
}

}  // namespace mace
