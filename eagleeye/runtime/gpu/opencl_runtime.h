#ifndef _EAGLEEYE_OPENCLRUNTIME_H_
#define _EAGLEEYE_OPENCLRUNTIME_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/common/EagleeyeLog.h"
#include <map>
#include <string>
#include <memory>
#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/opencl.h>
#endif
namespace eagleeye{

// Max execution time of OpenCL kernel for tuning to prevent UI stuck.
const float kMaxKernelExecTime = 1000.0;  // microseconds

// Base GPU cache size used for computing local work group size.
const int32_t kBaseGPUMemCacheSize = 16384;

enum GPUType {
  QUALCOMM_ADRENO,
  MALI,
  PowerVR,
  UNKNOWN,
};

enum OpenCLVersion {
  CL_VER_UNKNOWN,
  CL_VER_1_0,
  CL_VER_1_1,
  CL_VER_1_2,
  CL_VER_2_0,
  CL_VER_2_1,
};

/**
 * @brief get opencl error string 
 */ 
const char* OpenCLErrorToString(cl_int error);

/**
 * @brief check opencl error
 */ 
EagleeyeError OpenCLCheckError(cl_int error);


class OpenCLRuntime{
public:
    static OpenCLRuntime* getOpenCLEnv();
    static void setWritablePath(const char* writable_path="./");
    virtual ~OpenCLRuntime();

    /**
     * @brief compile program 
     * 
     * @param program_name 
     * @return cl_program 
     */
    cl_program compileProgram(std::string program_name, std::string options);

    /**
     * @brief register program source
     */ 
    void registerProgramSource(std::string name, std::string source);

    /**
     * @brief get command queue
     */
    cl_command_queue getOrCreateCommandQueue(bool is_order);
    
    /**
     * @brief get GPU TYPE
     * 
     * @return GPUType 
     */
    GPUType getGpuType() const{return this->m_gpu_type;};

    /**
     * @brief Get the Open CL Version object
     * 
     * @return OpenCLVersion 
     */
    OpenCLVersion getOpenCLVersion() const {return this->m_opencl_version;}
    
    /**
     * @brief get Max Image 2D Size
     * 
     * @return std::vector<size_t> 
     */
    std::vector<size_t> getMaxImage2DSize(){return {m_max_image_width, m_max_image_height};};

    /**
     * @brief get kernel max work group size
     */ 
    size_t getKernelMaxWorkGroupSize(){return device_group_size;};

    cl_device_id device_id;             // compute device id 
    cl_context context;                 // compute context
    cl_platform_id platform_id;
    cl_uint cu_num;
    size_t device_group_size;
    cl_ulong max_global_mem_size;
    cl_ulong device_global_mem_cache_size;
    cl_ulong max_constant_buffer_size;
    cl_ulong max_local_mem_size;

    std::map<std::string, std::string> m_register_program_source;
    std::map<std::string, cl_program> m_programs;

private:
    OpenCLRuntime();
    /**
     * @brief write kernel to binary file
     */ 
    bool writeBinaryToFile(const char* fileName, const char* birary, size_t numBytes); 

    /**
     * @brief read kernel from binary 
     */ 
    bool readBinaryFromFile(const char* fileName, char*& binary, size_t& numBytes);

    /**
     * @brief get kernel source code
     */ 
    EagleeyeError getProgramSourceByName(const std::string &program_name, std::string *source);

    /**
     * @brief 
     */ 
    bool init();
    
    GPUType ParseGPUType(std::string device_name);
    OpenCLVersion ParseDeviceVersion(std::string device_version);

    static std::string m_writable_path;
    static std::shared_ptr<OpenCLRuntime> m_env;
    OpenCLVersion m_opencl_version;
    GPUType m_gpu_type;
    bool m_is_profiling_enabled;
    
    size_t m_max_image_height;
    size_t m_max_image_width;

    cl_command_queue m_order_queue;
    cl_command_queue m_out_of_order_queue;
};

}
#else
namespace eagleeye{
class OpenCLRuntime{
public:
    OpenCLRuntime(){};
    ~OpenCLRuntime(){};

    static OpenCLRuntime* getOpenCLEnv();
};
}
#endif
#endif