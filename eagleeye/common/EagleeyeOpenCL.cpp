#include "eagleeye/common/EagleeyeOpenCL.h"
#include "eagleeye/common/EagleeyeLog.h"
#include <iostream>

#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
#include <CL/opencl.h>
namespace eagleeye{


template<>
void OpenCLKernelGroup::setKernelArg<std::string>(std::string kernel_name, int index, std::string name){
    int err = clSetKernelArg(m_kernels[kernel_name], index, sizeof(cl_mem), m_mems[name]->getObject());
    if(err != CL_SUCCESS){
        EAGLEEYE_LOGE("Failed to set arg %d for kernel %s", index, kernel_name.c_str());
    }
}

//******************        OpenCLMem        ******************//
OpenCLMem::OpenCLMem(OpenCLMemStatus mem_status, std::string name, unsigned int size){
    int err;
    switch(mem_status){
        case EAGLEEYE_CL_MEM_READ:
            m_mem = clCreateBuffer(OpenCLRuntime::getOpenCLEnv()->context,  CL_MEM_READ_ONLY, size, NULL, &err);
            break;
        case EAGLEEYE_CL_MEM_WRITE:
            m_mem = clCreateBuffer(OpenCLRuntime::getOpenCLEnv()->context,  CL_MEM_WRITE_ONLY, size, NULL, &err);
            break;
        case EAGLEEYE_CL_MEM_READ_WRITE_PINNED:
            m_mem = clCreateBuffer(OpenCLRuntime::getOpenCLEnv()->context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, size, NULL, &err);
            break;
        default:
            m_mem = clCreateBuffer(OpenCLRuntime::getOpenCLEnv()->context,  CL_MEM_READ_WRITE, size, NULL, &err);
            break;
    }
    if(err != CL_SUCCESS){
        EAGLEEYE_LOGE("fail to create buffer %s width code %d",this->m_name.c_str(), err);
    }
    m_name = name;
    m_size = size;
    m_mem_status = mem_status;
    m_mapped_ptr = NULL;
}
OpenCLMem::~OpenCLMem(){
    clReleaseMemObject(m_mem);
}

void OpenCLMem::copyToDevice(cl_command_queue queue, void* host_ptr, cl_bool blocking){
    int err = clEnqueueWriteBuffer(queue, m_mem, blocking, 0, m_size, host_ptr, 0, NULL, NULL);

    if(err != CL_SUCCESS){
        EAGLEEYE_LOGE("fail to write device buffer %s",this->m_name.c_str());
    }
}

void OpenCLMem::copyToHost(cl_command_queue queue, void* host_ptr, cl_bool blocking){
    int err = clEnqueueReadBuffer(queue, m_mem, blocking, 0, m_size, host_ptr, 0, NULL, NULL ); 
 
    if(err != CL_SUCCESS){
        EAGLEEYE_LOGE("fail to read device buffer %s",this->m_name.c_str());
    }
}

void OpenCLMem::copyToDeviceFromDevice(cl_command_queue queue, void* ptr, cl_bool blocking){
    int err = clEnqueueCopyBuffer(queue, (cl_mem)ptr, m_mem, 0, 0, m_size, 0, NULL, NULL);  
    if(err != CL_SUCCESS){
        EAGLEEYE_LOGE("fail to copy device buffer %s",this->m_name.c_str());
    }
}

void* OpenCLMem::map(cl_command_queue queue, size_t* row_pitch){
    if(this->m_mem_status != EAGLEEYE_CL_MEM_READ_WRITE_PINNED){
        return NULL;
    }

    if(m_mapped_ptr != NULL){
        return m_mapped_ptr;
    }

    cl_map_flags map_f = CL_MAP_WRITE|CL_MAP_READ;
    int err;
    void* data = clEnqueueMapBuffer(queue, m_mem, CL_TRUE, map_f, 0, this->m_size, 0, NULL, NULL, &err);
    if(err != CL_SUCCESS){
        EAGLEEYE_LOGD("fail to map device ptr (error code %d)", err);
        return NULL;
    }

    m_mapped_ptr = data;
    return data;
}

void OpenCLMem::unmap(cl_command_queue queue){
    if(this->m_mem_status != EAGLEEYE_CL_MEM_READ_WRITE_PINNED){
        return;
    }
    if(m_mapped_ptr == NULL){
        return;
    }

    int err = clEnqueueUnmapMemObject(queue, m_mem, (void*)m_mapped_ptr, 0, NULL, NULL);
    if(err != CL_SUCCESS){
        EAGLEEYE_LOGD("fail to unmap device ptr");
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
                        EagleeyeType pixel_type){
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
    default:
        m_image_format.image_channel_order=CL_RGBA;
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
        m_image=clCreateImage2D(OpenCLRuntime::getOpenCLEnv()->context,CL_MEM_READ_ONLY,&m_image_format,cols,rows,0,NULL,&err);
        break;
    case EAGLEEYE_CL_MEM_WRITE:
        m_image=clCreateImage2D(OpenCLRuntime::getOpenCLEnv()->context,CL_MEM_WRITE_ONLY,&m_image_format,cols,rows,0,NULL,&err);
        break;
    case EAGLEEYE_CL_MEM_READ_WRITE_PINNED:
        m_image=clCreateImage2D(OpenCLRuntime::getOpenCLEnv()->context,CL_MEM_READ_WRITE|CL_MEM_ALLOC_HOST_PTR,&m_image_format,cols,rows,0,NULL,&err);
        break;
    default:
        m_image=clCreateImage2D(OpenCLRuntime::getOpenCLEnv()->context,CL_MEM_READ_WRITE,&m_image_format,cols,rows,0,NULL,&err);
        break;
    } 
    if(err != CL_SUCCESS){
        EAGLEEYE_LOGE("fail to create image2D %s with code %d",this->m_name.c_str(), err);
    }

    m_mem_status = mem_status;
    this->m_mapped_ptr = NULL;
}

OpenCLImage::~OpenCLImage(){
    clReleaseMemObject(m_image);
}

void OpenCLImage::copyToDevice(cl_command_queue queue, void* host_ptr, cl_bool blocking){
    size_t origin[3];
    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;
    size_t region[3];
    region[0] = m_cols;
    region[1] = m_rows;
    region[2] = 1;
    int err = clEnqueueWriteImage(queue, m_image, blocking, origin, region, 0, 0, host_ptr, 0, NULL, NULL);
    if(err != CL_SUCCESS){
        EAGLEEYE_LOGE("fail to write device image %s",this->m_name.c_str());
    }
}

void OpenCLImage::copyToHost(cl_command_queue queue, void* host_ptr, cl_bool blocking){
    size_t origin[3];
    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;
    size_t region[3];
    region[0] = m_cols;
    region[1] = m_rows;
    region[2] = 1;
    int err = clEnqueueReadImage(queue, m_image, blocking, origin, region, 0,0,host_ptr,0,NULL,NULL);
    if(err != CL_SUCCESS){
        EAGLEEYE_LOGE("fail to read device image %s",this->m_name.c_str());
    }
}

void OpenCLImage::copyToDeviceFromDevice(cl_command_queue queue, void* host_ptr, cl_bool blocking){
    EAGLEEYE_LOGE("dont support");
}

void* OpenCLImage::map(cl_command_queue queue, size_t* row_pitch){
    if(this->m_mem_status != EAGLEEYE_CL_MEM_READ_WRITE_PINNED){
        return NULL;
    }
    
    if(m_mapped_ptr != NULL){
        return m_mapped_ptr;
    }

    size_t origin[3];
    origin[0] = 0;
    origin[1] = 0;
    origin[2] = 0;
    size_t region[3];
    region[0] = m_cols;
    region[1] = m_rows;
    region[2] = 1;
    cl_map_flags map_f = CL_MAP_WRITE|CL_MAP_READ;
    int err;
    void* data = clEnqueueMapImage(queue, m_image, CL_TRUE, map_f, origin, region, row_pitch,0,0,NULL,NULL,&err);
    if(err != CL_SUCCESS){
        EAGLEEYE_LOGD("fail to map device ptr (error code %d)",err);
        return NULL;
    }
    m_mapped_ptr = data;
    return data;
}
void OpenCLImage::unmap(cl_command_queue queue){
    if(this->m_mem_status != EAGLEEYE_CL_MEM_READ_WRITE_PINNED){
        return;
    }
    if(m_mapped_ptr == NULL){
        return;
    }

    int err = clEnqueueUnmapMemObject(queue, m_image, (void*)m_mapped_ptr, 0, NULL, NULL);
    if(err != CL_SUCCESS){
        EAGLEEYE_LOGD("fail to unmap device ptr");
    }
    m_mapped_ptr = NULL;
}
//*************************************************************//

//******************     OpenCLKernelGroup   ******************//
OpenCLKernelGroup::OpenCLKernelGroup(std::vector<std::string> kernel_groups, std::string program_name){
    m_env = OpenCLRuntime::getOpenCLEnv(); 
    // 1.step build command queue
    int err;
    m_queue = clCreateCommandQueue(m_env->context, m_env->device_id, 0, &err);
    if (!m_queue){
        EAGLEEYE_LOGE("Failed to create a command commands");
    }
    
    std::vector<std::string>::iterator iter, iend(kernel_groups.end());
    for(iter = kernel_groups.begin(); iter != iend; ++iter){
        if((*iter).length() > 0){
            this->createKernel(*iter, program_name);
        }
    }
}

OpenCLKernelGroup::~OpenCLKernelGroup(){
    clReleaseCommandQueue(m_queue);
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

void OpenCLKernelGroup::createKernel(std::string kernel_name, std::string program_name){
    cl_program program = m_env->compileProgram(program_name);
    int kernel_err;
    cl_kernel kernel = clCreateKernel(program, kernel_name.c_str(), &kernel_err);
    if (!kernel || kernel_err != CL_SUCCESS){
        EAGLEEYE_LOGE("Failed to create %s kernel with err code %d", kernel_name.c_str(), kernel_err);
    }
    m_kernels[kernel_name] = kernel;
}

void OpenCLKernelGroup::run(std::string kernel_name, size_t work_dims, size_t* global_size, size_t* local_size){
    int err = clEnqueueNDRangeKernel(m_queue, m_kernels[kernel_name], work_dims, NULL, global_size, local_size, 0, NULL, NULL);
    if(err != CL_SUCCESS){
        EAGLEEYE_LOGD("Failed to split work group for kernel %s with err code %d", kernel_name.c_str(), err);
    }

    err = clFinish(m_queue);
    if(err != CL_SUCCESS){
        EAGLEEYE_LOGD("Failed to run kernel %s with err code %d", kernel_name.c_str(), err);
    }
}

void OpenCLKernelGroup::copyToHost(std::string mem_name, void* host_ptr){
    this->m_mems[mem_name]->copyToHost(this->m_queue, host_ptr);
}

void OpenCLKernelGroup::copyToDevice(std::string mem_name, void* host_ptr){
    this->m_mems[mem_name]->copyToDevice(this->m_queue, host_ptr);
}

void OpenCLKernelGroup::swap(std::string left_name, std::string right_name){
    OpenCLObject* temp = this->m_mems[left_name];
    this->m_mems[left_name] = this->m_mems[right_name];
    this->m_mems[right_name] = temp;
}

void* OpenCLKernelGroup::map(std::string mem_name, size_t* row_pitch){
    return this->m_mems[mem_name]->map(this->m_queue, row_pitch);
}
void OpenCLKernelGroup::unmap(std::string mem_name){
    this->m_mems[mem_name]->unmap(this->m_queue);
}

//*************************************************************//

const char* oclErrorString(cl_int error)
{
	static const char* errorString[] = {
		"CL_SUCCESS",
		"CL_DEVICE_NOT_FOUND",
		"CL_DEVICE_NOT_AVAILABLE",
		"CL_COMPILER_NOT_AVAILABLE",
		"CL_MEM_OBJECT_ALLOCATION_FAILURE",
		"CL_OUT_OF_RESOURCES",
		"CL_OUT_OF_HOST_MEMORY",
		"CL_PROFILING_INFO_NOT_AVAILABLE",
		"CL_MEM_COPY_OVERLAP",
		"CL_IMAGE_FORMAT_MISMATCH",
		"CL_IMAGE_FORMAT_NOT_SUPPORTED",
		"CL_BUILD_PROGRAM_FAILURE",
		"CL_MAP_FAILURE",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"",
		"CL_INVALID_VALUE",
		"CL_INVALID_DEVICE_TYPE",
		"CL_INVALID_PLATFORM",
		"CL_INVALID_DEVICE",
		"CL_INVALID_CONTEXT",
		"CL_INVALID_QUEUE_PROPERTIES",
		"CL_INVALID_COMMAND_QUEUE",
		"CL_INVALID_HOST_PTR",
		"CL_INVALID_MEM_OBJECT",
		"CL_INVALID_IMAGE_FORMAT_DESCRIPTOR",
		"CL_INVALID_IMAGE_SIZE",
		"CL_INVALID_SAMPLER",
		"CL_INVALID_BINARY",
		"CL_INVALID_BUILD_OPTIONS",
		"CL_INVALID_PROGRAM",
		"CL_INVALID_PROGRAM_EXECUTABLE",
		"CL_INVALID_KERNEL_NAME",
		"CL_INVALID_KERNEL_DEFINITION",
		"CL_INVALID_KERNEL",
		"CL_INVALID_ARG_INDEX",
		"CL_INVALID_ARG_VALUE",
		"CL_INVALID_ARG_SIZE",
		"CL_INVALID_KERNEL_ARGS",
		"CL_INVALID_WORK_DIMENSION",
		"CL_INVALID_WORK_GROUP_SIZE",
		"CL_INVALID_WORK_ITEM_SIZE",
		"CL_INVALID_GLOBAL_OFFSET",
		"CL_INVALID_EVENT_WAIT_LIST",
		"CL_INVALID_EVENT",
		"CL_INVALID_OPERATION",
		"CL_INVALID_GL_OBJECT",
		"CL_INVALID_BUFFER_SIZE",
		"CL_INVALID_MIP_LEVEL",
		"CL_INVALID_GLOBAL_WORK_SIZE",
	};

	const int errorCount = sizeof(errorString) / sizeof(errorString[0]);
	const int index = -error;
	return (index >= 0 && index < errorCount) ? errorString[index] : "Unspecified Error";
}

}

#endif  