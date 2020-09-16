#include "eagleeye/basic/blob.h"
#include "eagleeye/common/EagleeyeOpenCL.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/basic/Math.h"
namespace eagleeye{
Range Range::ALL(){
    return Range(-1, -1);
}

Blob::Blob(size_t size, 
            Aligned aligned, 
            EagleeyeRuntime runtime,
            void* data,
            bool copy, 
            std::string group)
    :m_size(size),
    m_is_cpu_ready(false),
    m_is_cpu_waiting_from_gpu(false),
    m_is_gpu_ready(false),
    m_is_gpu_waiting_from_cpu(false),
    m_group(group),
    m_aligned(aligned),
    m_waiting_reset_runtime(false),
    m_waiting_runtime(EAGLEEYE_UNKNOWN_RUNTIME){
    m_runtime = runtime;
#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
    this->m_buffer_to_image_kernel = NULL;
    this->m_image_to_buffer_kernel = NULL;
#endif

    if(m_size < 0){
        EAGLEEYE_LOGE("blob size < 0");
        return;
    }

    if(m_size == 0){
        // dont apply memory
        return;
    }

    m_data_type = EAGLEEYE_UNDEFINED;
    // spin lock
    m_lock = std::shared_ptr<spinlock>(new spinlock(), [](spinlock* d) { delete d; });

    // apply device memory
    if(runtime.type() == EAGLEEYE_CPU){
        this->m_memory_type = CPU_BUFFER;
        this->m_gpu_memory_type = GPU_BUFFER;
        if(data == NULL){
            // allocate memory
            this->m_cpu_data = 
                    std::shared_ptr<unsigned char>(new unsigned char[m_size], 
                    [](unsigned char* arr) { delete [] arr; });
            memset(this->m_cpu_data.get(), 0, m_size);
        }
        else if(copy){
            // allocate and copy memory
            this->m_cpu_data = 
                    std::shared_ptr<unsigned char>(new unsigned char[m_size], 
                    [](unsigned char* arr) { delete [] arr; });
            memcpy(this->m_cpu_data.get(), data, size);		
        }
        else{
            // share cpu memory
            this->m_cpu_data = 
                        std::shared_ptr<unsigned char>((unsigned char*)data, 
                        [](unsigned char* arr){});
        }
    }
    else if(runtime.type() == EAGLEEYE_GPU){
#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
        this->m_memory_type = GPU_BUFFER;
        this->m_gpu_memory_type = GPU_BUFFER;
        if(data == NULL){
            // use gpu buffer
            this->m_gpu_data = 
                    std::shared_ptr<OpenCLObject>(new OpenCLMem(EAGLEEYE_CL_MEM_READ_WRITE, "t", size), 
                                                [](OpenCLObject* arr) { delete arr; });      
        }
        else{
            // allocate and copy GPU memory
            if(!copy){
                EAGLEEYE_LOGE("blob dont support share gpu memory");
            }
            
            this->m_gpu_data = 
                    std::shared_ptr<OpenCLMem>(new OpenCLMem(EAGLEEYE_CL_MEM_READ_WRITE, "t", size), 
                                                [](OpenCLMem* arr) { delete arr; });        
            // copy to device
            this->m_gpu_data->copyToDeviceFromDevice(OpenCLRuntime::getOpenCLEnv()->getCommandQueue(m_group), data, CL_TRUE);
        }
#endif
    }
}

Blob::Blob(std::vector<int64_t> shape, 
         EagleeyeType data_type, 
         MemoryType memory_type, 
         std::vector<int64_t> image_shape,
         std::string buffer_to_image_transform,
         std::string image_to_buffer_transform,
         Aligned aligned, 
         std::string group){
    if(shape.size() == 0){
        this->m_size = 0;
        return;
    }

    switch (data_type)
    {
    case EAGLEEYE_CHAR:
    case EAGLEEYE_UCHAR:
        m_size = sizeof(unsigned char)*std::accumulate(shape.begin(), shape.end(), 1, [](int64_t a, int64_t b){return a*b;});
        break;
    case EAGLEEYE_FLOAT:
        m_size = sizeof(float)*std::accumulate(shape.begin(), shape.end(), 1, [](int64_t a, int64_t b){return a*b;});
        break;
    default:
        EAGLEEYE_LOGE("dont support %d", data_type);
        return;
    }

    if(m_size == 0){
        return;
    }

    m_is_cpu_ready = false;
    m_is_cpu_waiting_from_gpu = false;
    m_is_gpu_ready = false;
    m_is_gpu_waiting_from_cpu = false;
    m_group = group;
    m_aligned = aligned;
    m_waiting_reset_runtime = false;
    m_waiting_runtime = EAGLEEYE_UNKNOWN_RUNTIME;
    m_data_type = data_type;
    m_memory_type = memory_type;
    m_image_shape = image_shape;                                // image shape (GPU_IMAGE)
    m_buffer_to_image_transform = buffer_to_image_transform;    // transfer GPU_BUFFER to GPU_IMAGE
    m_image_to_buffer_transform = image_to_buffer_transform;    // transfer GPU_IMAGE to GPU_BUFFER

#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
    m_buffer_to_image_kernel = NULL;
    m_image_to_buffer_kernel = NULL;
#endif    

    if(memory_type == CPU_BUFFER){
        m_runtime = EagleeyeRuntime(EAGLEEYE_CPU);
        m_gpu_memory_type = GPU_BUFFER;
    }
    else{
        m_runtime = EagleeyeRuntime(EAGLEEYE_GPU);
        m_gpu_memory_type = memory_type;
    }

    // spin lock
    this->m_lock = std::shared_ptr<spinlock>(new spinlock(), [](spinlock* d) { delete d; });

    // apply device memory
    if(this->m_runtime.type() == EAGLEEYE_CPU){
        // allocate memory
        size_t mem_size = this->buffersize();
        this->m_cpu_data = 
                std::shared_ptr<unsigned char>(new unsigned char[mem_size], 
                [](unsigned char* arr) { delete [] arr; });
        memset(this->m_cpu_data.get(), 0, mem_size);
    }
    else if(this->m_runtime.type() == EAGLEEYE_GPU){
#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
        // allocate GPU memory
        if(m_memory_type == GPU_BUFFER){
            // use gpu buffer
            this->m_gpu_data = 
                    std::shared_ptr<OpenCLObject>(
                        new OpenCLMem(EAGLEEYE_CL_MEM_READ_WRITE, 
                                    "t", 
                                    m_size), [](OpenCLObject* arr) { delete arr; });      
        }
        else{
            // use gpu image
            if(this->m_image_shape.size() > 0){
                this->m_gpu_data = 
                    std::shared_ptr<OpenCLObject>(
                        new OpenCLImage(EAGLEEYE_CL_MEM_READ_WRITE,
                                        "t", 
                                        this->m_image_shape[1], 
                                        this->m_image_shape[0], 
                                        4, 
                                        this->m_data_type),[](OpenCLObject* arr) { delete arr; });
            }
        }
#endif
    }
}

Blob::~Blob(){
#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
    if(this->m_buffer_to_image_kernel){
        delete this->m_buffer_to_image_kernel;
    }
    if(this->m_image_to_buffer_kernel){
        delete this->m_image_to_buffer_kernel;
    }
#endif
}

void Blob::_reset() const{
    if(this->m_waiting_reset_runtime){
        this->m_runtime = this->m_waiting_runtime;
        this->m_waiting_runtime = EagleeyeRuntime(EAGLEEYE_UNKNOWN_RUNTIME);
        this->m_waiting_reset_runtime = false;
    }

    this->m_is_cpu_waiting_from_gpu = false;
    this->m_is_cpu_ready = false;
    this->m_is_gpu_waiting_from_cpu = false;
    this->m_is_gpu_ready = false;
}

void Blob::_sync() const{
    if(this->m_is_cpu_waiting_from_gpu){
        // gpu -> cpu
    #ifdef EAGLEEYE_OPENCL_OPTIMIZATION  
        // 同步
        this->m_gpu_data.get()->finish(OpenCLRuntime::getOpenCLEnv()->getCommandQueue(m_group));
        // 设置
        m_is_cpu_ready = true;
        m_is_cpu_waiting_from_gpu = false;
    #endif
    }
    else if(this->m_is_gpu_waiting_from_cpu){
        // cpu -> gpu
    #ifdef EAGLEEYE_OPENCL_OPTIMIZATION  
        // 同步
        this->m_gpu_data.get()->finish(OpenCLRuntime::getOpenCLEnv()->getCommandQueue(m_group));
        // 设置
        m_is_gpu_waiting_from_cpu = false;
        m_is_gpu_ready = true;
    #endif
    }
    else{
        // do nothing
    }
}

void Blob::transfer(EagleeyeRuntime runtime, bool asyn) const{
    // check whether in reset process
    m_lock->lock();
    if(this->m_waiting_reset_runtime){
        // wating reset runtime
        this->_sync();
        this->_reset();
    }

    // check whether runtime is same
    if(runtime.type() == m_runtime.type()){
        m_lock->unlock();
        return;
    }

    switch (runtime.type())
    {
    case EAGLEEYE_CPU:
        // has done
        if(m_is_cpu_waiting_from_gpu || m_is_cpu_ready){
            break;
        }
        // transfer from Source to CPU
        switch (m_runtime.type())
        {
        case EAGLEEYE_GPU:
    #ifdef EAGLEEYE_OPENCL_OPTIMIZATION        
            if(this->m_cpu_data.get() == NULL){
                const size_t mem_size = this->buffersize();
                this->m_cpu_data = 
                        std::shared_ptr<unsigned char>(new unsigned char[mem_size], 
                                                        [](unsigned char* arr) { delete [] arr; });
            }      
            if(asyn){
                this->m_gpu_data.get()->copyToHost(OpenCLRuntime::getOpenCLEnv()->getCommandQueue(m_group), this->m_cpu_data.get(), CL_FALSE);
                this->m_is_cpu_waiting_from_gpu = true;
                this->m_is_cpu_ready = false;
            }      
            else{
                this->m_gpu_data.get()->copyToHost(OpenCLRuntime::getOpenCLEnv()->getCommandQueue(m_group), this->m_cpu_data.get(), CL_TRUE);
                this->m_is_cpu_waiting_from_gpu = false;
                this->m_is_cpu_ready = true;
            }
    #endif
            break;
        default:
            break;
        }
        break;
    case EAGLEEYE_GPU:
        // has done
        if(m_is_gpu_waiting_from_cpu || m_is_gpu_ready){
            break;
        }

        // transfer from Source to CPU
    #ifdef EAGLEEYE_OPENCL_OPTIMIZATION    
        switch (m_runtime.type())
        {
        case EAGLEEYE_CPU:
            if(this->m_gpu_data.get() == NULL){
                this->m_gpu_data = 
                        std::shared_ptr<OpenCLMem>(new OpenCLMem(EAGLEEYE_CL_MEM_READ_WRITE, "t", m_size), 
                                                    [](OpenCLMem* arr) { delete arr; });
            }
            if(asyn){
                this->m_gpu_data.get()->copyToDevice(OpenCLRuntime::getOpenCLEnv()->getCommandQueue(m_group), this->m_cpu_data.get(), CL_FALSE);
                this->m_is_gpu_waiting_from_cpu = true;
                this->m_is_gpu_ready = false;
            }
            else{
                this->m_gpu_data.get()->copyToDevice(OpenCLRuntime::getOpenCLEnv()->getCommandQueue(m_group), this->m_cpu_data.get(), CL_TRUE);
                this->m_is_gpu_waiting_from_cpu = false;
                this->m_is_gpu_ready = true;
            }
            break;        
        default:
            break;
        }
    #endif
        break;
    default:
        break;
    }
    // unlock
    m_lock->unlock();
}

void* Blob::gpu() const{
#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
    // 0.step do nothing
    if(m_size <= 0){
        return NULL;
    }

    // check whether in reset process
    m_lock->lock();
    if(this->m_waiting_reset_runtime){
        // wating reset runtime
        this->_sync();
        this->_reset();
    }

    // 1.step 直接返回
    if(m_runtime.type() == EAGLEEYE_GPU){
        m_lock->unlock();
        return (void*)(this->m_gpu_data.get()->getObject());
    }

    // 2.step 等待来自cpu同步完毕
    if(m_is_gpu_waiting_from_cpu){
        // 同步
        this->m_gpu_data.get()->finish(OpenCLRuntime::getOpenCLEnv()->getCommandQueue(m_group));
        // 设置
        m_is_gpu_waiting_from_cpu = false;
        m_is_gpu_ready = true;
    }

    // unlock
    m_lock->unlock();

    if(m_is_gpu_ready){
        return (void*)(this->m_gpu_data.get()->getObject());
    }

    // 4.step 同步调用
    this->transfer(EagleeyeRuntime(EAGLEEYE_GPU), false);
    return (void*)(this->m_gpu_data.get()->getObject());
#endif
    return NULL;
}

void* Blob::cpu() const{
    // 0.step do nothing
    if(m_size <= 0){
        return NULL;
    }

    // check whether in reset process
    m_lock->lock();
    if(this->m_waiting_reset_runtime){
        // wating reset runtime
        this->_sync();
        this->_reset();
    }

    // 1.step 直接返回
    if(m_runtime.type() == EAGLEEYE_CPU){
        m_lock->unlock();
        return (void*)(this->m_cpu_data.get());
    }

    // 2.step 等待来自gpu同步完毕
#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
    if(m_is_cpu_waiting_from_gpu){
        // 同步
        this->m_gpu_data.get()->finish(OpenCLRuntime::getOpenCLEnv()->getCommandQueue(m_group));
        // 设置
        m_is_cpu_ready = true;
        m_is_cpu_waiting_from_gpu = false;
    }
#endif

    // unlock
    m_lock->unlock();

    if(m_is_cpu_ready){
        return (void*)(this->m_cpu_data.get());
    }

    // 4.step 同步调用 
    this->transfer(EagleeyeRuntime(EAGLEEYE_CPU), false);
    return (void*)(this->m_cpu_data.get());
}

size_t Blob::blobsize() const{
    return this->m_size;
}

void Blob::schedule(EagleeyeRuntime runtime, bool asyn){
    // 0.step do nothing
    if(m_size <= 0){
        return;
    }

    // 1.step transfer to runtime
    this->transfer(runtime, asyn);

    // 2.step reset main runtime
    this->m_lock->lock();
    if(m_runtime.type() == runtime.type()){
        this->m_lock->unlock();
        return;
    }

    if(asyn){
        this->m_waiting_reset_runtime = true;
        this->m_waiting_runtime = runtime;
    }
    else{
        this->_reset();
    }
    this->m_lock->unlock();
}

EagleeyeType Blob::type() const{
    return this->m_data_type;
}

MemoryType Blob::memType() const{
    return this->m_memory_type;
}

size_t Blob::buffersize() const{
    if(this->m_memory_type == GPU_IMAGE){
        if(this->m_data_type == EAGLEEYE_FLOAT){
            return this->m_image_shape[0] * this->m_image_shape[1] * 4 * sizeof(float);
        }
        else if(this->m_data_type == EAGLEEYE_UCHAR){
            return this->m_image_shape[0] * this->m_image_shape[1] * 4 * sizeof(unsigned char);
        }
    }

    return this->blobsize();
}

bool Blob::empty() const{
    if(this->m_size == 0){
        return true;
    }
    if(this->m_runtime.type() == EAGLEEYE_CPU && this->m_cpu_data.get() == NULL){
        return true;
    }
    else if(this->m_runtime.type() == EAGLEEYE_GPU && this->m_gpu_data.get() == NULL){
        return true;
    }

    return false;
}

bool Blob::update(void* data, MemoryType mem_type){
    // support mem_type = CPU_BUFFER
    // TODO, support other GPU_BUFFER, GPU_IMAGE

    // data - host pointer
    if(data != NULL){
        // 使用data更新目标设备数据
        if(this->m_memory_type == CPU_BUFFER){
            memcpy(m_cpu_data.get(), data, m_size);
        }
        else{
#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
            if(this->m_gpu_memory_type == GPU_BUFFER){
                // GPU BUFFER
                this->m_gpu_data->copyToDevice(OpenCLRuntime::getOpenCLEnv()->getCommandQueue(m_group), data, CL_TRUE);
            }
            else{
                // GPU IMAGE
                // 0.step compile kernel
                if(m_buffer_to_image_kernel == NULL){
                    // compile
                    if(m_buffer_to_image_transform == "CONV2D_FILTER"){
                        m_buffer_to_image_kernel = new OpenCLKernelGroup(std::vector<std::string>{"filter_buffer_to_image"}, "buffer_to_image");
                    }
                    else if(m_buffer_to_image_transform == "IN_OUT_CHANNEL"){
                        m_buffer_to_image_kernel = new OpenCLKernelGroup(std::vector<std::string>{"in_out_buffer_to_image"}, "buffer_to_image");
                    }
                    else if(m_buffer_to_image_transform == "DW_CONV2D_FILTER"){
                        m_buffer_to_image_kernel = new OpenCLKernelGroup(std::vector<std::string>{"dw_filter_buffer_to_image"}, "buffer_to_image");
                    }
                }

                // 1.step build temp gpu buffer
                OpenCLMem* temp_gpu_buffer = new OpenCLMem(EAGLEEYE_CL_MEM_READ_WRITE, "t", m_size);
                temp_gpu_buffer->copyToDevice(OpenCLRuntime::getOpenCLEnv()->getCommandQueue(m_group), data, CL_TRUE);
                
                const uint32_t kwg_size = static_cast<uint32_t>(OpenCLRuntime::getOpenCLEnv()->getKernelMaxWorkGroupSize());
                size_t local_size[2];
                local_size[0] = 16;
                local_size[1] = kwg_size/16;

                size_t work_dims = 2;
                size_t global_size[2];
                size_t gws[2] = {static_cast<size_t>(this->m_image_shape[0]),static_cast<size_t>(this->m_image_shape[1])};
                global_size[0] = RoundUp(gws[0], local_size[0]);
                global_size[1] = RoundUp(gws[1], local_size[1]);

                // 2.step gpu buffer to gpu image
                if(m_buffer_to_image_transform == "CONV2D_FILTER"){
                    m_buffer_to_image_kernel->setKernelArg("filter_buffer_to_image", 0, (int)(gws[0]));
                    m_buffer_to_image_kernel->setKernelArg("filter_buffer_to_image", 1, (int)(gws[1]));
                    m_buffer_to_image_kernel->setKernelArg("filter_buffer_to_image", 2, temp_gpu_buffer->getObject());
                    m_buffer_to_image_kernel->setKernelArg("filter_buffer_to_image", 3, 0);
                    m_buffer_to_image_kernel->setKernelArg("filter_buffer_to_image", 4, (int)(this->m_shape[0]));
                    m_buffer_to_image_kernel->setKernelArg("filter_buffer_to_image", 5, (int)(this->m_shape[2]));
                    m_buffer_to_image_kernel->setKernelArg("filter_buffer_to_image", 6, (int)(this->m_shape[3]));
                    m_buffer_to_image_kernel->setKernelArg("filter_buffer_to_image", 7, (int)(this->m_shape[1] * this->m_shape[2] * this->m_shape[3]));
                    m_buffer_to_image_kernel->setKernelArg("filter_buffer_to_image", 8, this->m_gpu_data->getObject());
                    m_buffer_to_image_kernel->run("filter_buffer_to_image", work_dims, global_size, local_size);
                }
                else if(m_buffer_to_image_transform == "IN_OUT_CHANNEL"){
                    m_buffer_to_image_kernel->setKernelArg("in_out_buffer_to_image", 0, (int)(gws[0]));
                    m_buffer_to_image_kernel->setKernelArg("in_out_buffer_to_image", 1, (int)(gws[1]));
                    m_buffer_to_image_kernel->setKernelArg("in_out_buffer_to_image", 2, temp_gpu_buffer->getObject());
                    m_buffer_to_image_kernel->setKernelArg("in_out_buffer_to_image", 3, 0);
                    m_buffer_to_image_kernel->setKernelArg("in_out_buffer_to_image", 4, (int)(this->m_shape[1]));
                    m_buffer_to_image_kernel->setKernelArg("in_out_buffer_to_image", 5, (int)(this->m_shape[2]));
                    m_buffer_to_image_kernel->setKernelArg("in_out_buffer_to_image", 6, (int)(this->m_shape[3]));
                    m_buffer_to_image_kernel->setKernelArg("in_out_buffer_to_image", 7, this->m_gpu_data->getObject());
                    m_buffer_to_image_kernel->run("in_out_buffer_to_image", work_dims, global_size, local_size);
                }
                else if(m_buffer_to_image_transform == "DW_CONV2D_FILTER"){
                    m_buffer_to_image_kernel->setKernelArg("dw_filter_buffer_to_image", 0, (int)(gws[0]));
                    m_buffer_to_image_kernel->setKernelArg("dw_filter_buffer_to_image", 1, (int)(gws[1]));
                    m_buffer_to_image_kernel->setKernelArg("dw_filter_buffer_to_image", 2, temp_gpu_buffer->getObject());
                    m_buffer_to_image_kernel->setKernelArg("dw_filter_buffer_to_image", 3, 0);
                    m_buffer_to_image_kernel->setKernelArg("dw_filter_buffer_to_image", 4, (int)(this->m_shape[0]));
                    m_buffer_to_image_kernel->setKernelArg("dw_filter_buffer_to_image", 5, (int)(this->m_shape[1]));
                    m_buffer_to_image_kernel->setKernelArg("dw_filter_buffer_to_image", 6, (int)(this->m_shape[2]));
                    m_buffer_to_image_kernel->setKernelArg("dw_filter_buffer_to_image", 7, (int)(this->m_shape[3]));
                    m_buffer_to_image_kernel->setKernelArg("dw_filter_buffer_to_image", 8, this->m_gpu_data->getObject());
                    m_buffer_to_image_kernel->run("dw_filter_buffer_to_image", work_dims, global_size, local_size);
                }
                else{
                    // do nothing
                }
                delete temp_gpu_buffer;
            }
#endif
        }
    }
    else{
        // 更新目标设备数据
    }
    return true;
}

std::vector<int64_t> Blob::getImageShape() const{
    return this->m_image_shape;
}

void Blob::resizeImage(std::vector<int64_t> image_shape){
    assert(this->m_memory_type == GPU_IMAGE);
#ifdef EAGLEEYE_OPENCL_OPTIMIZATION    
    this->m_gpu_data = std::shared_ptr<OpenCLObject>(
            new OpenCLImage(EAGLEEYE_CL_MEM_READ_WRITE, "t", image_shape[0], image_shape[1], 4, this->m_data_type),
            [](OpenCLObject* arr) { delete arr; });

    this->m_image_shape = image_shape;
#endif
}
}