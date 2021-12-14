#include "eagleeye/basic/blob.h"
#include "eagleeye/common/EagleeyeOpenCL.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/basic/Math.h"
namespace eagleeye{
Range Range::ALL(){
    return Range(-1, -1);
}

Blob::Blob(){
    this->m_size = 0;
}

Blob::Blob(size_t size, 
            Aligned aligned, 
            EagleeyeRuntime runtime,
            void* data,
            bool copy)
    :m_size(size),
    m_is_cpu_ready(false),
    m_is_cpu_waiting_from_gpu(false),
    m_is_gpu_ready(false),
    m_is_gpu_waiting_from_cpu(false),
    m_aligned(aligned),
    m_waiting_reset_runtime(false),
    m_waiting_runtime(EAGLEEYE_UNKNOWN_RUNTIME){
    m_runtime = runtime;

    if(m_size <= 0){
        return;
    }

    if(runtime.type() != EAGLEEYE_CPU && runtime.type() != EAGLEEYE_GPU){
        EAGLEEYE_LOGE("only support CPU_BUFFER and GPU_BUFFER");
        return;
    }

    m_data_type = EAGLEEYE_UNDEFINED;
    // spin lock
    m_lock = std::shared_ptr<spinlock>(new spinlock(), [](spinlock* d) { delete d; });

    // apply device memory
    if(runtime.type() == EAGLEEYE_CPU){
        this->m_memory_type = CPU_BUFFER;
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
        if(data == NULL){
            // use gpu buffer
            this->m_gpu_data = 
                    std::shared_ptr<OpenCLObject>(new OpenCLMem(EAGLEEYE_CL_MEM_READ_WRITE, "t", size), 
                                                [](OpenCLObject* arr) { delete arr; });      
        }
        else{  
            // ignore copy param
            this->m_gpu_data = 
                    std::shared_ptr<OpenCLMem>(new OpenCLMem(EAGLEEYE_CL_MEM_READ_WRITE_PINNED, "t", size, data), 
                                                [](OpenCLMem* arr) { delete arr; });
        }
#endif
    }
}

Blob::Blob(unsigned int texture_id)
    :m_is_cpu_ready(false),
    m_is_cpu_waiting_from_gpu(false),
    m_is_gpu_ready(false),
    m_is_gpu_waiting_from_cpu(false),
    m_waiting_reset_runtime(false),
    m_waiting_runtime(EAGLEEYE_UNKNOWN_RUNTIME){
#ifdef EAGLEEYE_OPENCL_OPTIMIZATION    
    OpenCLImage* cl_img =  new OpenCLImage("t",texture_id);
    if(cl_img->m_rows == 0 || cl_img->m_cols == 0){
        return;
    }

    // only support channel_order = RGBA
    if(cl_img->m_image_format.image_channel_order == CL_RGBA && cl_img->m_image_format.image_channel_data_type == CL_UNSIGNED_INT8){
        m_size = 4*cl_img->m_rows*cl_img->m_cols;
        m_data_type = EAGLEEYE_RGBA;
    }
    else if(cl_img->m_image_format.image_channel_order == CL_RGBA && cl_img->m_image_format.image_channel_data_type == CL_FLOAT){
        m_size = sizeof(float)*4*cl_img->m_rows*cl_img->m_cols;
        m_data_type = EAGLEEYE_FLOAT4;
    }
    else{
        m_data_type = EAGLEEYE_UNDEFINED;
        m_size = 0;
        m_image_shape = std::vector<int64_t>{0,0};
        m_shape = std::vector<int64_t>{0,0};
        return;
    }

    this->m_gpu_data = 
        std::shared_ptr<OpenCLObject>(cl_img,[](OpenCLObject* arr) { delete arr; });

    m_lock = std::shared_ptr<spinlock>(new spinlock(), [](spinlock* d) { delete d; });
    this->m_memory_type = GPU_IMAGE;
    this->m_runtime = EagleeyeRuntime(EAGLEEYE_GPU);

    m_image_shape = std::vector<int64_t>{cl_img->m_cols, cl_img->m_rows};
    m_shape = std::vector<int64_t>{cl_img->m_rows, cl_img->m_cols};
#endif
}

Blob::Blob(std::vector<int64_t> shape, 
            EagleeyeType data_type, 
            MemoryType memory_type, 
            std::vector<int64_t> image_shape,
            Aligned aligned, void* data){
    this->m_size = 0;
    this->m_shape = shape;
    this->m_dims.ConstructFrom(shape);
    switch (data_type){
    case EAGLEEYE_CHAR:
    case EAGLEEYE_UCHAR:
        m_size = 
            sizeof(unsigned char)*std::accumulate(shape.begin(), shape.end(), 1, [](int64_t a, int64_t b){return a*b;});
        break;
    case EAGLEEYE_FLOAT:
        m_size = 
            sizeof(float)*std::accumulate(shape.begin(), shape.end(), 1, [](int64_t a, int64_t b){return a*b;});
        break;
    case EAGLEEYE_INT:
        m_size = 
            sizeof(int32_t)*std::accumulate(shape.begin(), shape.end(), 1, [](int64_t a, int64_t b){return a*b;});
        break;
    default:
        EAGLEEYE_LOGE("only support EAGLEEYE_CHAR, EAGLEEYE_UCHAR and EAGLEEYE_FLOAT");
        return;
    }

    if(m_size <= 0){
        return;
    }
    
    m_is_cpu_ready = false;
    m_is_cpu_waiting_from_gpu = false;
    m_is_gpu_ready = false;
    m_is_gpu_waiting_from_cpu = false;
    m_aligned = aligned;
    m_waiting_reset_runtime = false;
    m_waiting_runtime = EAGLEEYE_UNKNOWN_RUNTIME;
    m_data_type = data_type;
    m_memory_type = memory_type;
    m_image_shape = image_shape;                                // image shape (GPU_IMAGE)

    if(memory_type == CPU_BUFFER){
        m_runtime = EagleeyeRuntime(EAGLEEYE_CPU);
    }
    else{
        m_runtime = EagleeyeRuntime(EAGLEEYE_GPU);
    }

    // spin lock
    this->m_lock = std::shared_ptr<spinlock>(new spinlock(), [](spinlock* d) { delete d; });
    // apply device memory
    if(this->m_runtime.type() == EAGLEEYE_CPU){
        // allocate memory
        if(data == NULL){
            this->m_cpu_data = 
                    std::shared_ptr<unsigned char>(new unsigned char[m_size], 
                                                    [](unsigned char* arr) { delete [] arr; });
            memset(this->m_cpu_data.get(), 0, m_size);
        }
        else{
            this->m_cpu_data = 
                std::shared_ptr<unsigned char>((unsigned char*)data, 
                                                [](unsigned char* arr){});
        }
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
            if(this->m_image_shape.size() == 0){
                EAGLEEYE_LOGE("Must set image shape.");
                return;
            }
            unsigned int size = std::accumulate(shape.begin(), shape.end(), 1, [](int64_t a, int64_t b){return a*b;});
            unsigned int image_h = this->m_image_shape[1];
            unsigned int image_w = this->m_image_shape[0];
            unsigned int channels = size / (image_h * image_w);
            assert(channels * image_h * image_w == size);

            this->m_gpu_data = 
                std::shared_ptr<OpenCLObject>(
                    new OpenCLImage(EAGLEEYE_CL_MEM_READ_WRITE,
                                    "t", 
                                    image_h, 
                                    image_w, 
                                    channels, 
                                    this->m_data_type),[](OpenCLObject* arr) { delete arr; });
        }
#endif
    }
}

Blob::~Blob(){
}

void Blob::_reset() const{
    if(this->m_waiting_reset_runtime){
        this->m_runtime = this->m_waiting_runtime;
        this->m_waiting_runtime = EagleeyeRuntime(EAGLEEYE_UNKNOWN_RUNTIME);
        if(this->m_runtime.type() == EAGLEEYE_CPU){
            this->m_memory_type = CPU_BUFFER;
        }
        else{
            this->m_memory_type = this->m_gpu_data->type() == 0 ? GPU_BUFFER : GPU_IMAGE;
        }
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
        this->m_gpu_data.get()->finish();
        // 设置
        m_is_cpu_ready = true;
        m_is_cpu_waiting_from_gpu = false;
    #endif
    }
    else if(this->m_is_gpu_waiting_from_cpu){
        // cpu -> gpu
    #ifdef EAGLEEYE_OPENCL_OPTIMIZATION  
        // 同步
        this->m_gpu_data.get()->finish();
        // 设置
        m_is_gpu_waiting_from_cpu = false;
        m_is_gpu_ready = true;
    #endif
    }
    else{
        // do nothing
    }
}

void Blob::transfer(EagleeyeRuntime runtime, bool asyn, std::vector<int64_t> image_shape) const{
    // cpu_buffer -> gpu_buffer
    // cpu_buffer -> gpu_image (需要指定image大小)
    // gpu_buffer -> cpu_buffer
    // gpu_image -> cpu_buffer
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
                this->m_gpu_data.get()->copyToHost(this->m_cpu_data.get(), CL_FALSE);
                this->m_is_cpu_waiting_from_gpu = true;
                this->m_is_cpu_ready = false;
            }      
            else{
                this->m_gpu_data.get()->copyToHost(this->m_cpu_data.get(), CL_TRUE);
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
            if(image_shape.size() == 0){
                // gpu_buffer
                const size_t mem_size = this->buffersize();
                if(this->m_gpu_data.get() == NULL || this->m_gpu_data->type() != 0){
                    this->m_gpu_data = 
                            std::shared_ptr<OpenCLMem>(new OpenCLMem(EAGLEEYE_CL_MEM_READ_WRITE, "t", mem_size), 
                                                        [](OpenCLMem* arr) { delete arr; });
                }
            }
            else{
                // to gpu_image
                if(this->m_gpu_data.get() == NULL || this->m_gpu_data->type() != 0){
                    unsigned int size = std::accumulate(this->m_shape.begin(), this->m_shape.end(), 1, [](int64_t a, int64_t b){return a*b;});
                    unsigned int image_h = image_shape[1];
                    unsigned int image_w = image_shape[0];
                    unsigned int channels = size / (image_h * image_w);
                    assert(channels * image_h * image_w == size);
                    this->m_gpu_data = 
                            std::shared_ptr<OpenCLImage>(new OpenCLImage(EAGLEEYE_CL_MEM_READ_WRITE,
                                        "t", 
                                        image_shape[1], 
                                        image_shape[0], 
                                        channels, 
                                        this->m_data_type),[](OpenCLObject* arr) { delete arr; });
                }
            }

            if(asyn){
                this->m_gpu_data.get()->copyToDevice(this->m_cpu_data.get(), CL_FALSE);
                this->m_is_gpu_waiting_from_cpu = true;
                this->m_is_gpu_ready = false;
            }
            else{
                this->m_gpu_data.get()->copyToDevice(this->m_cpu_data.get(), CL_TRUE);
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

void* Blob::gpu(std::vector<int64_t> image_shape) const{
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
        this->m_gpu_data.get()->finish();
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
    this->transfer(EagleeyeRuntime(EAGLEEYE_GPU), false, image_shape);
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
        this->m_gpu_data.get()->finish();
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

void Blob::schedule(EagleeyeRuntime runtime, bool asyn, std::vector<int64_t> image_shape){
    // 0.step do nothing
    if(m_size <= 0){
        return;
    }

    // 1.step transfer to runtime
    this->transfer(runtime, asyn, image_shape);

    // 2.step reset main runtime
    this->m_lock->lock();
    if(m_runtime.type() == runtime.type()){
        this->m_lock->unlock();
        return;
    }

    if(runtime.type() == EAGLEEYE_GPU && image_shape.size() > 0){
        // 调度到GPU,并且GPU_IMAGE
        this->m_image_shape = image_shape;
    }

    // 3.step 设置等待状态
    this->m_waiting_reset_runtime = true;
    this->m_waiting_runtime = runtime;

    if(!asyn){
        // 阻塞调用时，直接重置
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
        unsigned int size = std::accumulate(this->m_shape.begin(), this->m_shape.end(), 1, [](int64_t a, int64_t b){return a*b;});
        unsigned int image_h = this->m_image_shape[1];
        unsigned int image_w = this->m_image_shape[0];
        unsigned int channels = size / (image_h * image_w);
        assert(channels * image_h * image_w == size);

        if(this->m_data_type == EAGLEEYE_FLOAT){
            return this->m_image_shape[0] * this->m_image_shape[1] * channels * sizeof(float);
        }
        else if(this->m_data_type == EAGLEEYE_UCHAR || this->m_data_type == EAGLEEYE_CHAR){
            return this->m_image_shape[0] * this->m_image_shape[1] * channels * sizeof(unsigned char);
        }
    }

    return this->blobsize();
}

bool Blob::empty() const{
    if(this->m_size <= 0){
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

bool Blob::update(void* data, MemoryType mem_type, std::string option){
    // support mem_type = CPU_BUFFER
    // TODO, support other GPU_BUFFER, GPU_IMAGE
    // data - host pointer
    if(data != NULL){
        this->m_lock->lock();
        // 检查是否处在调度状态
        if(this->m_waiting_reset_runtime){
            // wating reset runtime
            this->_sync();
            this->_reset();
        }
        else{
            // reset runtime
            this->_reset();
        }

        // 使用data更新目标设备数据
        if(this->m_memory_type == CPU_BUFFER){
            memcpy(m_cpu_data.get(), data, m_size);
        }
        else{
#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
            if(this->m_memory_type == GPU_BUFFER || (this->m_memory_type == GPU_IMAGE && option == "")){
                // GPU BUFFER
                this->m_gpu_data->copyToDevice(data, CL_TRUE);
            }
            else{
                // GPU IMAGE(需要根据kernel进行数据重新编排)
                // 0.step compile kernel
                OpenCLKernelGroup* okg = OpenCLKernelGroupManager::getInstance()->kgroup(option);
                if(okg == NULL){
                    // compile
                    if(option == "CONV2D_FILTER"){
                        okg = new OpenCLKernelGroup(std::vector<std::string>{"filter_buffer_to_image"}, "buffer_to_image");
                    }
                    else if(option == "IN_OUT_CHANNEL"){
                        okg = new OpenCLKernelGroup(std::vector<std::string>{"in_out_buffer_to_image"}, "buffer_to_image");
                    }
                    else if(option == "DW_CONV2D_FILTER"){
                        okg = new OpenCLKernelGroup(std::vector<std::string>{"dw_filter_buffer_to_image"}, "buffer_to_image");
                    }

                    OpenCLKernelGroupManager::getInstance()->add(option, okg);
                }

                // 1.step build temp gpu buffer
                OpenCLMem* temp_gpu_buffer = new OpenCLMem(EAGLEEYE_CL_MEM_READ_WRITE_PINNED, "t", m_size, data);                
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
                if(option == "CONV2D_FILTER"){
                    okg->setKernelArg("filter_buffer_to_image", 0, (int)(gws[0]));
                    okg->setKernelArg("filter_buffer_to_image", 1, (int)(gws[1]));
                    okg->setKernelArg("filter_buffer_to_image", 2, temp_gpu_buffer->getObject());
                    okg->setKernelArg("filter_buffer_to_image", 3, 0);
                    okg->setKernelArg("filter_buffer_to_image", 4, (int)(this->m_shape[0]));
                    okg->setKernelArg("filter_buffer_to_image", 5, (int)(this->m_shape[2]));
                    okg->setKernelArg("filter_buffer_to_image", 6, (int)(this->m_shape[3]));
                    okg->setKernelArg("filter_buffer_to_image", 7, (int)(this->m_shape[1] * this->m_shape[2] * this->m_shape[3]));
                    okg->setKernelArg("filter_buffer_to_image", 8, this->m_gpu_data->getObject());
                    okg->run("filter_buffer_to_image", work_dims, global_size, local_size);
                }
                else if(option == "IN_OUT_CHANNEL"){
                    okg->setKernelArg("in_out_buffer_to_image", 0, (int)(gws[0]));
                    okg->setKernelArg("in_out_buffer_to_image", 1, (int)(gws[1]));
                    okg->setKernelArg("in_out_buffer_to_image", 2, temp_gpu_buffer->getObject());
                    okg->setKernelArg("in_out_buffer_to_image", 3, 0);
                    okg->setKernelArg("in_out_buffer_to_image", 4, (int)(this->m_shape[1]));
                    okg->setKernelArg("in_out_buffer_to_image", 5, (int)(this->m_shape[2]));
                    okg->setKernelArg("in_out_buffer_to_image", 6, (int)(this->m_shape[3]));
                    okg->setKernelArg("in_out_buffer_to_image", 7, this->m_gpu_data->getObject());
                    okg->run("in_out_buffer_to_image", work_dims, global_size, local_size);
                }
                else if(option == "DW_CONV2D_FILTER"){
                    okg->setKernelArg("dw_filter_buffer_to_image", 0, (int)(gws[0]));
                    okg->setKernelArg("dw_filter_buffer_to_image", 1, (int)(gws[1]));
                    okg->setKernelArg("dw_filter_buffer_to_image", 2, temp_gpu_buffer->getObject());
                    okg->setKernelArg("dw_filter_buffer_to_image", 3, 0);
                    okg->setKernelArg("dw_filter_buffer_to_image", 4, (int)(this->m_shape[0]));
                    okg->setKernelArg("dw_filter_buffer_to_image", 5, (int)(this->m_shape[1]));
                    okg->setKernelArg("dw_filter_buffer_to_image", 6, (int)(this->m_shape[2]));
                    okg->setKernelArg("dw_filter_buffer_to_image", 7, (int)(this->m_shape[3]));
                    okg->setKernelArg("dw_filter_buffer_to_image", 8, this->m_gpu_data->getObject());
                    okg->run("dw_filter_buffer_to_image", work_dims, global_size, local_size);
                }
                delete temp_gpu_buffer;
            }
#endif
        }

        this->m_lock->unlock();
    }
    return true;
}

std::vector<int64_t> Blob::getImageShape() const{
    return this->m_image_shape;
}

std::vector<int64_t> Blob::shape() const{
    return std::move(this->m_shape);
}

void Blob::reshape(std::vector<int64_t> s){
    int old_num = std::accumulate(m_shape.begin(), m_shape.end(), 1, [](int64_t a, int64_t b){return a*b;});
    int new_num = std::accumulate(s.begin(), s.end(), 1, [](int64_t a, int64_t b){return a*b;});
    if(old_num != new_num){
        EAGLEEYE_LOGE("Shape not consistent.");
        return;
    }

    this->m_shape = s;
    this->m_dims.ConstructFrom(this->m_shape);
}
}