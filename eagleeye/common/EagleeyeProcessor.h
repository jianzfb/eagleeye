#ifndef _EAGLEEYE_PROCESSOR_H_
#define _EAGLEEYE_PROCESSOR_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/common/EagleeyeRuntime.h"

namespace eagleeye{
enum GPU_TestMode { QUICK_MODE, RANGE_MODE, SHMOO_MODE };
enum GPU_MemcpyKind { DEVICE_TO_HOST, HOST_TO_DEVICE, DEVICE_TO_DEVICE };
enum GPU_MemoryMode { PAGEABLE, PINNED };
enum GPU_AccessMode { MAPPED, DIRECT };    

class Processor{
public:
    Processor();
    virtual ~Processor();

    /**
     * @brief get memory transfer launch time
     * 
     * @param runtime 
     * @return double 
     */
    static double getL(EagleeyeRuntime runtime);

    /**
     * @brief get memory transfer bandwidth
     * 
     * @param from_runtime 
     * @param to_runtime 
     * @return double 
     */
    static double getB(EagleeyeRuntime from_runtime, EagleeyeRuntime to_runtime);

protected:
    static double test_OCL_DeviceToHostTransfer(unsigned int memSize, GPU_AccessMode accMode, GPU_MemoryMode memMode);
    static double test_OCL_HostToDeviceTransfer(unsigned int memSize, GPU_AccessMode accMode, GPU_MemoryMode memMode);

};
}
#endif