#include "eagleeye/runtime/gpu/opencl_runtime.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeStr.h"

#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
#include <CL/opencl.h>
#include <CL/cl.h>
#include "../../../EagleeyeCL_CODE.h"

namespace eagleeye
{
const std::string OpenCLErrorToString(cl_int error) {
  switch (error) {
    case CL_SUCCESS:
      return "CL_SUCCESS";
    case CL_DEVICE_NOT_FOUND:
      return "CL_DEVICE_NOT_FOUND";
    case CL_DEVICE_NOT_AVAILABLE:
      return "CL_DEVICE_NOT_AVAILABLE";
    case CL_COMPILER_NOT_AVAILABLE:
      return "CL_COMPILER_NOT_AVAILABLE";
    case CL_MEM_OBJECT_ALLOCATION_FAILURE:
      return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
    case CL_OUT_OF_RESOURCES:
      return "CL_OUT_OF_RESOURCES";
    case CL_OUT_OF_HOST_MEMORY:
      return "CL_OUT_OF_HOST_MEMORY";
    case CL_PROFILING_INFO_NOT_AVAILABLE:
      return "CL_PROFILING_INFO_NOT_AVAILABLE";
    case CL_MEM_COPY_OVERLAP:
      return "CL_MEM_COPY_OVERLAP";
    case CL_IMAGE_FORMAT_MISMATCH:
      return "CL_IMAGE_FORMAT_MISMATCH";
    case CL_IMAGE_FORMAT_NOT_SUPPORTED:
      return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
    case CL_BUILD_PROGRAM_FAILURE:
      return "CL_BUILD_PROGRAM_FAILURE";
    case CL_MAP_FAILURE:
      return "CL_MAP_FAILURE";
    case CL_MISALIGNED_SUB_BUFFER_OFFSET:
      return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
    case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST:
      return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
    // case CL_COMPILE_PROGRAM_FAILURE:
    //   return "CL_COMPILE_PROGRAM_FAILURE";
    // case CL_LINKER_NOT_AVAILABLE:
    //   return "CL_LINKER_NOT_AVAILABLE";
    // case CL_LINK_PROGRAM_FAILURE:
    //   return "CL_LINK_PROGRAM_FAILURE";
    // case CL_DEVICE_PARTITION_FAILED:
    //   return "CL_DEVICE_PARTITION_FAILED";
    // case CL_KERNEL_ARG_INFO_NOT_AVAILABLE:
    //   return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";
    // case CL_INVALID_VALUE:
    //   return "CL_INVALID_VALUE";
    // case CL_INVALID_DEVICE_TYPE:
    //   return "CL_INVALID_DEVICE_TYPE";
    // case CL_INVALID_PLATFORM:
    //   return "CL_INVALID_PLATFORM";
    // case CL_INVALID_DEVICE:
    //   return "CL_INVALID_DEVICE";
    // case CL_INVALID_CONTEXT:
    //   return "CL_INVALID_CONTEXT";
    // case CL_INVALID_QUEUE_PROPERTIES:
    //   return "CL_INVALID_QUEUE_PROPERTIES";
    // case CL_INVALID_COMMAND_QUEUE:
    //   return "CL_INVALID_COMMAND_QUEUE";
    // case CL_INVALID_HOST_PTR:
    //   return "CL_INVALID_HOST_PTR";
    // case CL_INVALID_MEM_OBJECT:
    //   return "CL_INVALID_MEM_OBJECT";
    // case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:
    //   return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
    // case CL_INVALID_IMAGE_SIZE:
    //   return "CL_INVALID_IMAGE_SIZE";
    // case CL_INVALID_SAMPLER:
    //   return "CL_INVALID_SAMPLER";
    // case CL_INVALID_BINARY:
    //   return "CL_INVALID_BINARY";
    // case CL_INVALID_BUILD_OPTIONS:
    //   return "CL_INVALID_BUILD_OPTIONS";
    // case CL_INVALID_PROGRAM:
    //   return "CL_INVALID_PROGRAM";
    // case CL_INVALID_PROGRAM_EXECUTABLE:
    //   return "CL_INVALID_PROGRAM_EXECUTABLE";
    // case CL_INVALID_KERNEL_NAME:
    //   return "CL_INVALID_KERNEL_NAME";
    // case CL_INVALID_KERNEL_DEFINITION:
    //   return "CL_INVALID_KERNEL_DEFINITION";
    // case CL_INVALID_KERNEL:
    //   return "CL_INVALID_KERNEL";
    // case CL_INVALID_ARG_INDEX:
    //   return "CL_INVALID_ARG_INDEX";
    // case CL_INVALID_ARG_VALUE:
    //   return "CL_INVALID_ARG_VALUE";
    // case CL_INVALID_ARG_SIZE:
    //   return "CL_INVALID_ARG_SIZE";
    // case CL_INVALID_KERNEL_ARGS:
    //   return "CL_INVALID_KERNEL_ARGS";
    // case CL_INVALID_WORK_DIMENSION:
    //   return "CL_INVALID_WORK_DIMENSION";
    // case CL_INVALID_WORK_GROUP_SIZE:
    //   return "CL_INVALID_WORK_GROUP_SIZE";
    // case CL_INVALID_WORK_ITEM_SIZE:
    //   return "CL_INVALID_WORK_ITEM_SIZE";
    // case CL_INVALID_GLOBAL_OFFSET:
    //   return "CL_INVALID_GLOBAL_OFFSET";
    // case CL_INVALID_EVENT_WAIT_LIST:
    //   return "CL_INVALID_EVENT_WAIT_LIST";
    // case CL_INVALID_EVENT:
    //   return "CL_INVALID_EVENT";
    // case CL_INVALID_OPERATION:
    //   return "CL_INVALID_OPERATION";
    // case CL_INVALID_GL_OBJECT:
    //   return "CL_INVALID_GL_OBJECT";
    // case CL_INVALID_BUFFER_SIZE:
    //   return "CL_INVALID_BUFFER_SIZE";
    // case CL_INVALID_MIP_LEVEL:
    //   return "CL_INVALID_MIP_LEVEL";
    // case CL_INVALID_GLOBAL_WORK_SIZE:
    //   return "CL_INVALID_GLOBAL_WORK_SIZE";
    // case CL_INVALID_PROPERTY:
    //   return "CL_INVALID_PROPERTY";
    // case CL_INVALID_IMAGE_DESCRIPTOR:
    //   return "CL_INVALID_IMAGE_DESCRIPTOR";
    // case CL_INVALID_COMPILER_OPTIONS:
    //   return "CL_INVALID_COMPILER_OPTIONS";
    // case CL_INVALID_LINKER_OPTIONS:
    //   return "CL_INVALID_LINKER_OPTIONS";
    // case CL_INVALID_DEVICE_PARTITION_COUNT:
    //   return "CL_INVALID_DEVICE_PARTITION_COUNT";
#if CL_HPP_TARGET_OPENCL_VERSION >= 200
    case CL_INVALID_PIPE_SIZE:
      return "CL_INVALID_PIPE_SIZE";
    case CL_INVALID_DEVICE_QUEUE:
      return "CL_INVALID_DEVICE_QUEUE";
#endif
    default:
      return makeString("UNKNOWN: ", error);
  }
}    

std::shared_ptr<OpenCLRuntime> OpenCLRuntime::m_env;
std::string OpenCLRuntime::m_writable_path;
OpenCLRuntime::OpenCLRuntime(){
    // initialize opencl environment
    if(this->m_writable_path.size() == 0){
        this->m_writable_path = "./";
    }
    this->m_sources["algorithm"] = OPENCL_ALGORITHM_CL;
    this->init();
}

OpenCLRuntime::~OpenCLRuntime(){
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

void OpenCLRuntime::addCustomSource(std::string name, std::string source){
    this->addSourceCode(name, source);
}

void OpenCLRuntime::addSourceCode(std::string name, std::string source){
    this->m_sources[name] = source;
}

cl_command_queue OpenCLRuntime::getCommandQueue(std::string group){
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

void OpenCLRuntime::setWritablePath(const char* writable_path){
    OpenCLRuntime::m_writable_path = writable_path;
}

OpenCLRuntime* OpenCLRuntime::getOpenCLEnv(){
    if(OpenCLRuntime::m_env.get() == NULL){
        OpenCLRuntime::m_env = std::shared_ptr<OpenCLRuntime>(new OpenCLRuntime());
    }
    return OpenCLRuntime::m_env.get();
}

bool OpenCLRuntime::init(){
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

    clGetDeviceInfo(device_id, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE,sizeof(cl_ulong), &this->device_global_mem_cache_size, NULL);
    EAGLEEYE_LOGD("GlobalMemCacheSize: %lu", this->device_global_mem_cache_size);

    // get maxConstantBufferSize
    clGetDeviceInfo(device_id, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE,sizeof(cl_ulong), &this->max_constant_buffer_size, NULL);
    this->max_constant_buffer_size = this->max_constant_buffer_size/1024;
    EAGLEEYE_LOGD("maxConstantBufferSize: %lu(KB)", this->max_constant_buffer_size);

    clGetDeviceInfo(device_id, CL_DEVICE_IMAGE2D_MAX_HEIGHT,sizeof(size_t), &this->m_max_image_height, NULL);
    clGetDeviceInfo(device_id, CL_DEVICE_IMAGE2D_MAX_WIDTH,sizeof(size_t), &this->m_max_image_width, NULL);
    EAGLEEYE_LOGD("max device IMAGE2D height %d width %d", m_max_image_height, m_max_image_width);

    // get maxLocalMemSize
    clGetDeviceInfo(device_id, CL_DEVICE_LOCAL_MEM_SIZE,sizeof(cl_ulong), &this->max_local_mem_size, NULL);
    this->max_local_mem_size = this->max_local_mem_size/1024;
    EAGLEEYE_LOGD("maxLocalMemSize: %lu(KB)", this->max_local_mem_size);

    // get device name
    char device_string[1024];
    clGetDeviceInfo(device_id, CL_DEVICE_NAME,sizeof(device_string), &device_string, NULL);
    EAGLEEYE_LOGD("GPU device name %s", device_string);
    this->m_gpu_type = ParseGPUType(device_string);

    // get device version
    char version_string[1024];
    clGetDeviceInfo(device_id, CL_DEVICE_VERSION,sizeof(version_string), &version_string, NULL);
    EAGLEEYE_LOGD("OPENCL version %s", version_string);
    this->m_opencl_version = ParseDeviceVersion(version_string);

    // Create a compute context 
    context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
    if(err != CL_SUCCESS){
        EAGLEEYE_LOGE("Failed to create a compute context!");
        return false;
    }

    return true;
}

GPUType OpenCLRuntime::ParseGPUType(std::string device_name) {
  constexpr const char *kQualcommAdrenoGPUStr = "QUALCOMM Adreno(TM)";
  constexpr const char *kMaliGPUStr = "Mali";
  constexpr const char *kPowerVRGPUStr = "PowerVR";

  if (device_name == kQualcommAdrenoGPUStr) {
    return GPUType::QUALCOMM_ADRENO;
  } else if (device_name.find(kMaliGPUStr) != std::string::npos) {
    return GPUType::MALI;
  } else if (device_name.find(kPowerVRGPUStr) != std::string::npos) {
    return GPUType::PowerVR;
  } else {
    return GPUType::UNKNOWN;
  }
}

OpenCLVersion OpenCLRuntime::ParseDeviceVersion(std::string device_version) {
  // OpenCL Device version string format:
  // OpenCL<space><major_version.minor_version><space>
  // <vendor-specific information>
  auto words = split(device_version, " ");
  if (words[1] == "2.1") {
    return OpenCLVersion::CL_VER_2_1;
  } else if (words[1] == "2.0") {
    return OpenCLVersion::CL_VER_2_0;
  } else if (words[1] == "1.2") {
    return OpenCLVersion::CL_VER_1_2;
  } else if (words[1] == "1.1") {
    return OpenCLVersion::CL_VER_1_1;
  } else if (words[1] == "1.0") {
    return OpenCLVersion::CL_VER_1_0;
  } else {
    EAGLEEYE_LOGE("Do not support OpenCL version: %s",words[1].c_str());
    return OpenCLVersion::CL_VER_UNKNOWN;
  }
}

bool OpenCLRuntime::writeBinaryToFile(const char* fileName, const char* birary, size_t numBytes){
    FILE *output = NULL;
    output = fopen(fileName, "wb");
    if(output == NULL)
        return false;

    fwrite(birary, sizeof(char), numBytes, output);
    fclose(output);

    return true;
}
bool OpenCLRuntime::readBinaryFromFile(const char* KernelBinary, char*& binary, size_t& numBytes){
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

cl_program OpenCLRuntime::compileProgram(std::string program_name, std::string options){
    std::cout<<"in compile program"<<std::endl;
    std::cout<<program_name+options<<std::endl;
    // only compile once
    if(this->m_programs.find(program_name+options) != this->m_programs.end()){
        return this->m_programs[program_name+options];
    }
    
    // 1.step try to load cl binary
    int err;                            // error code returned from api calls
    char* binary_compiled_algorithm_cl = NULL;
    size_t binary_compiled_algorithm_cl_size = 0;
    std::string algorithm_program_bin = m_writable_path + "//" + program_name+options + ".cl.bin";
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

        this->m_programs[program_name+options] = program;
        return program;
    }

    // 2.step try compile from source
    if(this->m_sources.find(program_name) == this->m_sources.end()){
        EAGLEEYE_LOGD("dont have module %s source code", program_name.c_str());
        return NULL;
    }

    EAGLEEYE_LOGD("try to compile algorithm program from source");
    const char* source_code = this->m_sources[program_name].c_str();
    // std::cout<<"source code"<<std::endl;
    // std::cout<<source_code<<std::endl;
    cl_program program = clCreateProgramWithSource(context, 1, (const char **) &source_code, NULL, &err);
    if (!program || err != CL_SUCCESS){
        EAGLEEYE_LOGE("Failed to create compute program");
        return NULL;
    }

    // Build the program executable
    // std::string options = "-cl-mad-enable -I "+this->m_writable_path;
    options += " -cl-mad-enable -I "+this->m_writable_path;
    err = clBuildProgram(program, 0, NULL, options.c_str(), NULL, NULL);
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
    this->m_programs[program_name+options] = program;
    return this->m_programs[program_name+options];
}

} // namespace eagleeye
#else
OpenCLRuntime* OpenCLRuntime::getOpenCLEnv(){
  return NULL;
}
#endif
