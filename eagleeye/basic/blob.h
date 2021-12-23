#ifndef _EAGLEEYE_BLOB_H_
#define _EAGLEEYE_BLOB_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/spinlock.hpp"
#include "eagleeye/common/EagleeyeRuntime.h"
#include "eagleeye/basic/Dim.h"
#include <vector>
#include <map>
#include <memory>


namespace eagleeye{
class OpenCLObject;
class OpenCLKernelGroup;
class Range
{
public:
	template<typename T> friend class Matrix;

	explicit Range(int start=0, int end=0)
		:s(start),e(end){};
	~Range(){};

    static Range ALL();

	int s;
	int e;
};

class Aligned{
public:
	Aligned(int aligned_bits=64):m_aligned_bits(aligned_bits){}
	~Aligned(){};

	int m_aligned_bits;
};

enum MemoryType{
    GPU_IMAGE = 0,
    GPU_BUFFER,
    CPU_BUFFER
};

class Blob{
public:
    /**
     * @brief empty constrcture
     */ 
    Blob();
    
    /**
     * @brief Construct a new Blob object (support heterogeneous device)
     * 
     * @param size blob size
     * @param aligned  memory aligned bits
     * @param runtime  memory device
     * @param data  data
     * @param copy  whether copy 
     */
    Blob(size_t size, 
         Aligned aligned, 
         EagleeyeRuntime runtime=EagleeyeRuntime(EAGLEEYE_CPU), 
         void* data=NULL, 
         bool copy=false);
    
    /**
     * @brief create blob by TEXTURE2D
     */ 
    Blob(unsigned int texture_id);

    /**
     * @brief create blob for tensor
     */ 
    Blob(std::vector<int64_t> shape, 
         EagleeyeType data_type, 
         MemoryType memory_type, 
         std::vector<int64_t> image_shape,
         Aligned aligned=Aligned(64),
         void* data=NULL);

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
    void transfer(EagleeyeRuntime runtime, bool asyn=true, std::vector<int64_t> image_shape=std::vector<int64_t>{}) const;
    
    /**
     * @brief transfer data to device and reset
     * 
     * @param runtime 
     */

    void schedule(EagleeyeRuntime runtime, bool asyn=true, std::vector<int64_t> image_shape=std::vector<int64_t>{});

    /**
     * @brief get pointer on GPU
     * 
     * @return void* 
     */
    void* gpu(std::vector<int64_t> image_shape=std::vector<int64_t>{}) const;

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
    size_t blobsize() const;

    /**
     * @brief get data type
     * 
     * @return EagleeyeType 
     */
    EagleeyeType type() const;

    /**
     * @brief get mem type
     * 
     * @return MemoryType 
     */
    MemoryType memType() const;

    /**
     * @brief Get the Runtime object
     * 
     * @return EagleeyeRuntime 
     */
    EagleeyeRuntime getRuntime() const{
        return this->m_runtime;
    }

    /**
     * @brief Get the Runtime Time object
     * 
     * @return EagleeyeRuntimeType 
     */
    EagleeyeRuntimeType getRuntimeType() const{
        return this->m_runtime.type();
    }

    /**
     * @brief check blob is empty
     * 
     * @return true 
     * @return false 
     */
    bool empty() const;

    /**
     * @brief update data
     * 
     * @param data 
     * @return true 
     * @return false 
     */
    bool update(void* data=NULL, 
                MemoryType mem_type=CPU_BUFFER,
                std::string option="");

    /**
     * @brief Get the Image Shape object (GPU_IMAGE)
     * 
     * @return std::vector<int64_t> 
     */
    std::vector<int64_t> getImageShape() const;

    /**
     * @brief get tensor shape
     */ 
    std::vector<int64_t> shape() const;

    /**
     * @brief reshape tensor
     * 
     * @param s 
     */
    void reshape(std::vector<int64_t> s);

    /**
     * @brief get blob dims
     * 
     * @return const Dim& 
     */
    const Dim &dims() const { return m_dims; }

    /**
     * @brief get element number
     * 
     * @return int64_t 
     */
    int64_t numel() const { return m_dims.production(); }

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

    /**
     * @brief buffer size
     * 
     * @return size_t 
     */
    virtual size_t buffersize() const;

    /**
     * @brief blob shape
     * 
     */
    std::vector<int64_t> m_shape;

    /**
     * @brief blob dims
     * 
     */
    Dim m_dims;

    /**
     * @brief blob range
     * 
     */
	std::vector<Range> m_range;

    /**
     * @brief data type
     */ 
    EagleeyeType m_data_type;

    /**
     * @brief memory type
     */ 
    mutable MemoryType m_memory_type;

    /**
     * @brief runtime 
     */ 
    mutable EagleeyeRuntime m_runtime;

    /**
     * @brief image shape (for GPU_IMAGE)
     */ 
    std::vector<int64_t> m_image_shape;
    
protected:
    size_t m_size;
    Aligned m_aligned;

    mutable bool m_waiting_reset_runtime;
    mutable EagleeyeRuntime m_waiting_runtime;

    mutable std::shared_ptr<unsigned char> m_cpu_data;
    mutable std::shared_ptr<OpenCLObject> m_gpu_data;

    mutable bool m_is_cpu_waiting_from_gpu;
    mutable bool m_is_cpu_ready;
    mutable bool m_is_gpu_waiting_from_cpu;
    mutable bool m_is_gpu_ready;
    mutable std::shared_ptr<spinlock> m_lock;
};    
}
#endif