#ifndef _EAGLEEYE_OPENCLRUNTIME_H_
#define _EAGLEEYE_OPENCLRUNTIME_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/common/EagleeyeLog.h"
#include <map>
#include <string>
#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
#include <CL/opencl.h>
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

const std::string OpenCLErrorToString(cl_int error);

// Helper function to get OpenCL error string from constant
const char* oclErrorString(cl_int error);

// companion inline function for error checking and exit on error WITH Cleanup Callback (if supplied)
// *********************************************************************
inline void __oclCheckErrorEX(cl_int iSample, cl_int iReference, void (*pCleanup)(int), const char* cFile, const int iLine)
{
    // An error condition is defined by the sample/test value not equal to the reference
    if (iReference != iSample)
    {
        // If the sample/test value isn't equal to the ref, it's an error by defnition, so override 0 sample/test value
        iSample = (iSample == 0) ? -9999 : iSample; 

        // Log the error info
        EAGLEEYE_LOGD("\n !!! Error # %i (%s) at line %i , in file %s !!!\n\n", iSample, oclErrorString(iSample), iLine, cFile);
        // Cleanup and exit, or just exit if no cleanup function pointer provided.  Use iSample (error code in this case) as process exit code.
        if (pCleanup != NULL)
        {
            pCleanup(iSample);
        }
        else 
        {
            exit(iSample);
        }
    }
}
// Error and Exit Handling Macros... 
// *********************************************************************
// Full error handling macro with Cleanup() callback (if supplied)... 
// (Companion Inline Function lower on page)
#define oclCheckErrorEX(a, b, c) __oclCheckErrorEX(a, b, c, __FILE__ , __LINE__) 

// Short version without Cleanup() callback pointer
// Both Input (a) and Reference (b) are specified as args
#define oclCheckError(a, b) oclCheckErrorEX(a, b, 0) 

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
     * @brief add custom source
     * 
     * @param name 
     * @param source 
     */
    void addCustomSource(std::string name, std::string source);
    
    void addSourceCode(std::string name, std::string source);

    /**
     * @brief Get the Command Queue object
     * 
     * @param group 
     */
    cl_command_queue getCommandQueue(std::string group);

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

    size_t getKernelMaxWorkGroupSize(){return device_group_size;};

    cl_device_id device_id;             // compute device id 
    cl_context context;                 // compute context
    // cl_program math_program;
    // cl_program algorithm_program;
    // cl_program square_program;
    cl_platform_id platform_id;
    cl_uint cu_num;
    size_t device_group_size;
    cl_ulong max_global_mem_size;
    cl_ulong device_global_mem_cache_size;
    cl_ulong max_constant_buffer_size;
    cl_ulong max_local_mem_size;

    std::map<std::string, cl_program> m_programs;
    std::map<std::string, std::string> m_sources;
    std::map<std::string, cl_command_queue> m_command_queue;

private:
    OpenCLRuntime();
    bool writeBinaryToFile(const char* fileName, const char* birary, size_t numBytes); 
    bool readBinaryFromFile(const char* fileName, char*& binary, size_t& numBytes);
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
    
};

}
#else
class OpenCLRuntime{
public:
    OpenCLRuntime(){};
    ~OpenCLRuntime(){};

    static OpenCLRuntime* getOpenCLEnv();
};
#endif
#endif