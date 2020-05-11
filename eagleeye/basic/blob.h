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
    void transfer(EagleeyeRuntime runtime, bool asyn=true) const;
    
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
    void* cpu() const;

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

    /**
     * @brief Get the Runtime Time object
     * 
     * @return EagleeyeRuntimeType 
     */
    EagleeyeRuntimeType getRuntimeType(){
        return this->m_runtime.type();
    }

protected:
    std::vector<int64_t> m_shape;
	std::vector<Range> m_range;

private:
    size_t m_size;
    EagleeyeRuntime m_runtime;
    std::string m_group;
    
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