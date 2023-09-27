#include "eagleeye/basic/blob.h"
#include "eagleeye/common/EagleeyeOpenCL.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/basic/Math.h"
#include "eagleeye/basic/type.h"
namespace eagleeye{
Range Range::ALL(){
    return Range(-1, -1);
}

Blob::Blob(){
    this->m_size = 0;
    this->m_offset = 0 ;
    this->m_elem_size = 0;
}

Blob::Blob(int64_t size, EagleeyeType data_type, MemoryType memory_type, Aligned aligned, 
            void* data,
            bool copy)
        :m_offset(0),
        m_size(0),
        m_elem_size(0),
        m_is_cpu_ready(false),
        m_is_cpu_waiting_from_gpu(false),
        m_is_gpu_ready(false),
        m_is_gpu_waiting_from_cpu(false),
        m_aligned(aligned),
        m_waiting_reset_runtime(false),
        m_data_type(data_type),
        m_waiting_runtime(EAGLEEYE_UNKNOWN_RUNTIME),
        m_memory_type(memory_type){
    if(size == 0){
        return;
    }

    this->m_dims.ConstructFrom(std::vector<int64_t>{size});
    this->m_elem_size = TypeInfo::getElemSize(this->m_data_type);
    m_size = m_elem_size*this->m_dims.production();
    if(m_size == 0){
        return;
    }
    if(m_memory_type == GPU_IMAGE){
        EAGLEEYE_LOGE("Dont support GPU_IMAGE.");
        return;
    }

    if(m_memory_type == CPU_BUFFER){
        m_runtime = EagleeyeRuntime(EAGLEEYE_CPU);
    }
    else{
        m_runtime = EagleeyeRuntime(EAGLEEYE_GPU);
    }

    // spin lock
    m_lock = std::shared_ptr<spinlock>(new spinlock(), [](spinlock* d) { delete d; });

    // apply device memory
    if(m_runtime.type() == EAGLEEYE_CPU){
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
            memcpy(this->m_cpu_data.get(), data, m_size);		
        }
        else{
            // share cpu memory
            this->m_cpu_data = 
                        std::shared_ptr<unsigned char>((unsigned char*)data, 
                        [](unsigned char* arr){});
        }
    }
    else if(m_runtime.type() == EAGLEEYE_GPU){
#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
        if(data == NULL){
            // use gpu buffer
            this->m_gpu_data = 
                    std::shared_ptr<OpenCLObject>(new OpenCLMem(EAGLEEYE_CL_MEM_READ_WRITE, "t", m_size), 
                                                [](OpenCLObject* arr) { delete arr; });      
        }
        else{  
            // ignore copy param
            this->m_gpu_data = 
                    std::shared_ptr<OpenCLMem>(new OpenCLMem(EAGLEEYE_CL_MEM_READ_WRITE_PINNED, "t", m_size, data), 
                                                [](OpenCLMem* arr) { delete arr; });
        }
#endif
    }
}

Blob::Blob(int64_t h, int64_t w, EagleeyeType data_type, MemoryType memory_type, Aligned aligned, 
            void* data,
            bool copy)
    :m_offset(0),
    m_size(0),
    m_is_cpu_ready(false),
    m_is_cpu_waiting_from_gpu(false),
    m_is_gpu_ready(false),
    m_is_gpu_waiting_from_cpu(false),
    m_aligned(aligned),
    m_waiting_reset_runtime(false),
    m_data_type(data_type),
    m_waiting_runtime(EAGLEEYE_UNKNOWN_RUNTIME),
    m_memory_type(memory_type){
    if(h == 0 || w == 0){
        return;
    }

    // dims 
    this->m_dims.ConstructFrom(std::vector<int64_t>{h,w});
    this->m_elem_size = TypeInfo::getElemSize(this->m_data_type);
    m_size = m_elem_size*this->m_dims.production();
    if(m_size <= 0){
        return;
    }
    if(m_memory_type == GPU_IMAGE){
        EAGLEEYE_LOGE("Dont support GPU_IMAGE.");
        return;
    }
    if(m_memory_type == CPU_BUFFER){
        m_runtime = EagleeyeRuntime(EAGLEEYE_CPU);
    }
    else{
        m_runtime = EagleeyeRuntime(EAGLEEYE_GPU);
    }

    // spin lock
    m_lock = std::shared_ptr<spinlock>(new spinlock(), [](spinlock* d) { delete d; });

    // apply device memory
    if(m_runtime.type() == EAGLEEYE_CPU){
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
            memcpy(this->m_cpu_data.get(), data, m_size);		
        }
        else{
            // share cpu memory
            this->m_cpu_data = 
                        std::shared_ptr<unsigned char>((unsigned char*)data, 
                        [](unsigned char* arr){});
        }
    }
    else if(m_runtime.type() == EAGLEEYE_GPU){
#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
        if(data == NULL){
            // use gpu buffer
            this->m_gpu_data = 
                    std::shared_ptr<OpenCLObject>(new OpenCLMem(EAGLEEYE_CL_MEM_READ_WRITE, "t", m_size), 
                                                [](OpenCLObject* arr) { delete arr; });      
        }
        else{  
            // ignore copy param
            this->m_gpu_data = 
                    std::shared_ptr<OpenCLObject>(new OpenCLMem(EAGLEEYE_CL_MEM_READ_WRITE_PINNED, "t", m_size, data), 
                                                [](OpenCLObject* arr) { delete arr; });
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
    // TODO, 需要验证
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
        return;
    }

    this->m_gpu_data = 
        std::shared_ptr<OpenCLObject>(cl_img,[](OpenCLObject* arr) { delete arr; });

    m_lock = std::shared_ptr<spinlock>(new spinlock(), [](spinlock* d) { delete d; });
    this->m_memory_type = GPU_IMAGE;
    this->m_runtime = EagleeyeRuntime(EAGLEEYE_GPU);

    m_image_shape = std::vector<int64_t>{cl_img->m_cols, cl_img->m_rows};
    this->m_dims.ConstructFrom(std::vector<int64_t>{cl_img->m_rows, cl_img->m_cols});
#endif
}

Blob::Blob(const std::vector<int64_t> shape, 
            EagleeyeType data_type, 
            MemoryType memory_type, 
            std::vector<int64_t> image_shape,
            Aligned aligned, void* data)
        :m_size(0), m_offset(0), m_is_cpu_ready(false), m_is_cpu_waiting_from_gpu(false),
            m_is_gpu_ready(false), m_is_gpu_waiting_from_cpu(false), m_aligned(aligned),
            m_waiting_reset_runtime(false), m_waiting_runtime(EAGLEEYE_UNKNOWN_RUNTIME), 
            m_data_type(data_type), m_memory_type(memory_type), m_image_shape(image_shape){
    if(shape.size() == 0){
        return;
    }

    this->m_dims.ConstructFrom(shape);
    this->m_elem_size = TypeInfo::getElemSize(this->m_data_type);
    m_size = m_elem_size*this->m_dims.production();
    if(m_size <= 0){
        return;
    }
    
    if(m_memory_type == GPU_IMAGE && this->m_image_shape.size() == 0){
        return;
    }

    if(m_memory_type == CPU_BUFFER){
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
            int64_t size = this->m_dims.production();
            int64_t image_h = this->m_image_shape[1];
            int64_t image_w = this->m_image_shape[0];
            int64_t channels = size / (image_h * image_w);
            assert(channels * image_h * image_w == size);
            if(channels != 1 && channels != 3 && channels != 4){
                EAGLEEYE_LOGE("Channels must be 1,3 or 4.");
                return;
            }

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
#ifdef EAGLEEYE_OPENCL_OPTIMIZATION            
            this->m_memory_type = this->m_gpu_data->type() == 0 ? GPU_BUFFER : GPU_IMAGE;
#endif
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
    if(this->empty()){
        return;
    }
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
                this->m_cpu_data = 
                        std::shared_ptr<unsigned char>(new unsigned char[this->blobsize()], 
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
                if(this->m_gpu_data.get() == NULL || this->m_gpu_data->type() != 0){
                    this->m_gpu_data = 
                            std::shared_ptr<OpenCLMem>(new OpenCLMem(EAGLEEYE_CL_MEM_READ_WRITE, "t", this->blobsize()), 
                                                        [](OpenCLMem* arr) { delete arr; });
                }
            }
            else{
                // to gpu_image
                if(this->m_gpu_data.get() == NULL || this->m_gpu_data->type() != 0){
                    int64_t size = this->m_dims.production();
                    int64_t image_h = image_shape[1];
                    int64_t image_w = image_shape[0];
                    int64_t channels = size / (image_h * image_w);
                    assert(channels * image_h * image_w == size);
                    if(channels != 1 && channels != 3 && channels != 4){
                        EAGLEEYE_LOGE("Channels must be 1,3 or 4.");
                        return;
                    }

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

void* Blob::gpu(std::vector<int64_t> image_shape, bool is_offset) const{
    if(is_offset){
        EAGLEEYE_LOGE("Dont support offset gpu mem.");
        return NULL;
    }

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
        return this->m_gpu_data.get();
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
        return this->m_gpu_data.get();
    }

    // 4.step 同步调用
    this->transfer(EagleeyeRuntime(EAGLEEYE_GPU), false, image_shape);
    return this->m_gpu_data.get();
#endif
    return NULL;
}

void* Blob::cpu(bool is_offset) const{
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

    int64_t offset = is_offset?this->m_offset:0;
    // 1.step 直接返回
    if(m_runtime.type() == EAGLEEYE_CPU){
        m_lock->unlock();
        return (void*)(this->m_cpu_data.get() + offset);
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
        return (void*)(this->m_cpu_data.get() + offset);
    }

    // 4.step 同步调用 
    this->transfer(EagleeyeRuntime(EAGLEEYE_CPU), false);
    return (void*)(this->m_cpu_data.get() + offset);
}

size_t Blob::blobsize() const{
    return this->m_size;
}

size_t Blob::bloboffset() const{
    return this->m_offset;
}

EagleeyeType Blob::type() const{
    return this->m_data_type;
}

MemoryType Blob::memType() const{
    return this->m_memory_type;
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
    if(mem_type == GPU_IMAGE || mem_type == GPU_BUFFER){
        EAGLEEYE_LOGE("Dont support GPU_IMAGE/GPU_BUFFER in mem_type.");
        return false;
    }
    if(data == NULL){
        return false;
    }
    if(this->empty()){
        // 如果为空，直接返回
        return NULL;
    }

    // copy
    if(mem_type == CPU_BUFFER){
        if(this->m_memory_type == CPU_BUFFER){
            memcpy(this->cpu(true), data, this->numel()*this->m_elem_size);
        }
        else{
#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
        if(this->m_memory_type == GPU_BUFFER || (this->m_memory_type == GPU_IMAGE && option == "")){
            // GPU BUFFER
            OpenCLMem* ocl_mem = (OpenCLMem*)this->gpu();
            ocl_mem->copyToDevice(data, CL_TRUE, 0, this->m_offset, this->numel()*this->m_elem_size);
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
                okg->setKernelArg("filter_buffer_to_image", 4, (int)(this->m_dims[0]));
                okg->setKernelArg("filter_buffer_to_image", 5, (int)(this->m_dims[2]));
                okg->setKernelArg("filter_buffer_to_image", 6, (int)(this->m_dims[3]));
                okg->setKernelArg("filter_buffer_to_image", 7, (int)(this->m_dims[1] * this->m_dims[2] * this->m_dims[3]));
                okg->setKernelArg("filter_buffer_to_image", 8, this->m_gpu_data->getObject());
                okg->run("filter_buffer_to_image", work_dims, global_size, local_size);
            }
            else if(option == "IN_OUT_CHANNEL"){
                okg->setKernelArg("in_out_buffer_to_image", 0, (int)(gws[0]));
                okg->setKernelArg("in_out_buffer_to_image", 1, (int)(gws[1]));
                okg->setKernelArg("in_out_buffer_to_image", 2, temp_gpu_buffer->getObject());
                okg->setKernelArg("in_out_buffer_to_image", 3, 0);
                okg->setKernelArg("in_out_buffer_to_image", 4, (int)(this->m_dims[1]));
                okg->setKernelArg("in_out_buffer_to_image", 5, (int)(this->m_dims[2]));
                okg->setKernelArg("in_out_buffer_to_image", 6, (int)(this->m_dims[3]));
                okg->setKernelArg("in_out_buffer_to_image", 7, this->m_gpu_data->getObject());
                okg->run("in_out_buffer_to_image", work_dims, global_size, local_size);
            }
            else if(option == "DW_CONV2D_FILTER"){
                okg->setKernelArg("dw_filter_buffer_to_image", 0, (int)(gws[0]));
                okg->setKernelArg("dw_filter_buffer_to_image", 1, (int)(gws[1]));
                okg->setKernelArg("dw_filter_buffer_to_image", 2, temp_gpu_buffer->getObject());
                okg->setKernelArg("dw_filter_buffer_to_image", 3, 0);
                okg->setKernelArg("dw_filter_buffer_to_image", 4, (int)(this->m_dims[0]));
                okg->setKernelArg("dw_filter_buffer_to_image", 5, (int)(this->m_dims[1]));
                okg->setKernelArg("dw_filter_buffer_to_image", 6, (int)(this->m_dims[2]));
                okg->setKernelArg("dw_filter_buffer_to_image", 7, (int)(this->m_dims[3]));
                okg->setKernelArg("dw_filter_buffer_to_image", 8, this->m_gpu_data->getObject());
                okg->run("dw_filter_buffer_to_image", work_dims, global_size, local_size);
            }
            delete temp_gpu_buffer;
        }
#endif
        }
    }

    return true;
}

bool Blob::update(){
    if(this->empty()){
        // 如果为空，直接返回
        return true;
    }

    if(this->m_memory_type == GPU_IMAGE){
        EAGLEEYE_LOGE("Dont support GPU_IMAGE.");
        return false;
    }

    if(this->m_memory_type == CPU_BUFFER){
        // GPU(发生更新)->CPU(主存储)
        void* gpu_ptr = this->gpu();
        if(gpu_ptr == NULL){
            return false;
        }
#ifdef EAGLEEYE_OPENCL_OPTIMIZATION  
        OpenCLMem* ocl_mem = (OpenCLMem*)gpu_ptr;
        ocl_mem->copyToHost(this->cpu(true), true, this->m_offset, 0, this->numel()*this->m_elem_size);
#endif
    }
    else{
        // CPU(发生更新)->GPU(主存储)
        void* cpu_ptr = this->cpu(true);                    // cpu 是偏移后的指针
        if(cpu_ptr == NULL){
            return false;
        }
#ifdef EAGLEEYE_OPENCL_OPTIMIZATION        
        OpenCLMem* ocl_mem = (OpenCLMem*)this->gpu();   // gpu 未偏移
        ocl_mem->copyToDevice(cpu_ptr, true, 0, this->m_offset, this->numel()*this->m_elem_size);
#endif
    }
    return true;
}

std::vector<int64_t> Blob::getImageShape() const{
    return this->m_image_shape;
}

void Blob::reshape(std::vector<int64_t> shape){
    int num = std::accumulate(shape.begin(), shape.end(), 1, [](int64_t a, int64_t b){return a*b;});
    if(this->m_dims.production() != num){
        EAGLEEYE_LOGE("Shape not consistent.");
        return;
    }

    this->m_dims.ConstructFrom(shape);
}

void Blob::reshape(Dim dim){
    if(this->m_dims.production() != dim.production()){
        EAGLEEYE_LOGE("Shape not consistent.");
        return;
    }

    this->m_dims = dim;
}
}