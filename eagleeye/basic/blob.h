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
    
    Blob(int64_t size, EagleeyeType data_type, MemoryType memory_type, Aligned aligned, 
         void* data=NULL, 
         bool copy=false);

    /**
     * @brief Construct a new Blob object (support heterogeneous device)
     * 
     * @param size blob size
     * @param aligned  memory aligned bits
     * @param runtime  memory device
     * @param data  data
     * @param copy  whether copy 
     */
    Blob(int64_t h, int64_t w, EagleeyeType data_type, MemoryType memory_type, Aligned aligned, 
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
     * @brief get pointer on GPU
     * 
     * @return void* 
     */
    void* gpu(std::vector<int64_t> image_shape=std::vector<int64_t>{}, bool is_offset=false) const;
    
    template<typename T>
    const T* gpu(std::vector<int64_t> image_shape=std::vector<int64_t>{}, bool is_offset=false) const{
        return static_cast<T*>(this->gpu(image_shape), is_offset);
    }
    template<typename T>
    T* gpu(std::vector<int64_t> image_shape=std::vector<int64_t>{}, bool is_offset=false){
        return static_cast<T*>(this->gpu(image_shape), is_offset);
    }

    /**
     * @brief get pointer on CPU
     * 
     * @return void* 
     */
    void* cpu(bool is_offset=false) const;

    template<typename T>
    const T* cpu(bool is_offset=false) const {
        return static_cast<T*>(this->cpu(is_offset));
    }
    template<typename T>
    T* cpu(bool is_offset=false){
        return static_cast<T*>(this->cpu(is_offset));
    }

    /**
     * @brief get blob size
     * 
     * @return size_t 
     */
    size_t blobsize() const;

    /**
     * @brief get blob offset
     * 
     * @return size_t 
     */
    size_t bloboffset() const;

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
    bool update(void* data,  MemoryType mem_type=CPU_BUFFER, std::string option="");

    /**
     * @brief update 
     * 
     * @return true 
     * @return false 
     */
    bool update();

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
    void reshape(std::vector<int64_t> shape);
    void reshape(Dim dim);

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
     * @brief blob dims
     * 
     */
    Dim m_dims;

    /**
     * @brief data type
     */ 
    EagleeyeType m_data_type;

    /**
     * @brief memory type
     */ 
    mutable MemoryType m_memory_type;               // 初始化后不能更改主存储位置

    /**
     * @brief runtime 
     */ 
    mutable EagleeyeRuntime m_runtime;              // 初始化后不能更改运行平台

    /**
     * @brief image shape (for GPU_IMAGE)
     */ 
    std::vector<int64_t> m_image_shape;
    
protected:
    mutable size_t m_size;
    mutable size_t m_offset;    
    Aligned m_aligned;
    int64_t m_elem_size;

    mutable bool m_waiting_reset_runtime;           // ignore
    mutable EagleeyeRuntime m_waiting_runtime;      // ignore

    mutable std::shared_ptr<unsigned char> m_cpu_data;
    mutable std::shared_ptr<OpenCLObject> m_gpu_data;

    mutable bool m_is_cpu_waiting_from_gpu;
    mutable bool m_is_cpu_ready;
    mutable bool m_is_gpu_waiting_from_cpu;
    mutable bool m_is_gpu_ready;
    mutable std::shared_ptr<spinlock> m_lock;   // lock
};    
}
#endif