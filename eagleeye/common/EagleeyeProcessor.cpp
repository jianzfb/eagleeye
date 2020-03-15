#include "eagleeye/common/EagleeyeProcessor.h"
#include "eagleeye/common/EagleeyeOpenCL.h"
#include "eagleeye/common/EagleeyeTime.h"
namespace eagleeye{
#define MEMCOPY_ITERATIONS  100
#define DEFAULT_SIZE        ( 32 * ( 1 << 20 ) )    //32 M

Processor::Processor(){

}

Processor::~Processor(){

}

double Processor::getL(EagleeyeRuntime runtime){
    // 1.step load from file

    // 2.step online test
    double L = 0;
    switch (runtime.type())
    {
    case EAGLEEYE_CPU:
        L = 0;
        break;
    case EAGLEEYE_GPU:
        L = 0;
        break;
    case EAGLEEYE_QUALCOMM_DSP:
        L = 0;
        break;
    default:
        break;
    }
    return L;
}

double Processor::getB(EagleeyeRuntime from_runtime, EagleeyeRuntime to_runtime){
    double B = 0.0;
    // 1.step load from file

    // 2.step online test
    if(from_runtime.type() == EAGLEEYE_CPU && to_runtime.type() == EAGLEEYE_GPU){
        B = Processor::test_OCL_HostToDeviceTransfer(DEFAULT_SIZE,DIRECT,PAGEABLE);
    }
    else if(from_runtime.type() == EAGLEEYE_GPU && to_runtime.type() == EAGLEEYE_CPU){
        B = Processor::test_OCL_DeviceToHostTransfer(DEFAULT_SIZE, DIRECT, PAGEABLE);
    }
    
    return B;
}

double Processor::test_OCL_DeviceToHostTransfer(unsigned int memSize, GPU_AccessMode accMode, GPU_MemoryMode memMode)
{
    double elapsedTimeInSec = 0.0;
    double bandwidthInMBs = 0.0;
#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
    unsigned char *h_data = NULL;
    cl_mem cmPinnedData = NULL;
    cl_mem cmDevData = NULL;
    cl_int ciErrNum = CL_SUCCESS;
    cl_command_queue cqCommandQueue = clCreateCommandQueue(OpenCLRuntime::getOpenCLEnv()->context, OpenCLRuntime::getOpenCLEnv()->device_id, 0, &ciErrNum);

    //allocate and init host memory, pinned or conventional
    if(memMode == PINNED)
    {
        // Create a host buffer
        cmPinnedData = clCreateBuffer(OpenCLRuntime::getOpenCLEnv()->context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, memSize, NULL, &ciErrNum);
        oclCheckError(ciErrNum, CL_SUCCESS);

        // Get a mapped pointer
        h_data = (unsigned char*)clEnqueueMapBuffer(cqCommandQueue, cmPinnedData, CL_TRUE, CL_MAP_WRITE, 0, memSize, 0, NULL, NULL, &ciErrNum);
        oclCheckError(ciErrNum, CL_SUCCESS);

        //initialize 
        for(unsigned int i = 0; i < memSize/sizeof(unsigned char); i++)
        {
            h_data[i] = (unsigned char)(i & 0xff);
        }

        // unmap and make data in the host buffer valid
        ciErrNum = clEnqueueUnmapMemObject(cqCommandQueue, cmPinnedData, (void*)h_data, 0, NULL, NULL);
        oclCheckError(ciErrNum, CL_SUCCESS);
    }
    else 
    {
        // standard host alloc
        h_data = (unsigned char *)malloc(memSize);

        //initialize 
        for(unsigned int i = 0; i < memSize/sizeof(unsigned char); i++)
        {
            h_data[i] = (unsigned char)(i & 0xff);
        }
    }

    // allocate device memory 
    cmDevData = clCreateBuffer(OpenCLRuntime::getOpenCLEnv()->context, CL_MEM_READ_WRITE, memSize, NULL, &ciErrNum);
    oclCheckError(ciErrNum, CL_SUCCESS);

    // initialize device memory 
    if(memMode == PINNED)
    {
	    // Get a mapped pointer
        h_data = (unsigned char*)clEnqueueMapBuffer(cqCommandQueue, cmPinnedData, CL_TRUE, CL_MAP_WRITE, 0, memSize, 0, NULL, NULL, &ciErrNum);	        

        ciErrNum = clEnqueueWriteBuffer(cqCommandQueue, cmDevData, CL_FALSE, 0, memSize, h_data, 0, NULL, NULL);
        oclCheckError(ciErrNum, CL_SUCCESS);
    }
    else
    {
        ciErrNum = clEnqueueWriteBuffer(cqCommandQueue, cmDevData, CL_FALSE, 0, memSize, h_data, 0, NULL, NULL);
        oclCheckError(ciErrNum, CL_SUCCESS);
    }
    oclCheckError(ciErrNum, CL_SUCCESS);

    // Sync queue to host, start timer 0, and copy data from GPU to Host
    ciErrNum = clFinish(cqCommandQueue);
    // shrDeltaT(0);
    EAGLEEYE_TIME_START(device2host);
    if(accMode == DIRECT)
    { 
        // DIRECT:  API access to device buffer 
        for(unsigned int i = 0; i < MEMCOPY_ITERATIONS; i++)
        {
            ciErrNum = clEnqueueReadBuffer(cqCommandQueue, cmDevData, CL_FALSE, 0, memSize, h_data, 0, NULL, NULL);
            oclCheckError(ciErrNum, CL_SUCCESS);
        }
        ciErrNum = clFinish(cqCommandQueue);
        oclCheckError(ciErrNum, CL_SUCCESS);
    } 
    else 
    {
        // MAPPED: mapped pointers to device buffer for conventional pointer access
        void* dm_idata = clEnqueueMapBuffer(cqCommandQueue, cmDevData, CL_TRUE, CL_MAP_WRITE, 0, memSize, 0, NULL, NULL, &ciErrNum);
        oclCheckError(ciErrNum, CL_SUCCESS);
        for(unsigned int i = 0; i < MEMCOPY_ITERATIONS; i++)
        {
            memcpy(h_data, dm_idata, memSize);
        }
        ciErrNum = clEnqueueUnmapMemObject(cqCommandQueue, cmDevData, dm_idata, 0, NULL, NULL);
        oclCheckError(ciErrNum, CL_SUCCESS);
    }
    
    //get the the elapsed time in seconds
    // elapsedTimeInSec = shrDeltaT(0);
    EAGLEEYE_TIME_END(device2host);
    elapsedTimeInSec = EAGLEEYE_TIME_GET(device2host) / (1000.0*1000.0);

    //calculate bandwidth in MB/s
    bandwidthInMBs = ((double)memSize * (double)MEMCOPY_ITERATIONS) / (elapsedTimeInSec * (double)(1 << 20));

    //clean up memory
    if(cmDevData)clReleaseMemObject(cmDevData);
    if(cmPinnedData) 
    {
	    clEnqueueUnmapMemObject(cqCommandQueue, cmPinnedData, (void*)h_data, 0, NULL, NULL);	
	    clReleaseMemObject(cmPinnedData);	
    }
    else{
        free(h_data);
    }

    h_data = NULL;
    clReleaseCommandQueue(cqCommandQueue);
#endif
    return bandwidthInMBs;
}

double Processor::test_OCL_HostToDeviceTransfer(unsigned int memSize, GPU_AccessMode accMode, GPU_MemoryMode memMode)
{
    double elapsedTimeInSec = 0.0;
    double bandwidthInMBs = 0.0;
#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
    unsigned char* h_data = NULL;
    cl_mem cmPinnedData = NULL;
    cl_mem cmDevData = NULL;
    cl_int ciErrNum = CL_SUCCESS;
    cl_command_queue cqCommandQueue = clCreateCommandQueue(OpenCLRuntime::getOpenCLEnv()->context, OpenCLRuntime::getOpenCLEnv()->device_id, 0, &ciErrNum);

    // Allocate and init host memory, pinned or conventional
    if(memMode == PINNED)
    { 
        // Create a host buffer
        cmPinnedData = clCreateBuffer(OpenCLRuntime::getOpenCLEnv()->context, CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR, memSize, NULL, &ciErrNum);
        oclCheckError(ciErrNum, CL_SUCCESS);

        // Get a mapped pointer
        h_data = (unsigned char*)clEnqueueMapBuffer(cqCommandQueue, cmPinnedData, CL_TRUE, CL_MAP_WRITE, 0, memSize, 0, NULL, NULL, &ciErrNum);
        oclCheckError(ciErrNum, CL_SUCCESS);

        //initialize 
        for(unsigned int i = 0; i < memSize/sizeof(unsigned char); i++)
        {
            h_data[i] = (unsigned char)(i & 0xff);
        }
	
        // unmap and make data in the host buffer valid
        ciErrNum = clEnqueueUnmapMemObject(cqCommandQueue, cmPinnedData, (void*)h_data, 0, NULL, NULL);
        oclCheckError(ciErrNum, CL_SUCCESS);
		h_data = NULL;  // buffer is unmapped
    }
    else 
    {
        // standard host alloc
        h_data = (unsigned char *)malloc(memSize);

        //initialize 
        for(unsigned int i = 0; i < memSize/sizeof(unsigned char); i++)
        {
            h_data[i] = (unsigned char)(i & 0xff);
        }
    }

    // allocate device memory 
    cmDevData = clCreateBuffer(OpenCLRuntime::getOpenCLEnv()->context, CL_MEM_READ_WRITE, memSize, NULL, &ciErrNum);
    oclCheckError(ciErrNum, CL_SUCCESS);

    // Sync queue to host, start timer 0, and copy data from Host to GPU
    clFinish(cqCommandQueue);
    // shrDeltaT(0);
    EAGLEEYE_TIME_START(host2device);
    if(accMode == DIRECT)
    { 
	    if(memMode == PINNED) 
        {
            // Get a mapped pointer
            h_data = (unsigned char*)clEnqueueMapBuffer(cqCommandQueue, cmPinnedData, CL_TRUE, CL_MAP_READ, 0, memSize, 0, NULL, NULL, &ciErrNum);
            oclCheckError(ciErrNum, CL_SUCCESS);
	    }

        // DIRECT:  API access to device buffer 
        for(unsigned int i = 0; i < MEMCOPY_ITERATIONS; i++)
        {
                ciErrNum = clEnqueueWriteBuffer(cqCommandQueue, cmDevData, CL_FALSE, 0, memSize, h_data, 0, NULL, NULL);
                oclCheckError(ciErrNum, CL_SUCCESS);
        }
        ciErrNum = clFinish(cqCommandQueue);
        oclCheckError(ciErrNum, CL_SUCCESS);
    } 
    else 
    {
        // MAPPED: mapped pointers to device buffer and conventional pointer access
        void* dm_idata = clEnqueueMapBuffer(cqCommandQueue, cmDevData, CL_TRUE, CL_MAP_WRITE, 0, memSize, 0, NULL, NULL, &ciErrNum);
		oclCheckError(ciErrNum, CL_SUCCESS);
		if(memMode == PINNED ) 
		{
			h_data = (unsigned char*)clEnqueueMapBuffer(cqCommandQueue, cmPinnedData, CL_TRUE, CL_MAP_READ, 0, memSize, 0, NULL, NULL, &ciErrNum); 
            oclCheckError(ciErrNum, CL_SUCCESS); 
        } 
        for(unsigned int i = 0; i < MEMCOPY_ITERATIONS; i++)
        {
            memcpy(dm_idata, h_data, memSize);
        }
        ciErrNum = clEnqueueUnmapMemObject(cqCommandQueue, cmDevData, dm_idata, 0, NULL, NULL);
        oclCheckError(ciErrNum, CL_SUCCESS);
    }
    
    //get the the elapsed time in seconds
    // elapsedTimeInSec = shrDeltaT(0);
    EAGLEEYE_TIME_END(host2device);
    elapsedTimeInSec = EAGLEEYE_TIME_GET(host2device) / (1000.0*1000.0);

    //calculate bandwidth in MB/s
    bandwidthInMBs = ((double)memSize * (double)MEMCOPY_ITERATIONS)/(elapsedTimeInSec * (double)(1 << 20));

    //clean up memory
    if(cmDevData)clReleaseMemObject(cmDevData);
    if(cmPinnedData) 
    {
	    clEnqueueUnmapMemObject(cqCommandQueue, cmPinnedData, (void*)h_data, 0, NULL, NULL);
	    clReleaseMemObject(cmPinnedData);
    }
    else{
        free(h_data);
    }
    h_data = NULL;
#endif
    return bandwidthInMBs;
}
}