#ifndef _EAGLEEYE_BLOB_H_
#define _EAGLEEYE_BLOB_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/common/EagleeyeRuntime.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/common/EagleeyeOpenCL.h"
#include "eagleeye/basic/spinlock.hpp"
#include <vector>
#include <map>
#include <memory>

namespace eagleeye{
class Blob{
public:
    /**
     * @brief Construct a new Blob object (support heterogeneous device)
     * 
     * @param size 
     * @param runtime 
     * @param data 
     * @param copy 
     */
    Blob(size_t size, EagleeyeRuntime runtime=EagleeyeRuntime(EAGLEEYE_CPU), void* data=NULL, bool copy=false, std::string group="default");
    
    /**
     * @brief Destroy the Blob object
     * 
     */
    virtual ~Blob();

    /**
     * @brief transfer data to device
     * 
     * @param runtime 
     * @param asyn 
     */
    void transfer(EagleeyeRuntime runtime, bool asyn=true);
    
    /**
     * @brief put to runtime
     * 
     * @param runtime 
     */

    void schedule(EagleeyeRuntime runtime);
    /**
     * @brief get pointer on GPU
     * 
     * @return void* 
     */
    void* gpu();

    /**
     * @brief get pointer on DSP
     * 
     * @return void* 
     */
    void* dsp();

    /**
     * @brief get pointer on CPU
     * 
     * @return void* 
     */
    void* cpu();

    /**
     * @brief update state
     * 
     */
    void update();

    /**
     * @brief get blob size
     * 
     * @return size_t 
     */
    size_t blobsize();

    /**
     * @brief Get the Runtime object
     * 
     * @return EagleeyeRuntime 
     */
    EagleeyeRuntime getRuntime(){
        return this->m_runtime;
    }

protected:
    size_t m_size;
    EagleeyeRuntime m_runtime;
    
    std::vector<int64_t> m_shape;
	std::vector<Range> m_range;
    std::shared_ptr<unsigned char> m_cpu_data;

#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
    std::shared_ptr<OpenCLMem> m_gpu_data;
#endif

    bool m_is_dsp_waiting_from_cpu;
    bool m_is_dsp_waiting_from_gpu;
    bool m_is_dsp_ready;
    bool m_is_cpu_waiting_from_gpu;
    bool m_is_cpu_waiting_from_dsp;
    bool m_is_cpu_ready;
    bool m_is_gpu_waiting_from_cpu;
    bool m_is_gpu_waiting_from_dsp;
    bool m_is_gpu_ready;
    std::shared_ptr<spinlock> m_lock;
    std::string m_group;

};    
}
#endif