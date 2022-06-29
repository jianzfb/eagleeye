#ifndef _EAGLEEYE_CPU_RUNTIME_H_
#define _EAGLEEYE_CPU_RUNTIME_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/engine/thread_pool.h"
#include <vector>
#include <functional>


namespace eagleeye{
struct CPUFreq {
  size_t core_id;
  float freq;
};

class ThreadPool;
class CPURuntime{
public:
    CPURuntime();
    virtual ~CPURuntime();

    /**
     * @brief Get/Create the Thread Pool object
     * 
     * @param num_threads 
     * @param policy 
     * @return ThreadPool* 
     */
    ThreadPool* GetOrCreateThreadPool(const int num_threads, CPUAffinityPolicy policy);
    
    /**
     * @brief Get the Thread Pool object
     * 
     * @return ThreadPool* 
     */
    ThreadPool* GetThreadPool(){
        return this->m_thread_pool;
    };
    /**
     * @brief get cpu max freq about hardware
     * 
     * @param cpu_max_freqs 
     * @return EagleeyeError 
     */
    EagleeyeError GetCPUMaxFreq(std::vector<float>& cpu_max_freqs);

    /**
     * @brief get cpu cores about hardware
     * 
     * @param cpu_max_freqs 
     * @param policy 
     * @param thread_count 
     * @param cores 
     * @return EagleeyeError 
     */
    EagleeyeError GetCPUCoresToUse(const std::vector<float> &cpu_max_freqs,
                                    const CPUAffinityPolicy policy,
                                    int *thread_count,
                                    std::vector<size_t> *cores);

    /**
     * @brief set thread affinity
     * 
     * @param policy 
     * @return EagleeyeError 
     */
    EagleeyeError SchedSetAffinity(CPUAffinityPolicy policy); 

private:
    int GetCpuCoresForPerfomance(const std::vector<CPUFreq> &cpu_freqs, 
                                    const std::function<bool(const float &x, const float &y)> &comp);

    ThreadPool *m_thread_pool;
};    
}
#endif