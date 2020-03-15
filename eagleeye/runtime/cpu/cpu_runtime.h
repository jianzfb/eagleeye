#ifndef _EAGLEEYE_CPU_RUNTIME_H_
#define _EAGLEEYE_CPU_RUNTIME_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/common/EagleeyeThreadPool.h"

namespace eagleeye{
struct CPUFreq {
  size_t core_id;
  float freq;
};

class CPURuntime{
public:
    CPURuntime(const int num_threads, 
                    CPUAffinityPolicy policy);
    virtual ~CPURuntime();

    int num_threads() const {
        if(this->m_thread_pool){
            return this->m_thread_pool->getThreadsNum();
        }
        return 0;
    }

    CPUAffinityPolicy policy() const {
        return m_policy;
    }

    ThreadPool* getThreadPool(){
        return m_thread_pool;
    }

    EagleeyeError GetCPUCoresToUse(const std::vector<float> &cpu_max_freqs,
                                    const CPUAffinityPolicy policy,
                                    int *thread_count,
                                    std::vector<size_t> *cores);
                                    
private:
    int GetCpuCoresForPerfomance(const std::vector<CPUFreq> &cpu_freqs, 
                                    const std::function<bool(const float &x, const float &y)> &comp);

    ThreadPool *m_thread_pool;
    CPUAffinityPolicy m_policy;
};    
}
#endif