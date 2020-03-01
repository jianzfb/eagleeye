#include "eagleeye/basic/blob.h"
namespace eagleeye{
Blob::Blob(size_t size, EagleeyeRuntime runtime,void* data, bool copy, std::string group)
    :m_size(size),
    m_is_cpu_ready(false),
    m_is_cpu_waiting_from_gpu(false),
    m_is_cpu_waiting_from_dsp(false),
    m_is_gpu_ready(false),
    m_is_gpu_waiting_from_cpu(false),
    m_is_gpu_waiting_from_dsp(false),
    m_is_dsp_ready(false),
    m_is_dsp_waiting_from_cpu(false),
    m_is_dsp_waiting_from_gpu(false),
    m_group(group){
    // spin lock
    m_lock = std::shared_ptr<spinlock>(new spinlock(), [](spinlock* d) { delete d; });
    m_runtime = runtime;
    if(m_size == 0){
        // dont apply memory
        return;
    }

    if(runtime.type() == EAGLEEYE_CPU){
        if(data == NULL){
            this->m_cpu_data = std::shared_ptr<unsigned char>(new unsigned char[m_size], [](unsigned char* arr) { delete [] arr; });
        }
        else if(copy){
            this->m_cpu_data = std::shared_ptr<unsigned char>(new unsigned char[m_size], [](unsigned char* arr) { delete [] arr; });
            memcpy(this->m_cpu_data.get(), data, size);		
        }
        else{
            this->m_cpu_data = std::shared_ptr<unsigned char>((unsigned char*)data, [](unsigned char* arr){});
        }
    }
    else if(runtime.type() == EAGLEEYE_GPU){
#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
        this->m_gpu_data = std::shared_ptr<OpenCLMem>(new OpenCLMem(EAGLEEYE_CL_MEM_READ_WRITE, "t", size), [](OpenCLMem* arr) { delete arr; });        
        if(data != NULL){
            this->m_gpu_data.get()->copyToDevice(OpenCLEnv::getOpenCLEnv()->getCommandQueue(m_group), data, CL_TRUE);
        }
#endif
    }
    else if(runtime.type() == EAGLEEYE_QUALCOMM_DSP){
    }
}

Blob::~Blob(){
}

void Blob::update(){
    this->m_is_cpu_waiting_from_gpu = false;
    this->m_is_cpu_waiting_from_dsp = false;
    this->m_is_cpu_ready = false;
    this->m_is_gpu_waiting_from_cpu = false;
    this->m_is_gpu_waiting_from_dsp = false;
    this->m_is_gpu_ready = false;
    this->m_is_dsp_waiting_from_cpu = false;
    this->m_is_dsp_waiting_from_gpu = false;
    this->m_is_dsp_ready = false;
}

void Blob::transfer(EagleeyeRuntime runtime, bool asyn){
    if(runtime.type() == m_runtime.type()){
        return;
    }

    // lock
    m_lock->lock();

    switch (runtime.type())
    {
    case EAGLEEYE_CPU:
        // has done
        if(m_is_cpu_waiting_from_dsp || m_is_cpu_waiting_from_gpu || m_is_cpu_ready){
            break;
        }
        // transfer from Source to CPU
        switch (m_runtime.type())
        {
        case EAGLEEYE_GPU:
    #ifdef EAGLEEYE_OPENCL_OPTIMIZATION        
            if(this->m_cpu_data.get() == NULL){
                this->m_cpu_data = std::shared_ptr<unsigned char>(new unsigned char[m_size], [](unsigned char* arr) { delete [] arr; });
            }      
            if(asyn){
                this->m_gpu_data.get()->copyToHost(OpenCLEnv::getOpenCLEnv()->getCommandQueue(m_group), this->m_cpu_data.get(), CL_FALSE);
                this->m_is_cpu_waiting_from_gpu = true;
                this->m_is_cpu_ready = false;
            }      
            else{
                this->m_gpu_data.get()->copyToHost(OpenCLEnv::getOpenCLEnv()->getCommandQueue(m_group), this->m_cpu_data.get(), CL_TRUE);
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
        if(m_is_gpu_waiting_from_dsp || m_is_gpu_waiting_from_cpu || m_is_gpu_ready){
            break;
        }

        // transfer from Source to CPU
    #ifdef EAGLEEYE_OPENCL_OPTIMIZATION    
        switch (m_runtime.type())
        {
        case EAGLEEYE_CPU:
            if(this->m_gpu_data.get() == NULL){
                this->m_gpu_data = std::shared_ptr<OpenCLMem>(new OpenCLMem(EAGLEEYE_CL_MEM_READ_WRITE, "t", m_size), [](OpenCLMem* arr) { delete arr; });
            }
            if(asyn){
                this->m_gpu_data.get()->copyToDevice(OpenCLEnv::getOpenCLEnv()->getCommandQueue(m_group), this->m_cpu_data.get(), CL_FALSE);
                this->m_is_gpu_waiting_from_cpu = true;
                this->m_is_gpu_ready = false;
            }
            else{
                this->m_gpu_data.get()->copyToDevice(OpenCLEnv::getOpenCLEnv()->getCommandQueue(m_group), this->m_cpu_data.get(), CL_TRUE);
                this->m_is_gpu_waiting_from_cpu = false;
                this->m_is_gpu_ready = true;
            }
            break;        
        default:
            break;
        }
    #endif
        break;
    case EAGLEEYE_QUALCOMM_DSP:
        break;
    default:
        break;
    }
    // unlock
    m_lock->unlock();
}

void* Blob::gpu(){
    // 0.step do nothing
    if(m_size == 0){
        return NULL;
    }

#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
    // 1.step 直接返回
    if(m_runtime.type() == EAGLEEYE_GPU){
        return (void*)(this->m_gpu_data.get());
    }

    // 2.step 等待来自cpu同步完毕
    // lock
    m_lock->lock();

    if(m_is_gpu_waiting_from_cpu){
        // 同步
        this->m_gpu_data.get()->finish(OpenCLEnv::getOpenCLEnv()->getCommandQueue(m_group));
        // 设置
        m_is_gpu_waiting_from_cpu = false;
        m_is_gpu_ready = true;
    }

    // 3.step 等待来自dsp同步完毕
    if(m_is_gpu_waiting_from_dsp){
        m_is_gpu_waiting_from_dsp = false;
        m_is_gpu_ready = true;
    }

    // unlock
    m_lock->unlock();

    if(m_is_gpu_ready){
        return (void*)(*(this->m_gpu_data.get()->getObject()));
    }

    // 4.step 同步调用
    this->transfer(EagleeyeRuntime(EAGLEEYE_GPU),false);
    return (void*)(*(this->m_gpu_data.get()->getObject()));
#endif
    return NULL;
}
void* Blob::dsp(){
    return NULL;
}
void* Blob::cpu(){
    // 0.step do nothing
    if(m_size == 0){
        return NULL;
    }

    // 1.step 直接返回
    if(m_runtime.type() == EAGLEEYE_CPU){
        return (void*)(this->m_cpu_data.get());
    }

    // 2.step 等待来自gpu同步完毕
    // lock
    m_lock->lock();

#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
    if(m_is_cpu_waiting_from_gpu){
        // 同步
        this->m_gpu_data.get()->finish(OpenCLEnv::getOpenCLEnv()->getCommandQueue(m_group));
        // 设置
        m_is_cpu_ready = true;
        m_is_cpu_waiting_from_gpu = false;
    }
#endif
    // 3.step 等待来自dsp同步完毕
    if(m_is_cpu_waiting_from_dsp){
        m_is_cpu_waiting_from_dsp = false;
        m_is_cpu_ready = true;
    }

    // unlock
    m_lock->unlock();

    if(m_is_cpu_ready){
        return (void*)(this->m_cpu_data.get());
    }

    // 4.step 同步调用 
    this->transfer(EagleeyeRuntime(EAGLEEYE_CPU),false);
    return (void*)(this->m_cpu_data.get());
}

size_t Blob::blobsize(){
    return this->m_size;
}

void Blob::schedule(EagleeyeRuntime runtime){
    this->m_lock->lock();
    this->m_runtime = runtime;
    this->m_lock->unlock();
}
}