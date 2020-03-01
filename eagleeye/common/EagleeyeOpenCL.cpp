#include "eagleeye/common/EagleeyeOpenCL.h"
#include "eagleeye/common/EagleeyeLog.h"
#include <iostream>

#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
#include <CL/opencl.h>
#include "../../EagleeyeCL_CODE.h"
namespace eagleeye{
std::shared_ptr<OpenCLEnv> OpenCLEnv::m_env;
std::string OpenCLEnv::m_writable_path;
OpenCLEnv::OpenCLEnv(){
    // initialize opencl environment
    if(this->m_writable_path.size() == 0){
        this->m_writable_path = "./";
    }
    this->m_sources["algorithm"] = OPENCL_ALGORITHM_CL;
    this->m_sources["square"] = OPENCL_SQUARE_CL;
    this->init();
}

OpenCLEnv::~OpenCLEnv(){
    // 1.step release context
    clReleaseContext(context);

    // 2.step release program
    std::map<std::string, cl_program>::iterator iter, iend(m_programs.end());
    for(iter = m_programs.begin(); iter != iend; ++iter){
        clReleaseProgram(iter->second);
    }

    // 3.step release command queue
    std::map<std::string, cl_command_queue>::iterator citer, ciend(m_command_queue.end());
    for(citer=m_command_queue.begin(); citer != ciend; ++citer){
        clReleaseCommandQueue(citer->second);
    }
}

void OpenCLEnv::addCustomSource(std::string name, std::string source){
    this->m_sources[name] = source;
}

cl_command_queue OpenCLEnv::getCommandQueue(std::string group){
    if(m_command_queue.find(group) != m_command_queue.end()){
        return m_command_queue[group];
    }

    int err;
    m_command_queue[group] = clCreateCommandQueue(context, device_id, 0, &err);
    if (err != CL_SUCCESS){
        EAGLEEYE_LOGE("Failed to create a command queue for group %s with error code %d", group.c_str(), err);
        return NULL;
    }
    return m_command_queue[group];
}


void OpenCLEnv::setWritablePath(const char* writable_path){
    OpenCLEnv::m_writable_path = writable_path;
}

OpenCLEnv* OpenCLEnv::getOpenCLEnv(){
    if(OpenCLEnv::m_env.get() == NULL){
        OpenCLEnv::m_env = std::shared_ptr<OpenCLEnv>(new OpenCLEnv());
    }
    return OpenCLEnv::m_env.get();
}

bool OpenCLEnv::init(){
    // initialize opencl environment
    cl_uint ret_num_platforms;
    int err;                            // error code returned from api calls
    // Connect to a compute device
    err = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
    if(err != CL_SUCCESS){
        EAGLEEYE_LOGE("Failed to get platform info");
        return false;
    }

    int gpu = 1;
    err = clGetDeviceIDs(platform_id, gpu ? CL_DEVICE_TYPE_GPU : CL_DEVICE_TYPE_CPU, 1, &device_id, NULL);
    if (err != CL_SUCCESS){
        EAGLEEYE_LOGE("Failed to create a device group!");
        return false;
    }

    // get device cu
    clGetDeviceInfo(device_id,CL_DEVICE_MAX_COMPUTE_UNITS,sizeof(cl_uint),&this->cu_num,NULL);
    EAGLEEYE_LOGD("computing units num %d at selected device",this->cu_num);

    // get device max work group size
    clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE,sizeof(size_t), &this->device_group_size, NULL);
    EAGLEEYE_LOGD("max work group size %d at selected device",this->device_group_size);

    // get maxGlobalMemSize
    clGetDeviceInfo(device_id, CL_DEVICE_GLOBAL_MEM_SIZE,sizeof(cl_ulong), &this->max_global_mem_size, NULL);
    this->max_global_mem_size = this->max_global_mem_size/1024/1024;
    EAGLEEYE_LOGD("maxGlobalMemSize: %lu(MB)", this->max_global_mem_size);
 
    // get maxConstantBufferSize
    clGetDeviceInfo(device_id, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE,sizeof(cl_ulong), &this->max_constant_buffer_size, NULL);
    this->max_constant_buffer_size = this->max_constant_buffer_size/1024;
    EAGLEEYE_LOGD("maxConstantBufferSize: %lu(KB)", this->max_constant_buffer_size);

    // get maxLocalMemSize
    clGetDeviceInfo(device_id, CL_DEVICE_LOCAL_MEM_SIZE,sizeof(cl_ulong), &this->max_local_mem_size, NULL);
    this->max_local_mem_size = this->max_local_mem_size/1024;
    EAGLEEYE_LOGD("maxLocalMemSize: %lu(KB)", this->max_local_mem_size);

    // Create a compute context 
    context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
    if(err != CL_SUCCESS){
        EAGLEEYE_LOGE("Failed to create a compute context!");
        return false;
    }

    return true;
}

bool OpenCLEnv::writeBinaryToFile(const char* fileName, const char* birary, size_t numBytes){
    FILE *output = NULL;
    output = fopen(fileName, "wb");
    if(output == NULL)
        return false;

    fwrite(birary, sizeof(char), numBytes, output);
    fclose(output);

    return true;
}
bool OpenCLEnv::readBinaryFromFile(const char* KernelBinary, char*& binary, size_t& numBytes){
	FILE* binaryFile;
	binaryFile = fopen(KernelBinary, "rb");
	if (!binaryFile){
        EAGLEEYE_LOGE("couldnt load %s kernel binary",KernelBinary);
		return false;
	}
	fseek(binaryFile, 0L, SEEK_END);
	size_t size = ftell(binaryFile);
	rewind(binaryFile);
	binary = NULL;
	if (size <= 0){
        EAGLEEYE_LOGE("file size[%d] is err!\n", (int)size);
		fclose(binaryFile);
		return false;
	}
	else{
		binary = (char *)malloc(size);
	}
	if (!binary){
        EAGLEEYE_LOGE("malloc is err!");
		fclose(binaryFile);
		return false;
	}
	fread(binary, sizeof(char), size, binaryFile);
	fclose(binaryFile);
    numBytes = size;
    return true;
}

Matrix<float> cl_square(const Matrix<float>& data){
    Matrix<float> data_cp = data;
    if(!data.isfull()){
        data_cp = data.clone();
    }
    
    int rows = data_cp.rows();
    int cols = data_cp.cols();
    unsigned int count = rows*cols;
    size_t work_dims = 1;
    size_t global_size[1] = {count};
    float* data_ptr = data_cp.dataptr();
    EAGLEEYE_OPENCL_DECLARE_KERNEL_GROUP(square);    
    EAGLEEYE_OPENCL_KERNEL_GROUP(square, square, square);
    EAGLEEYE_OPENCL_CREATE_READ_BUFFER(square, input, sizeof(float)*rows*cols);
    EAGLEEYE_OPENCL_CREATE_WRITE_BUFFER(square, output, sizeof(float)*rows*cols);
    EAGLEEYE_OPENCL_COPY_TO_DEVICE(square, input, data_ptr);
    
    EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(square, square, 0, input);
    EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(square, square, 1, output);
    EAGLEEYE_OPENCL_KERNEL_SET_ARG(square, square, 3, count);
    EAGLEEYE_OPENCL_KERNEL_RUN(square, square, work_dims, global_size, global_size);
    
    Matrix<float> results(rows, cols);
    float* results_ptr = results.dataptr();
    EAGLEEYE_OPENCL_COPY_TO_HOST(square, output, results_ptr);

    EAGLEEYE_OPENCL_RELEASE_KERNEL_GROUP(square);
    return results;
}

cl_program OpenCLEnv::compileProgram(std::string program_name){
    // only compile once
    if(this->m_programs.find(program_name) != this->m_programs.end()){
        return this->m_programs[program_name];
    }
    
    // 1.step try to load cl binary
    int err;                            // error code returned from api calls
    char* binary_compiled_algorithm_cl = NULL;
    size_t binary_compiled_algorithm_cl_size = 0;
    std::string algorithm_program_bin = m_writable_path + "//" + program_name + ".cl.bin";
    EAGLEEYE_LOGD("try to load algorithm kernel from %s", algorithm_program_bin.c_str());
    bool is_ok = this->readBinaryFromFile(algorithm_program_bin.c_str(),binary_compiled_algorithm_cl, binary_compiled_algorithm_cl_size);
    if(is_ok){
        cl_program program = clCreateProgramWithBinary(context, 1, &device_id, &binary_compiled_algorithm_cl_size, (const unsigned char **) & binary_compiled_algorithm_cl, NULL, &err);
        free(binary_compiled_algorithm_cl);

        if (!program || err != CL_SUCCESS){
            EAGLEEYE_LOGE("Failed to create compute algorithm program");
            return NULL;
        }

        err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
        if (err != CL_SUCCESS){
            size_t len;
            char buffer[2048];
    
            EAGLEEYE_LOGE("Failed to build program executable!");
            clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
            EAGLEEYE_LOGE("%s",buffer);
            return NULL;
        }

        EAGLEEYE_LOGD("success to load algorithm program");

        this->m_programs[program_name] = program;
        return program;
    }

    // 2.step try compile from source
    if(this->m_sources.find(program_name) == this->m_sources.end()){
        EAGLEEYE_LOGD("dont have module %s source code", program_name.c_str());
        return NULL;
    }

    EAGLEEYE_LOGD("try to compile algorithm program from source");
    const char* source_code = this->m_sources[program_name].c_str();
    cl_program program = clCreateProgramWithSource(context, 1, (const char **) &source_code, NULL, &err);
    if (!program || err != CL_SUCCESS){
        EAGLEEYE_LOGE("Failed to create compute program");
        return NULL;
    }

    // Build the program executable
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (err != CL_SUCCESS){
        size_t len;
        char buffer[2048];

        EAGLEEYE_LOGE("Failed to build program executable with error code %d", err);
        err = clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        EAGLEEYE_LOGD("clGetProgramBuildInfo return code %d", err);
        EAGLEEYE_LOGD("clGetProgramBuildInfo build info \"%s\" ",buffer);
        return NULL;
    }

    //save complied program
    EAGLEEYE_LOGD("try to write complied algorithm program");
    char **binaries = (char **)malloc( sizeof(char *) * 1 ); //只有一个设备
    size_t *binarySizes = (size_t*)malloc( sizeof(size_t) * 1 );

    err = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES,sizeof(size_t) * 1, binarySizes, NULL);
    binaries[0] = (char *)malloc( sizeof(char) * binarySizes[0]);
    err = clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(char *) * 1, binaries, NULL);            
    this->writeBinaryToFile(algorithm_program_bin.c_str(), binaries[0],binarySizes[0]);

    free(binaries);
    free(binarySizes);
    EAGLEEYE_LOGD("finish write complied algorithm program");
    this->m_programs[program_name] = program;
    return this->m_programs[program_name];
}

//******************        OpenCLMem        ******************//
OpenCLMem::OpenCLMem(OpenCLMemStatus mem_status, std::string name, unsigned int size){
    int err;
    switch(mem_status){
        case EAGLEEYE_CL_MEM_READ:
            m_mem = clCreateBuffer(OpenCLEnv::getOpenCLEnv()->context,  CL_MEM_READ_ONLY, size, NULL, &err);
            break;
        case EAGLEEYE_CL_MEM_WRITE:
            m_mem = clCreateBuffer(OpenCLEnv::getOpenCLEnv()->context,  CL_MEM_WRITE_ONLY, size, NULL, &err);
            break;
        case EAGLEEYE_CL_MEM_READ_WRITE_PINNED:
            m_mem = clCreateBuffer(OpenCLEnv::getOpenCLEnv()->context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, size, NULL, &err);
            break;
        default:
            m_mem = clCreateBuffer(OpenCLEnv::getOpenCLEnv()->context,  CL_MEM_READ_WRITE, size, NULL, &err);
            break;
    }
    if(err != CL_SUCCESS){
        EAGLEEYE_LOGE("fail to create buffer %s width code %d",this->m_name.c_str(), err);
    }
    m_name = name;
    m_size = size;

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

void* OpenCLMem::map(cl_command_queue queue){
    if(this->m_mem_status != EAGLEEYE_CL_MEM_READ_WRITE_PINNED){
        return NULL;
    }
    if(m_mapped_ptr != NULL){
        return m_mapped_ptr;
    }

    cl_map_flags map_f = CL_MAP_WRITE;
    int err;
    void* data = clEnqueueMapBuffer(queue, m_mem, CL_TRUE, map_f, 0, this->m_size, 0, NULL, NULL, &err);
    if(err != CL_SUCCESS){
        EAGLEEYE_LOGD("fail to map device ptr");
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
        m_image=clCreateImage2D(OpenCLEnv::getOpenCLEnv()->context,CL_MEM_READ_ONLY,&m_image_format,cols,rows,0,NULL,&err);
        break;
    case EAGLEEYE_CL_MEM_WRITE:
        m_image=clCreateImage2D(OpenCLEnv::getOpenCLEnv()->context,CL_MEM_WRITE_ONLY,&m_image_format,cols,rows,0,NULL,&err);
        break;
    case EAGLEEYE_CL_MEM_READ_WRITE_PINNED:
        m_image=clCreateImage2D(OpenCLEnv::getOpenCLEnv()->context,CL_MEM_READ_WRITE|CL_MEM_ALLOC_HOST_PTR,&m_image_format,cols,rows,0,NULL,&err);
        break;
    default:
        m_image=clCreateImage2D(OpenCLEnv::getOpenCLEnv()->context,CL_MEM_READ_WRITE,&m_image_format,cols,rows,0,NULL,&err);
        break;
    } 
    if(err != CL_SUCCESS){
        EAGLEEYE_LOGE("fail to create image2D %s with code %d",this->m_name.c_str(), err);
    }

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

void* OpenCLImage::map(cl_command_queue queue){
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
    cl_map_flags map_f = CL_MAP_WRITE;
    int err;
    void* data = clEnqueueMapImage(queue, m_image, CL_TRUE, map_f, origin, region, 0,0,0,NULL,NULL,&err);
    if(err != CL_SUCCESS){
        EAGLEEYE_LOGD("fail to map device ptr");
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
    m_env = OpenCLEnv::getOpenCLEnv(); 
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

void* OpenCLKernelGroup::map(std::string mem_name){
    return this->m_mems[mem_name]->map(this->m_queue);
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