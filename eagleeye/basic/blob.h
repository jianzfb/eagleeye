#ifndef _EAGLEEYE_BLOB_H_
#define _EAGLEEYE_BLOB_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/common/EagleeyeRuntime.h"
#include "eagleeye/basic/spinlock.hpp"
#include <vector>
#include <map>
#include <memory>

namespace eagleeye{
class OpenCLMem;    
class Range
{
public:
	template<typename T> friend class Matrix;

	explicit Range(unsigned int start=0,unsigned int end=0)
		:s(start),e(end){};
	~Range(){};

    static Range ALL();

	unsigned int s;
	unsigned int e;
};

class Aligned{
public:
	Aligned(int aligned_bits):m_aligned_bits(aligned_bits){}
	~Aligned(){};

	int m_aligned_bits;
};

class Blob{
public:
    /**
     * @brief Construct a new Blob object (support heterogeneous device)
     * 
     * @param size blob size
     * @param aligned  memory aligned bits
     * @param runtime  memory device
     * @param data  data
     * @param copy  whether copy 
     * @param group group name
     */
    Blob(size_t size, 
         Aligned aligned=Aligned(64), 
         EagleeyeRuntime runtime=EagleeyeRuntime(EAGLEEYE_CPU), 
         void* data=NULL, 
         bool copy=false, 
         std::string group="default");
    
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
    void transfer(EagleeyeRuntime runtime, bool asyn=true) const;
    
    /**
     * @brief transfer data to device and reset
     * 
     * @param runtime 
     */

    void schedule(EagleeyeRuntime runtime, bool asyn=true);

    /**
     * @brief get pointer on GPU
     * 
     * @return void* 
     */
    void* gpu() const;

    /**
     * @brief get pointer on DSP
     * 
     * @return void* 
     */
    void* dsp() const;

    /**
     * @brief get pointer on CPU
     * 
     * @return void* 
     */
    void* cpu() const;

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

    /**
     * @brief Get the Runtime Time object
     * 
     * @return EagleeyeRuntimeType 
     */
    EagleeyeRuntimeType getRuntimeType(){
        return this->m_runtime.type();
    }

protected:
    /**
     * @brief sync memory between device
     * 
     */
    void _sync() const;

    /**
     * @brief reset state flag 
     * 
     */
    void _reset() const;

    std::vector<int64_t> m_shape;
	std::vector<Range> m_range;

private:
    size_t m_size;
    mutable EagleeyeRuntime m_runtime;
    std::string m_group;
    Aligned m_aligned;

    mutable bool m_waiting_reset_runtime;
    mutable EagleeyeRuntime m_waiting_runtime;

    mutable std::shared_ptr<unsigned char> m_cpu_data;
    mutable std::shared_ptr<OpenCLMem> m_gpu_data;

    mutable bool m_is_dsp_waiting_from_cpu;
    mutable bool m_is_dsp_waiting_from_gpu;
    mutable bool m_is_dsp_ready;
    mutable bool m_is_cpu_waiting_from_gpu;
    mutable bool m_is_cpu_waiting_from_dsp;
    mutable bool m_is_cpu_ready;
    mutable bool m_is_gpu_waiting_from_cpu;
    mutable bool m_is_gpu_waiting_from_dsp;
    mutable bool m_is_gpu_ready;
    mutable std::shared_ptr<spinlock> m_lock;
};    
}
#endif