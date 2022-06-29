#include "eagleeye/common/EagleeyeRuntime.h"
#include "eagleeye/runtime/cpu/cpu_runtime.h"
#include "eagleeye/runtime/gpu/opencl_runtime.h"
namespace eagleeye
{
EagleeyeCPU::EagleeyeCPU():EagleeyeRuntime(EAGLEEYE_CPU){ 
    m_cpu_runtime = NULL;
}

CPURuntime* EagleeyeCPU::getOrCreateRuntime(){
    if(m_cpu_runtime == NULL){
        m_cpu_runtime = new CPURuntime();
    }

    return m_cpu_runtime;
}

EagleeyeGPU::EagleeyeGPU():EagleeyeRuntime(EAGLEEYE_GPU){
    m_gpu_runtime = OpenCLRuntime::getOpenCLEnv();
}


} // namespace eagleeye
