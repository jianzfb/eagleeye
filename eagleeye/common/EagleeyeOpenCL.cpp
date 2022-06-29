#include "eagleeye/common/EagleeyeOpenCL.h"
#include "eagleeye/common/EagleeyeLog.h"
#include <iostream>

#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
#include <CL/opencl.h>
#include <GLES3/gl3.h>

namespace eagleeye{
template<>
void OpenCLKernelGroup::setKernelArg<std::string>(std::string kernel_name, int index, std::string name){
    int err = clSetKernelArg(m_kernels[kernel_name], index, sizeof(cl_mem), m_mems[name]->getObject());
    if(err != CL_SUCCESS){
        EAGLEEYE_LOGE("Failed to set arg %d for kernel %s with error %s", index, kernel_name.c_str(), OpenCLErrorToString(err));
    }
}

//******************        OpenCLMem        ******************//
OpenCLMem::OpenCLMem(OpenCLMemStatus mem_status, 
                    std::string name, 
                    unsigned int size, 
                    void* host_ptr){
    int err;
    switch(mem_status){
        case EAGLEEYE_CL_MEM_READ:
            m_mem = 
                clCreateBuffer(OpenCLRuntime::getOpenCLEnv()->context,  
                                CL_MEM_READ_ONLY, 
                                size, 
                                NULL, 
                                &err);
            break;
        case EAGLEEYE_CL_MEM_WRITE:
            m_mem = 
                clCreateBuffer(OpenCLRuntime::getOpenCLEnv()->context,  
                                CL_MEM_WRITE_ONLY, 
                                size, 
                                NULL, 
                                &err);
            break;
        case EAGLEEYE_CL_MEM_READ_WRITE_PINNED:
            m_mem = 
                clCreateBuffer(OpenCLRuntime::getOpenCLEnv()->context, 
                                CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 
                                size, 
                                host_ptr, 
                                &err);
            break;
        default:
            m_mem = 
                clCreateBuffer(OpenCLRuntime::getOpenCLEnv()->context,  
                                CL_MEM_READ_WRITE, 
                                size, 
                                NULL, 
                                &err);
            break;
    }
    if(err != CL_SUCCESS){
        EAGLEEYE_LOGE("fail to create buffer with %s", OpenCLErrorToString(err));
    }

    m_name = name;
    m_size = size;
    m_mem_status = mem_status;
    m_mapped_ptr = NULL;
}

OpenCLMem::~OpenCLMem(){
    clReleaseMemObject(m_mem);
}

void OpenCLMem::copyToDevice(void* host_ptr, cl_bool blocking, int64_t src_offset, int64_t dst_offset, int64_t cp_size, int64_t src_offset_y, int64_t dst_offset_y, int64_t cp_size_y){
    // ignore: int64_t src_offset_y, int64_t dst_offset_y, int64_t cp_size_y
    int err = 
        clEnqueueWriteBuffer(OpenCLRuntime::getOpenCLEnv()->getOrCreateCommandQueue(false), 
                            m_mem, blocking, dst_offset, cp_size==0?m_size:cp_size, host_ptr, 0, NULL, &this->m_event);

    if(err != CL_SUCCESS){
        EAGLEEYE_LOGE("fail to write device buffer with error %s", OpenCLErrorToString(err));
    }
}

void OpenCLMem::copyToHost(void* host_ptr, cl_bool blocking, int64_t src_offset, int64_t dst_offset, int64_t cp_size, int64_t src_offset_y, int64_t dst_offset_y, int64_t cp_size_y){
    // ignore: int64_t src_offset_y, int64_t dst_offset_y, int64_t cp_size_y
    int err = 
        clEnqueueReadBuffer(OpenCLRuntime::getOpenCLEnv()->getOrCreateCommandQueue(false), 
                            m_mem, blocking, src_offset, cp_size==0?m_size:cp_size, host_ptr, 0, NULL, &this->m_event ); 
 
    if(err != CL_SUCCESS){
        EAGLEEYE_LOGE("fail to read device buffer with error %s", OpenCLErrorToString(err));
    }
}

void OpenCLMem::copyToDeviceFromDevice(void* ptr, cl_bool blocking, int64_t src_offset, int64_t dst_offset, int64_t cp_size, int64_t src_offset_y, int64_t dst_offset_y, int64_t cp_size_y){
    // ignore: int64_t src_offset_y, int64_t dst_offset_y, int64_t cp_size_y
    int err = 
        clEnqueueCopyBuffer(OpenCLRuntime::getOpenCLEnv()->getOrCreateCommandQueue(false),
                         (cl_mem)ptr, m_mem, src_offset, dst_offset, cp_size==0?m_size:cp_size, 0, NULL, &this->m_event);  

    if(err != CL_SUCCESS){
        EAGLEEYE_LOGE("fail to copy device buffer with error %s", OpenCLErrorToString(err));
    }
}

void* OpenCLMem::map(cl_bool blocking, int64_t src_offset, int64_t cp_size, int64_t src_offset_y, int64_t cp_size_y){
    if(this->m_mem_status != EAGLEEYE_CL_MEM_READ_WRITE_PINNED){
        return NULL;
    }

    if(m_mapped_ptr != NULL){
        return m_mapped_ptr;
    }

    cl_map_flags map_f = CL_MAP_WRITE|CL_MAP_READ;
    int err;
    void* data = 
        clEnqueueMapBuffer(OpenCLRuntime::getOpenCLEnv()->getOrCreateCommandQueue(false),
            m_mem, 
            blocking, 
            map_f, 
            src_offset, 
            cp_size==0?m_size:cp_size, 
            0, 
            NULL, 
            &this->m_event, 
            &err);

    if(err != CL_SUCCESS){
        EAGLEEYE_LOGD("fail to map device ptr with error %s",OpenCLErrorToString(err));
        return NULL;
    }

    m_mapped_ptr = data;
    return data;
}

void OpenCLMem::unmap(){
    if(this->m_mem_status != EAGLEEYE_CL_MEM_READ_WRITE_PINNED){
        return;
    }
    if(m_mapped_ptr == NULL){
        return;
    }

    int err = 
        clEnqueueUnmapMemObject(OpenCLRuntime::getOpenCLEnv()->getOrCreateCommandQueue(false), 
            m_mem, 
            (void*)m_mapped_ptr, 
            0, 
            NULL, 
            &this->m_event);

    if(err != CL_SUCCESS){
        EAGLEEYE_LOGD("fail to unmap device ptr with error %s", OpenCLErrorToString(err));
    }
    m_mapped_ptr = NULL;
}
//*************************************************************//

//******************        OpenCLIMAGE        ******************//
OpenCLImage::OpenCLImage(OpenCLMemStatus mem_status,
                        std::string name, 
                        unsigned int rows,
                        unsigned int cols, 
                        unsigned int channels, 
                        EagleeyeType pixel_type,
                        void* host_ptr){
    this->m_name = name;
    // channels
    switch (channels)
    {
    case 1:
        m_image_format.image_channel_order=CL_R;
        break;
    case 3:
        m_image_format.image_channel_order=CL_RGB;
        break;
    case 4:
        m_image_format.image_channel_order=CL_RGBA;
        break;
    default:
        m_image_format.image_channel_order=CL_R;
        break;
    }
    this->m_channels = channels;

    // pixel type
    switch (pixel_type)
    {
    case EAGLEEYE_UCHAR:
        m_image_format.image_channel_data_type=CL_UNSIGNED_INT8;
        break;
    case EAGLEEYE_FLOAT:
        m_image_format.image_channel_data_type=CL_FLOAT;
        break;
    default:
        m_image_format.image_channel_data_type=CL_FLOAT;
        break;
    }
    this->m_pixel_type = pixel_type;

    this->m_rows = rows;
    this->m_cols = cols;
    // mem status
    int err;
    switch (mem_status)
    {
    case EAGLEEYE_CL_MEM_READ:
        m_image = 
            clCreateImage2D(OpenCLRuntime::getOpenCLEnv()->context,
                            CL_MEM_READ_ONLY,
                            &m_image_format,
                            cols,
                            rows,
                            0,
                            NULL,
                            &err);
        break;
    case EAGLEEYE_CL_MEM_WRITE:
        m_image = 
            clCreateImage2D(OpenCLRuntime::getOpenCLEnv()->context,
                            CL_MEM_WRITE_ONLY,
                            &m_image_format,
                            cols,
                            rows,
                            0,
                            NULL,
                            &err);
        break;
    case EAGLEEYE_CL_MEM_READ_WRITE_PINNED:
        m_image = 
            clCreateImage2D(OpenCLRuntime::getOpenCLEnv()->context,
                            CL_MEM_READ_WRITE|CL_MEM_COPY_HOST_PTR,
                            &m_image_format,
                            cols,
                            rows,
                            0,
                            host_ptr,
                            &err);
        break;
    default:
        m_image = 
            clCreateImage2D(OpenCLRuntime::getOpenCLEnv()->context,
                            CL_MEM_READ_WRITE,
                            &m_image_format,
                            cols,
                            rows,
                            0,
                            NULL,
                            &err);
        break;
    } 
    if(err != CL_SUCCESS){
        EAGLEEYE_LOGE("fail to create image with error %s", OpenCLErrorToString(err));
    }

    m_mem_status = mem_status;
    this->m_mapped_ptr = NULL;
}

OpenCLImage::OpenCLImage(std::string name, unsigned int texture_id){
    this->m_name = name;
    int err;
    m_image = 
        clCreateFromGLTexture2D(OpenCLRuntime::getOpenCLEnv()->context,
                                CL_MEM_READ_ONLY,
                                GL_TEXTURE_2D,
                                0,
                                texture_id,
                                &err);
    if(err != CL_SUCCESS){
        EAGLEEYE_LOGE("Fail to create image from GL Texture2D with error %s.", OpenCLErrorToString(err));
        this->m_cols = 0;
        this->m_rows = 0;
        return;
    }

    size_t width;
    clGetImageInfo(m_image, CL_IMAGE_WIDTH, sizeof(size_t), &width, NULL);
    size_t height;
    clGetImageInfo(m_image, CL_IMAGE_HEIGHT, sizeof(size_t), &height, NULL);

    this->m_rows = height;
    this->m_cols = width;
    EAGLEEYE_LOGD("Image texture2d width %d height %d", this->m_cols, this->m_rows);
    this->m_mem_status = EAGLEEYE_CL_MEM_READ;
    this->m_mapped_ptr = NULL;
    clGetImageInfo(m_image, CL_IMAGE_FORMAT, sizeof(cl_image_format), &this->m_image_format, NULL);
}

OpenCLImage::~OpenCLImage(){
    clReleaseMemObject(m_image);
}

void OpenCLImage::copyToDevice(void* host_ptr, cl_bool blocking, int64_t src_offset_x, int64_t dst_offset_x, int64_t cp_size_x, int64_t src_offset_y, int64_t dst_offset_y, int64_t cp_size_y){
    size_t origin[3];
    origin[0] = src_offset_x;
    origin[1] = src_offset_y;
    origin[2] = 0;
    size_t region[3];
    region[0] = cp_size_x==0?m_cols:cp_size_x;
    region[1] = cp_size_y==0?m_rows:cp_size_y;
    region[2] = 1;
    int err = 
        clEnqueueWriteImage(OpenCLRuntime::getOpenCLEnv()->getOrCreateCommandQueue(false), 
                            m_image, 
                            blocking,
                            origin, 
                            region, 
                            0, 
                            0, 
                            host_ptr, 
                            0, 
                            NULL, 
                            &this->m_event);

    if(err != CL_SUCCESS){
        EAGLEEYE_LOGE("fail to write device image with error %s", OpenCLErrorToString(err));
    }
}

void OpenCLImage::copyToHost(void* host_ptr, cl_bool blocking, int64_t src_offset_x, int64_t dst_offset_x, int64_t cp_size_x, int64_t src_offset_y, int64_t dst_offset_y, int64_t cp_size_y){
    size_t origin[3];
    origin[0] = src_offset_x;
    origin[1] = src_offset_y;
    origin[2] = 0;
    size_t region[3];
    region[0] = cp_size_x==0?m_cols:cp_size_x;
    region[1] = cp_size_y==0?m_rows:cp_size_y;
    region[2] = 1;
    int err = 
        clEnqueueReadImage(OpenCLRuntime::getOpenCLEnv()->getOrCreateCommandQueue(false), 
                            m_image, 
                            blocking, 
                            origin, 
                            region, 
                            0,
                            0,
                            host_ptr,
                            0,
                            NULL,
                            &this->m_event);

    if(err != CL_SUCCESS){
        EAGLEEYE_LOGE("fail to read device image with error %s", OpenCLErrorToString(err));
    }
}

void OpenCLImage::copyToDeviceFromDevice(void* host_ptr, cl_bool blocking, int64_t src_offset_x, int64_t dst_offset_x, int64_t cp_size_x, int64_t src_offset_y, int64_t dst_offset_y, int64_t cp_size_y){
    EAGLEEYE_LOGE("dont support");
}

void* OpenCLImage::map(cl_bool blocking, int64_t src_offset_x, int64_t cp_size_x, int64_t src_offset_y, int64_t cp_size_y){
    if(this->m_mem_status != EAGLEEYE_CL_MEM_READ_WRITE_PINNED){
        return NULL;
    }
    
    if(m_mapped_ptr != NULL){
        return m_mapped_ptr;
    }

    size_t origin[3];
    origin[0] = src_offset_x;
    origin[1] = src_offset_y;
    origin[2] = 0;
    size_t region[3];
    region[0] = cp_size_x==0?m_cols:cp_size_x;
    region[1] = cp_size_y==0?m_rows:cp_size_y;
    region[2] = 1;
    cl_map_flags map_f = CL_MAP_WRITE|CL_MAP_READ;
    int err;
    size_t image_row_pitch;
    void* data = 
        clEnqueueMapImage(OpenCLRuntime::getOpenCLEnv()->getOrCreateCommandQueue(false), 
                            m_image, 
                            blocking, 
                            map_f, 
                            origin, 
                            region, 
                            &image_row_pitch,
                            0,
                            0,
                            NULL,
                            &m_event,
                            &err);

    if(err != CL_SUCCESS){
        EAGLEEYE_LOGD("fail to map device ptr with error %s", OpenCLErrorToString(err));
        return NULL;
    }
    m_mapped_ptr = data;
    return data;
}

void OpenCLImage::unmap(){
    if(this->m_mem_status != EAGLEEYE_CL_MEM_READ_WRITE_PINNED){
        return;
    }
    if(m_mapped_ptr == NULL){
        return;
    }

    int err = 
        clEnqueueUnmapMemObject(OpenCLRuntime::getOpenCLEnv()->getOrCreateCommandQueue(false), 
                                m_image,
                                (void*)m_mapped_ptr, 
                                0, 
                                NULL, 
                                &this->m_event);

    if(err != CL_SUCCESS){
        EAGLEEYE_LOGD("fail to unmap device ptr with error %s", OpenCLErrorToString(err));
    }
    m_mapped_ptr = NULL;
}
//*************************************************************//

//******************     OpenCLKernelGroup   ******************//
OpenCLKernelGroup::OpenCLKernelGroup(std::vector<std::string> kernel_groups, std::string program_name, std::string options){
    // get opencl env
    m_env = OpenCLRuntime::getOpenCLEnv(); 
    
    // create opencl kernel
    int err;    
    std::vector<std::string>::iterator iter, iend(kernel_groups.end());
    for(iter = kernel_groups.begin(); iter != iend; ++iter){
        if((*iter).length() > 0){
            this->createKernel(*iter, program_name, options);
        }
    }
}

OpenCLKernelGroup::OpenCLKernelGroup(std::vector<std::string> kernel_groups, std::string program_name, std::set<std::string> build_options){
    std::string build_options_str;
    for (auto &option : build_options) {
        build_options_str += " " + option;
    }

    // 1.step create opencl kernel
    m_env = OpenCLRuntime::getOpenCLEnv();     
    std::vector<std::string>::iterator iter, iend(kernel_groups.end());
    for(iter = kernel_groups.begin(); iter != iend; ++iter){
        if((*iter).length() > 0){
            this->createKernel(*iter, program_name, build_options_str);
        }
    }
}

OpenCLKernelGroup::~OpenCLKernelGroup(){
    std::map<std::string, cl_kernel>::iterator iter, iend(m_kernels.end());
    for(iter=m_kernels.begin(); iter != iend; ++iter){
        clReleaseKernel(iter->second);
    }

    std::map<std::string, OpenCLObject*>::iterator ii, iiend(m_mems.end());
    for(ii=m_mems.begin(); ii!=iiend; ++ii){
        delete ii->second;
    }
}

void OpenCLKernelGroup::createDeviceMem(std::string name, unsigned int size, OpenCLMemStatus mem_status){
    if(m_mems.find(name) != m_mems.end()){
        if(((OpenCLMem*)m_mems[name])->m_size == size){
            // 存在buffer并且大小相同
            return;
        }

        delete m_mems[name];
    }
    
    m_mems[name] = new OpenCLMem(mem_status, name, size);
}

void OpenCLKernelGroup::createDeviceImage(std::string name, 
                                            unsigned int rows, 
                                            unsigned int cols, 
                                            unsigned int channels, 
                                            EagleeyeType pixel, 
                                            OpenCLMemStatus mem_status){
    if(m_mems.find(name) != m_mems.end()){
        OpenCLImage* s = (OpenCLImage*)m_mems[name];
        if(s->m_rows == rows && 
            s->m_cols == cols && 
            s->m_pixel_type == pixel && 
            s->m_channels == channels){
            // 存在image并且大小相同
            return;
        }

        delete m_mems[name];
    }
    
    m_mems[name] = new OpenCLImage(mem_status, name, rows, cols, channels, pixel);
}

void OpenCLKernelGroup::createKernel(std::string kernel_name, std::string program_name, std::string options){
    // log 
    EAGLEEYE_LOGD("Create kernel %s from %s (%d in %d).", kernel_name.c_str(), program_name.c_str(), m_kernels.size(), m_kernels.size() + 1);
    cl_program program = m_env->compileProgram(program_name, options);

    int kernel_err;
    cl_kernel kernel = clCreateKernel(program, kernel_name.c_str(), &kernel_err);
    if (!kernel || kernel_err != CL_SUCCESS){
        EAGLEEYE_LOGE("Failed to create %s kernel with err code %d.", kernel_name.c_str(), OpenCLErrorToString(kernel_err));
    }
    m_kernels[kernel_name] = kernel;
}

int OpenCLKernelGroup::run(std::string kernel_name, size_t work_dims, size_t* global_size, size_t* local_size, bool block){
    cl_event event;
    int err = 
        clEnqueueNDRangeKernel(OpenCLRuntime::getOpenCLEnv()->getOrCreateCommandQueue(false), 
                                m_kernels[kernel_name], 
                                work_dims, 
                                NULL, 
                                global_size, 
                                local_size, 
                                0,
                                NULL, 
                                &event);

    if(err != CL_SUCCESS){
        EAGLEEYE_LOGD("Failed to launch kernel %s with err %s", kernel_name.c_str(), OpenCLErrorToString(err));
        return -1;
    }

    if(block){
        err = 
            clEnqueueWaitForEvents(OpenCLRuntime::getOpenCLEnv()->getOrCreateCommandQueue(false), 1, &event);

        if(err != CL_SUCCESS){
            EAGLEEYE_LOGD("Failed to launch kernel %s with err %s", kernel_name.c_str(), OpenCLErrorToString(err));
            return -1;
        }
    }
    else{
        // 加入等待事件
        this->m_events[kernel_name] = event;
    }
    return 0;
}

int OpenCLKernelGroup::finish(){
    int event_waiting_num = m_events.size();
    cl_event* event_waiting_list = NULL;

    if(event_waiting_num > 0){
        cl_event* event_waiting_list = new cl_event[event_waiting_num];
        std::map<std::string, cl_event>::iterator iter,iend(m_events.end());
        int event_i = 0;
        for(iter = m_events.begin(); iter != iend; ++iter){
            event_waiting_list[event_i] = iter->second;
            event_i += 1;
        }
    }

    int err = 
            clEnqueueWaitForEvents(OpenCLRuntime::getOpenCLEnv()->getOrCreateCommandQueue(false), 
                                    event_waiting_num, 
                                    event_waiting_list);
    
    if(event_waiting_list != NULL){
        delete []event_waiting_list;
    }

    if(err != CL_SUCCESS){
        EAGLEEYE_LOGD("fail waiting finish with err %s", OpenCLErrorToString(err));
        return -1;
    }

    return 0;
}

void OpenCLKernelGroup::copyToHost(std::string mem_name, void* host_ptr){
    this->m_mems[mem_name]->copyToHost(host_ptr);
}

void OpenCLKernelGroup::copyToDevice(std::string mem_name, void* host_ptr){
    this->m_mems[mem_name]->copyToDevice(host_ptr);
}

void OpenCLKernelGroup::swap(std::string left_name, std::string right_name){
    OpenCLObject* temp = this->m_mems[left_name];
    this->m_mems[left_name] = this->m_mems[right_name];
    this->m_mems[right_name] = temp;
}

void* OpenCLKernelGroup::map(std::string mem_name, cl_bool blocking){
    return this->m_mems[mem_name]->map(blocking);
}
void OpenCLKernelGroup::unmap(std::string mem_name){
    this->m_mems[mem_name]->unmap();
}

//*************************************************************//
std::string DtToCLDt(const EagleeyeType dt) {
  switch (dt) {
    case EAGLEEYE_FLOAT:
      return "float";
    case EAGLEEYE_HALF_FLOAT:
      return "half";
    case EAGLEEYE_UCHAR:
      return "uchar";
    default:
      EAGLEEYE_LOGE("Unsupported data type: %d",int(dt));
      return "";
  }
}

std::string DtToCLCMDDt(const EagleeyeType dt) {
  switch (dt) {
    case EAGLEEYE_FLOAT:
      return "f";
    case EAGLEEYE_HALF_FLOAT:
      return "h";
    default:
      EAGLEEYE_LOGE("Not supported data type for opencl cmd data type");
      return "";
  }
}

std::shared_ptr<OpenCLKernelGroupManager> OpenCLKernelGroupManager::m_kernel_manager;    
OpenCLKernelGroupManager::OpenCLKernelGroupManager(){

}
OpenCLKernelGroupManager::~OpenCLKernelGroupManager(){
    std::map<std::string, OpenCLKernelGroup*>::iterator iter, iend(m_kernel_map.end());
    for(iter = m_kernel_map.begin(); iter != iend; ++iter){
        delete iter->second;
    }
    m_kernel_map.clear();
}

std::shared_ptr<OpenCLKernelGroupManager> OpenCLKernelGroupManager::getInstance(){
    if(m_kernel_manager.get() == NULL){
        m_kernel_manager = 
            std::shared_ptr<OpenCLKernelGroupManager>(new OpenCLKernelGroupManager(), 
                                [](OpenCLKernelGroupManager* ptr){delete ptr;});
    }

    return m_kernel_manager;
}

bool OpenCLKernelGroupManager::add(std::string name, OpenCLKernelGroup* k){
    if(this->m_kernel_map.find(name) != this->m_kernel_map.end()){
        EAGLEEYE_LOGE("%s has been in OpenCLKernelGroupManager", name.c_str());
        return false;
    }

    m_kernel_map[name] = k;
    return true;
}

bool OpenCLKernelGroupManager::release(std::string name){
    if(this->m_kernel_map.find(name) == this->m_kernel_map.end()){
        EAGLEEYE_LOGE("%s not in OpenCLKernelGroupManager", name.c_str());
        return false;
    }

    m_kernel_map.erase(m_kernel_map.find(name));
    return true;
}

void OpenCLKernelGroupManager::clear(){
    std::map<std::string, OpenCLKernelGroup*>::iterator iter, iend(m_kernel_map.end());
    for(iter = m_kernel_map.begin(); iter != iend; ++iter){
        delete iter->second;
    }
    m_kernel_map.clear();
}

OpenCLKernelGroup* OpenCLKernelGroupManager::kgroup(std::string name){
    if(this->m_kernel_map.find(name) == this->m_kernel_map.end()){
        return NULL;
    }

    return this->m_kernel_map[name];
}
}


#endif  

