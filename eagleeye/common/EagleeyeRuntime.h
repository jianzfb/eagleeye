#ifndef _EAGLEEYE_RUNTIME_H_
#define _EAGLEEYE_RUNTIME_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/runtime/cpu/cpu_runtime.h"
#include <string>

namespace eagleeye{
class OpenCLRuntime;
class EagleeyeRuntime{
public:
    EagleeyeRuntime(EagleeyeRuntimeType runtime=EAGLEEYE_CPU)
        :m_runtime(runtime){
    }
    virtual ~EagleeyeRuntime(){
    }

    EagleeyeRuntimeType type() const{
        return this->m_runtime;
    }

    std::string device() const{
        switch (m_runtime){
        case EAGLEEYE_CPU:
            return "CPU";
        case EAGLEEYE_GPU:
            return "GPU";
        case EAGLEEYE_QUALCOMM_DSP:
            return "DSP";
        case EAGLEEYE_QUALCOMM_NPU:
            return "NPU";
        default:
            return "UNKNOWN";
        }
    }

    std::string prefix(std::string prefix) const{
        char buf[1024] = {0};
        switch (m_runtime){
        case EAGLEEYE_CPU:
            snprintf(buf, sizeof(buf), prefix.c_str(), "cpu");
            return std::string(buf);
        case EAGLEEYE_GPU:
            snprintf(buf, sizeof(buf), prefix.c_str(), "gpu");
            return std::string(buf);
        case EAGLEEYE_QUALCOMM_DSP:
            snprintf(buf, sizeof(buf), prefix.c_str(), "qualcomm_dsp");
            return std::string(buf);
        case EAGLEEYE_QUALCOMM_NPU:
            snprintf(buf, sizeof(buf), prefix.c_str(), "qualcomm_npu");
            return std::string(buf);
        default:
            return prefix;
        }
    }
    
private:
    EagleeyeRuntimeType m_runtime;
}; 

class EagleeyeCPU:EagleeyeRuntime{
public:
    EagleeyeCPU();
    virtual ~EagleeyeCPU(){
        delete m_cpu_runtime;
    }

    CPURuntime* getOrCreateRuntime();
    
    CPURuntime* getRuntime(){
        return m_cpu_runtime;
    }

private:
    CPURuntime* m_cpu_runtime;
};

class EagleeyeGPU:EagleeyeRuntime{
public:
    EagleeyeGPU();
    virtual ~EagleeyeGPU(){};

    OpenCLRuntime* getRuntime(){
        return m_gpu_runtime;
    }
private:
    OpenCLRuntime* m_gpu_runtime;
};
}
#endif