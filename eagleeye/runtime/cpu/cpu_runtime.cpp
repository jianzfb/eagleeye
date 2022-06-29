#include "eagleeye/runtime/cpu/cpu_runtime.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/engine/thread_pool.h"
#include "eagleeye/port/env.h"
#include <algorithm>
#include <numeric>

namespace eagleeye
{
constexpr int kMinCpuCoresForPerformance = 3;
constexpr int kMaxCpuCoresForPerformance = 5;    
CPURuntime::CPURuntime(){
    this->m_thread_pool = NULL;
}

CPURuntime::~CPURuntime(){
    if(this->m_thread_pool){
        delete m_thread_pool;
    }
}

EagleeyeError CPURuntime::GetCPUMaxFreq(std::vector<float>& cpu_max_freqs){
  return port::Env::Default()->GetCPUMaxFreq(&cpu_max_freqs);
}

EagleeyeError CPURuntime::SchedSetAffinity(CPUAffinityPolicy policy){
    std::vector<float> cpu_max_freqs;
    if (port::Env::Default()->GetCPUMaxFreq(&cpu_max_freqs) != EagleeyeError::EAGLEEYE_NO_ERROR) {
        EAGLEEYE_LOGE("Fail to get cpu max frequencies");
    }

    std::vector<size_t> cores_to_use;
    int thread_count = 1;
    this->GetCPUCoresToUse(cpu_max_freqs, policy, &thread_count, &cores_to_use);
    if (!cores_to_use.empty()) {
        if (port::Env::Default()->SchedSetAffinity(cores_to_use) != EagleeyeError::EAGLEEYE_NO_ERROR) {
            EAGLEEYE_LOGE("Failed to sched_set_affinity");
        }
    }

    return EagleeyeError::EAGLEEYE_NO_ERROR;
}


int CPURuntime::GetCpuCoresForPerfomance(
    const std::vector<CPUFreq> &cpu_freqs,
    const std::function<bool(const float &x, const float &y)> &comp) {
  float total_freq = std::accumulate(cpu_freqs.begin(), cpu_freqs.end(), 0,
                                     [](float accum, CPUFreq cpu_freq) {
                                       return accum + cpu_freq.freq;
                                     });
  int64_t valid_cpu_nums = std::count_if(cpu_freqs.begin(), cpu_freqs.end(),
                                        [](CPUFreq cpu_freq) {
                                          return cpu_freq.freq != 0;
                                        });
  float avg_freq = total_freq / valid_cpu_nums;

  int cores_to_use = 0;
  for (auto cpu_info : cpu_freqs) {
    if ((comp(cpu_info.freq, avg_freq)
        && cores_to_use < kMaxCpuCoresForPerformance)
        || cores_to_use < kMinCpuCoresForPerformance) {
      ++cores_to_use;
    }
  }

  return cores_to_use;
}

ThreadPool* CPURuntime::GetOrCreateThreadPool(const int num_threads, CPUAffinityPolicy policy){
  if(this->m_thread_pool == NULL || this->m_thread_pool->getThreadsNum() != num_threads){
      if(this->m_thread_pool != NULL){
        delete this->m_thread_pool;
      }

      this->m_thread_pool = new ThreadPool(num_threads, policy);    
  }

  return this->m_thread_pool;
}


EagleeyeError CPURuntime::GetCPUCoresToUse(const std::vector<float> &cpu_max_freqs,
                            const CPUAffinityPolicy policy,
                            int *thread_count,
                            std::vector<size_t> *cores) {
  if (cpu_max_freqs.empty()) {
    *thread_count = 1;
    EAGLEEYE_LOGE("CPU core is empty");
    return EagleeyeError::EAGLEEYE_RUNTIME_ERROR;
  }
  *thread_count = std::max(*thread_count, 0);
  const int cpu_count = static_cast<int>(cpu_max_freqs.size());
  if (*thread_count == 0 || *thread_count > cpu_count) {
    *thread_count = cpu_count;
  }

  if (policy != CPUAffinityPolicy::AFFINITY_NONE) {
    std::vector<CPUFreq> cpu_freq(cpu_max_freqs.size());
    for (size_t i = 0; i < cpu_max_freqs.size(); ++i) {
      cpu_freq[i].core_id = i;
      cpu_freq[i].freq = cpu_max_freqs[i];
    }
    if (policy == CPUAffinityPolicy::AFFINITY_POWER_SAVE ||
        policy == CPUAffinityPolicy::AFFINITY_LITTLE_ONLY) {
      std::sort(cpu_freq.begin(),
                cpu_freq.end(),
                [=](const CPUFreq &lhs, const CPUFreq &rhs) {
                  return lhs.freq < rhs.freq;
                });
    } else if (policy == CPUAffinityPolicy::AFFINITY_HIGH_PERFORMANCE ||
        policy == CPUAffinityPolicy::AFFINITY_BIG_ONLY) {
      std::sort(cpu_freq.begin(),
                cpu_freq.end(),
                [](const CPUFreq &lhs, const CPUFreq &rhs) {
                  return lhs.freq > rhs.freq;
                });
    }

    // decide num of cores to use
    int cores_to_use = 0;
    if (policy == CPUAffinityPolicy::AFFINITY_BIG_ONLY) {
      cores_to_use =
          GetCpuCoresForPerfomance(cpu_freq, std::greater_equal<float>());
    } else if (policy == CPUAffinityPolicy::AFFINITY_LITTLE_ONLY) {
      cores_to_use =
          GetCpuCoresForPerfomance(cpu_freq, std::less_equal<float>());
    } else {
      cores_to_use = *thread_count;
    }
    EAGLEEYE_CHECK(cores_to_use > 0, "number of cores to use should > 0");
    cores->resize(static_cast<size_t>(cores_to_use));
    for (int i = 0; i < cores_to_use; ++i) {
      EAGLEEYE_LOGD("Bind thread to core: %d with freq %d",cpu_freq[i].core_id, cpu_freq[i].freq);  
      (*cores)[i] = static_cast<int>(cpu_freq[i].core_id);
    }
    if (*thread_count == 0 || *thread_count > cores_to_use) {
      *thread_count = cores_to_use;
    }
  }

  return EagleeyeError::EAGLEEYE_NO_ERROR;
}
} // namespace eagleeye
