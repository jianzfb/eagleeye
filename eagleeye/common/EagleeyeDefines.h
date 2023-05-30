#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/engine/thread_pool.h"
#include "eagleeye/port/env.h"
#include "eagleeye/common/EagleeyeRuntime.h"
#include "eagleeye/runtime/cpu/cpu_runtime.h"

/* support basic for loop
 * for (int i = 0; i < work_size; ++i)
 */
#define EAGLEEYE_PARALLEL_BEGIN(index, tid, work_size)      \
  {                                                     \
    std::pair<std::function<void(const int64_t)>, int64_t> task;            \
    task.second = work_size;                                                \
    task.first = [&](const int64_t index) {
#define EAGLEEYE_PARALLEL_END()                       \
  }                                                   \
  ;                                                   \
  eagleeye::port::Env::Default()->GetCPUDevice()->getOrCreateRuntime()->GetThreadPool()->Run(task); \
  }
