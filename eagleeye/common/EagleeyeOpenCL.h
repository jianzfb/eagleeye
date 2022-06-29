#ifndef _EAGLEEYE_EAGLEEYEOPENCL_H_
#define _EAGLEEYE_EAGLEEYEOPENCL_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/common/EagleeyeTime.h"
#include "eagleeye/runtime/gpu/opencl_runtime.h"
#include <memory>
#include <map>
#include <set>

#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
#include <CL/opencl.h>
#include <CL/cl.h>
namespace eagleeye{
enum OpenCLMemStatus{
    EAGLEEYE_CL_MEM_READ,
    EAGLEEYE_CL_MEM_WRITE,
    EAGLEEYE_CL_MEM_READ_WRITE,
    EAGLEEYE_CL_MEM_READ_WRITE_PINNED,
};
    
class OpenCLObject{
public:
    OpenCLObject(): m_event(NULL){};
    virtual ~OpenCLObject(){};

    virtual void copyToHost(void* host_ptr, cl_bool blocking=CL_TRUE, int64_t src_offset=0, int64_t dst_offset=0, int64_t cp_size=0, int64_t src_offset_y=0, int64_t dst_offset_y=0, int64_t cp_size_y=0) = 0;
    virtual void copyToDevice(void* host_ptr, cl_bool blocking=CL_TRUE, int64_t src_offset=0, int64_t dst_offset=0, int64_t cp_size=0, int64_t src_offset_y=0, int64_t dst_offset_y=0, int64_t cp_size_y=0) = 0;
    virtual void copyToDeviceFromDevice(void* ptr, cl_bool blocking=CL_TRUE, int64_t src_offset=0, int64_t dst_offset=0, int64_t cp_size=0, int64_t src_offset_y=0, int64_t dst_offset_y=0, int64_t cp_size_y=0) = 0;
    virtual void* map(cl_bool blocking, int64_t src_offset=0, int64_t cp_size=0, int64_t src_offset_y=0, int64_t cp_size_y=0){return NULL;}
    virtual void unmap(){};

    void finish(){
        cl_int err = 
            clEnqueueWaitForEvents(OpenCLRuntime::getOpenCLEnv()->getOrCreateCommandQueue(false),
                                    1,
                                    &this->m_event);
        if(err != CL_SUCCESS){
            EAGLEEYE_LOGD("mem syn fail with err %s", OpenCLErrorToString(err));
        }
    }

    // 返回opencl对象
    virtual cl_mem getObject() = 0;

    // 返回opencl对象类型
    virtual int type(){return -1;}

    cl_event m_event;
};

class OpenCLMem:public OpenCLObject{
public:
    OpenCLMem(OpenCLMemStatus mem_status, std::string name, unsigned int size, void* host_ptr=NULL);
    virtual ~OpenCLMem();

    virtual void copyToHost(void* host_ptr, cl_bool blocking=CL_TRUE, int64_t src_offset=0, int64_t dst_offset=0, int64_t cp_size=0, int64_t src_offset_y=0, int64_t dst_offset_y=0, int64_t cp_size_y=0);
    virtual void copyToDevice(void* host_ptr, cl_bool blocking=CL_TRUE, int64_t src_offset=0, int64_t dst_offset=0, int64_t cp_size=0, int64_t src_offset_y=0, int64_t dst_offset_y=0, int64_t cp_size_y=0);
    virtual void copyToDeviceFromDevice(void* ptr, cl_bool blocking=CL_TRUE, int64_t src_offset=0, int64_t dst_offset=0, int64_t cp_size=0, int64_t src_offset_y=0, int64_t dst_offset_y=0, int64_t cp_size_y=0);
    virtual void* map(cl_bool blocking, int64_t src_offset=0, int64_t cp_size=0, int64_t src_offset_y=0, int64_t cp_size_y=0);
    virtual void unmap();

    // 返回opencl对象类型
    virtual int type(){return 0;}

    // 返回opencl对象
    virtual cl_mem getObject(){
        return m_mem;
    }
    cl_mem m_mem;
    std::string m_name;
    unsigned int m_size;
    OpenCLMemStatus m_mem_status;

    void* m_mapped_ptr;
};

class OpenCLImage:public OpenCLObject{
public:
    OpenCLImage(OpenCLMemStatus mem_status,
                std::string name, 
                unsigned int rows,
                unsigned int cols, 
                unsigned int channels, 
                EagleeyeType pixel_type,
                void* host_ptr=NULL);
    OpenCLImage(std::string name, unsigned int texture_id);
    virtual ~OpenCLImage();

    virtual void copyToHost(void* host_ptr, cl_bool blocking=CL_TRUE, int64_t src_offset_x=0, int64_t dst_offset_x=0, int64_t cp_size_x=0, int64_t src_offset_y=0, int64_t dst_offset_y=0, int64_t cp_size_y=0);
    virtual void copyToDevice(void* host_ptr, cl_bool blocking=CL_TRUE, int64_t src_offset_x=0, int64_t dst_offset_x=0, int64_t cp_size_x=0, int64_t src_offset_y=0, int64_t dst_offset_y=0, int64_t cp_size_y=0);
    virtual void copyToDeviceFromDevice(void* ptr, cl_bool blocking=CL_TRUE, int64_t src_offset_x=0, int64_t dst_offset_x=0, int64_t cp_size_x=0, int64_t src_offset_y=0, int64_t dst_offset_y=0, int64_t cp_size_y=0);
    virtual void* map(cl_bool blocking, int64_t src_offset_x=0, int64_t cp_size_x=0, int64_t src_offset_y=0, int64_t cp_size_y=0);
    virtual void unmap();

    // 返回opencl对象类型
    virtual int type(){return 1;}

    // 返回opencl对象
    virtual cl_mem getObject(){
        return m_image;
    }

    EagleeyeType m_pixel_type;
    int m_channels;
    cl_image_format m_image_format;
    cl_mem m_image;
    int m_rows;
    int m_cols;
    std::string m_name;
    OpenCLMemStatus m_mem_status;
    void* m_mapped_ptr;
};

class OpenCLKernelGroup{
public:
    /**
     * @brief Construct a new Open C L Kernel Group object
     * 
     * @param group 
     */
    OpenCLKernelGroup(std::vector<std::string> kernel_groups, std::string program_name, std::string options=std::string());
    OpenCLKernelGroup(std::vector<std::string> kernel_groups, std::string program_name, std::set<std::string> build_options);
    
    /**
     * @brief Destroy the Open C L Kernel Group object
     * 
     */
    virtual ~OpenCLKernelGroup();

    /**
     * @brief run kernel
     * 
     * @param kernel_name 
     * @param work_dims 
     * @param global_size 
     * @param lobal_size 
     */
    int run(std::string kernel_name, size_t work_dims, size_t* global_size, size_t* lobal_size, bool block=true);

    /**
     * @biref finish queue
     */
    int finish();

    template<typename T>
    void setKernelArg(std::string kernel_name, int index, T value){
        int err = clSetKernelArg(m_kernels[kernel_name], index, sizeof(T), &value);
        if(err != CL_SUCCESS){
            EAGLEEYE_LOGE("Failed to set arg %d for kernel %s with error %s", index, kernel_name.c_str(), OpenCLErrorToString(err));
        }
    }

    /**
     * @brief copy device mem to host
     * 
     * @param mem_name 
     * @param host_ptr 
     * @param size 
     */
    void copyToHost(std::string mem_name, void* host_ptr);

    /**
     * @brief copy host mem to device
     * 
     * @param mem_name 
     * @param host_ptr 
     * @param size 
     */
    void copyToDevice(std::string mem_name, void* host_ptr);

    /**
     * @brief Create a Device Mem object
     * 
     * @param name device memory buffer name
     * @param size 
     * @param mem_status 
     */
    void createDeviceMem(std::string name, unsigned int size, OpenCLMemStatus mem_status);

    /**
     * @brief Create a Device Image object
     * 
     * @param name 
     * @param rows 
     * @param cols 
     * @param channels 
     * @param pixel 
     * @param mem_status 
     */
    void createDeviceImage(std::string name, unsigned int rows, unsigned int cols, unsigned int channels, EagleeyeType pixel, OpenCLMemStatus mem_status);
    
    /**
     * @brief swap left and right mem
     * 
     * @param left_name 
     * @param right_name 
     */
    void swap(std::string left_name, std::string right_name);

    /**
     * @brief map opencl buffer to host
     * 
     * @param mem_name 
     * @param row_pitch 
     * @return void* 
     */
    void* map(std::string mem_name, cl_bool blocking);

    /**
     * @brief unmap
     * 
     * @param mem_name 
     */
    void unmap(std::string mem_name);

    std::map<std::string, cl_kernel> m_kernels;

protected:
    /**
     * @brief Create a Kernel object
     * 
     * @param kernel_name 
     * @param program_name 
     */
    void createKernel(std::string kernel_name, std::string program_name, std::string options=std::string());

private:
    std::map<std::string, OpenCLObject*> m_mems;
    OpenCLRuntime* m_env;
    std::map<std::string, cl_event> m_events;
};

template<>
void OpenCLKernelGroup::setKernelArg<std::string>(std::string kernel_name, int index, std::string name);

#define EAGLEEYE_OPENCL_REGISTER_SOURCE_CODE(program, code)    \
    OpenCLRuntime::getOpenCLEnv()->registerProgramSource(#program, code);


// #define EAGLEEYE_OPENCL_KERNEL_GROUP(group, program, K1) \
//     group##_kernels = new OpenCLKernelGroup(std::vector<std::string>{#K1}, #program);

// #define EAGLEEYE_OPENCL_KERNEL_2_GROUP(group, program, K1, K2) \
//     group##_kernels = new OpenCLKernelGroup(std::vector<std::string>{#K1, #K2}, #program);

// #define EAGLEEYE_OPENCL_KERNEL_3_GROUP(group, program, K1, K2, K3) \
//     group##_kernels = new OpenCLKernelGroup(std::vector<std::string>{#K1, #K2, #K3}, #program);

// #define EAGLEEYE_OPENCL_KERNEL_4_GROUP(group, program, K1, K2, K3, K4) \
//     group##_kernels = new OpenCLKernelGroup(std::vector<std::string>{#K1, #K2, #K3, #K4}, #program);

#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)
#define VARGS_(_12, _11, _10, _9, _8, _7, _6, _5, _4, _3, _2, _1, N, ...) N 
#define VARGS(...) VARGS_(__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

#define EAGLEEYE_OPENCL_KERNEL_GROUP_1(group, program, a) \
    OpenCLKernelGroupManager::getInstance()->add(#group, new OpenCLKernelGroup(std::vector<std::string>{#a}, #program));
#define EAGLEEYE_OPENCL_KERNEL_GROUP_2(group, program, a,b) \
    OpenCLKernelGroupManager::getInstance()->add(#group, new OpenCLKernelGroup(std::vector<std::string>{#a,#b}, #program));
#define EAGLEEYE_OPENCL_KERNEL_GROUP_3(group, program, a,b,c) \
    OpenCLKernelGroupManager::getInstance()->add(#group, new OpenCLKernelGroup(std::vector<std::string>{#a,#b,#c}, #program));
#define EAGLEEYE_OPENCL_KERNEL_GROUP_4(group, program, a,b,c,d) \
    OpenCLKernelGroupManager::getInstance()->add(#group, new OpenCLKernelGroup(std::vector<std::string>{#a,#b,#c,#d}, #program));
#define EAGLEEYE_OPENCL_KERNEL_GROUP_5(group, program, a,b,c,d,e) \
    OpenCLKernelGroupManager::getInstance()->add(#group, new OpenCLKernelGroup(std::vector<std::string>{#a,#b,#c,#d,#e}, #program));
#define EAGLEEYE_OPENCL_KERNEL_GROUP_6(group, program, a,b,c,d,e,f) \
    OpenCLKernelGroupManager::getInstance()->add(#group, new OpenCLKernelGroup(std::vector<std::string>{#a,#b,#c,#d,#e,#f}, #program));
#define EAGLEEYE_OPENCL_KERNEL_GROUP_7(group, program, a,b,c,d,e,f,g) \
    OpenCLKernelGroupManager::getInstance()->add(#group, new OpenCLKernelGroup(std::vector<std::string>{#a,#b,#c,#d,#e,#f,#g}, #program));
#define EAGLEEYE_OPENCL_KERNEL_GROUP_8(group, program, a,b,c,d,e,f,g,h) \
    OpenCLKernelGroupManager::getInstance()->add(#group, new OpenCLKernelGroup(std::vector<std::string>{#a,#b,#c,#d,#e,#f,#g,#h}, #program));
#define EAGLEEYE_OPENCL_KERNEL_GROUP_9(group, program, a,b,c,d,e,f,g,h,i) \
    OpenCLKernelGroupManager::getInstance()->add(#group, new OpenCLKernelGroup(std::vector<std::string>{#a,#b,#c,#d,#e,#f,#g,#h,#i}, #program));
#define EAGLEEYE_OPENCL_KERNEL_GROUP_10(group, program, a,b,c,d,e,f,g,h,i,j) \
    OpenCLKernelGroupManager::getInstance()->add(#group, new OpenCLKernelGroup(std::vector<std::string>{#a,#b,#c,#d,#e,#f,#g,#h,#i,#j}, #program));
#define EAGLEEYE_OPENCL_KERNEL_GROUP(...) CONCAT(EAGLEEYE_OPENCL_KERNEL_GROUP_,VARGS(__VA_ARGS__))(__VA_ARGS__)

#define EAGLEEYE_OPENCL_KERNEL_OPTION_GROUP(group, program, a, options) \
    gOpenCLKernelGroupManager::getInstance()->add(#group, new OpenCLKernelGroup(std::vector<std::string>{#a}, #program, options));

#define EAGLEEYE_OPENCL_CREATE_READ_BUFFER(group, name, size) \ 
    OpenCLKernelGroupManager::getInstance()->kgroup(#group)->createDeviceMem(#name, size, EAGLEEYE_CL_MEM_READ);

#define EAGLEEYE_OPENCL_CREATE_WRITE_BUFFER(group, name, size) \ 
    OpenCLKernelGroupManager::getInstance()->kgroup(#group)->createDeviceMem(#name, size, EAGLEEYE_CL_MEM_WRITE);

#define EAGLEEYE_OPENCL_CREATE_READ_WRITE_BUFFER(group, name, size) \ 
    OpenCLKernelGroupManager::getInstance()->kgroup(#group)->createDeviceMem(#name, size, EAGLEEYE_CL_MEM_READ_WRITE);

#define EAGLEEYE_OPENCL_CREATE_READ_WRITE_PINNED_BUFFER(group, name, size) \ 
    OpenCLKernelGroupManager::getInstance()->kgroup(#group)->createDeviceMem(#name, size, EAGLEEYE_CL_MEM_READ_WRITE_PINNED);

#define EAGLEEYE_OPENCL_CREATE_READ_IMAGE(group, name, rows, cols, channels, pixel) \ 
    OpenCLKernelGroupManager::getInstance()->kgroup(#group)->createDeviceImage(#name, rows, cols, channels, pixel, EAGLEEYE_CL_MEM_READ);

#define EAGLEEYE_OPENCL_CREATE_WRITE_IMAGE(group, name, rows, cols, channels, pixel) \ 
    OpenCLKernelGroupManager::getInstance()->kgroup(#group)->createDeviceImage(#name, rows, cols, channels, pixel, EAGLEEYE_CL_MEM_WRITE);

#define EAGLEEYE_OPENCL_CREATE_READ_WRITE_IMAGE(group, name, rows, cols, channels, pixel) \ 
    OpenCLKernelGroupManager::getInstance()->kgroup(#group)->createDeviceImage(#name, rows, cols, channels, pixel, EAGLEEYE_CL_MEM_READ_WRITE);

#define EAGLEEYE_OPENCL_CREATE_READ_WRITE_PINNED_IMAGE(group, name, rows, cols, channels, pixel) \ 
    OpenCLKernelGroupManager::getInstance()->kgroup(#group)->createDeviceImage(#name, rows, cols, channels, pixel, EAGLEEYE_CL_MEM_READ_WRITE_PINNED);

#define EAGLEEYE_OPENCL_MAP_BUFFER(group, name) \
    OpenCLKernelGroupManager::getInstance()->kgroup(#group)->map(#name);

#define EAGLEEYE_OPENCL_MAP_IMAGE(group, name, row_pitch) \
    OpenCLKernelGroupManager::getInstance()->kgroup(#group)->map(#name, row_pitch);

#define EAGLEEYE_OPENCL_UNMAP(group, name) \
    OpenCLKernelGroupManager::getInstance()->kgroup(#group)->unmap(#name);

#define EAGLEEYE_OPENCL_COPY_TO_DEVICE(group, name, data) \
    OpenCLKernelGroupManager::getInstance()->kgroup(#group)->copyToDevice(#name, data);

#define EAGLEEYE_OPENCL_COPY_TO_HOST(group, name, data) \
    OpenCLKernelGroupManager::getInstance()->kgroup(#group)->copyToHost(#name, data);

#define EAGLEEYE_OPENCL_SWAP(group, name_1, name_2) \ 
    OpenCLKernelGroupManager::getInstance()->kgroup(#group)->swap(#name_1, #name_2);

#define EAGLEEYE_OPENCL_KERNEL_RUN(group, kernel, work_dims, global_size, local_size) \
    OpenCLKernelGroupManager::getInstance()->kgroup(#group)->run(#kernel, work_dims, global_size, local_size);

#define EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(group, kernel, index, value) \
    OpenCLKernelGroupManager::getInstance()->kgroup(#group)->setKernelArg(#kernel, index, std::string(#value));

#define EAGLEEYE_OPENCL_KERNEL_SET_ARG(group, kernel, index, value) \
    OpenCLKernelGroupManager::getInstance()->kgroup(#group)->setKernelArg(#kernel, index, value);

#define EAGLEEYE_OPECNCL_GLOBAL_SIZE(global_size, local_size) ((global_size)%(local_size)==0) ? (global_size) : (((global_size)/(local_size)+1)*(local_size))

// #define COPY_VARGS_(_5, _4, _3, _2, _1, N, ...) N 
// #define COPY_VARGS(...) COPY_VARGS_(__VA_ARGS__,1,0)

// #define EAGLEEYE_OPENCL_COPY_FROM_HOST_1(kernel, buff, size, data, tag) \
//     clEnqueueWriteBuffer(OpenCLRuntime::getOpenCLEnv()->m_command_map[tag+#kernel], kernel##_##buff, CL_TRUE, 0, size, data, 0, NULL, NULL);
// #define EAGLEEYE_OPENCL_COPY_FROM_HOST_0(kernel, buff, size, data) EAGLEEYE_OPENCL_COPY_FROM_HOST_1(kernel, buff, size, data, std::string("_"))
// #define EAGLEEYE_OPENCL_COPY_FROM_HOST(...) CONCAT(EAGLEEYE_OPENCL_COPY_FROM_HOST_, COPY_VARGS(__VA_ARGS__))(__VA_ARGS__)

// #define EAGLEEYE_OPENCL_COPY_FROM_DEVICE_1(kernel, buff, size, data,tag) \
//     clEnqueueReadBuffer(OpenCLRuntime::getOpenCLEnv()->m_command_map[tag+#kernel], kernel##_##buff, CL_TRUE, 0, size, data, 0, NULL, NULL );  
// #define EAGLEEYE_OPENCL_COPY_FROM_DEVICE_0(kernel, buff, size, data) EAGLEEYE_OPENCL_COPY_FROM_DEVICE_1(kernel, buff, size, data,std::string("_"))
// #define EAGLEEYE_OPENCL_COPY_FROM_DEVICE(...) CONCAT(EAGLEEYE_OPENCL_COPY_FROM_DEVICE_, COPY_VARGS(__VA_ARGS__))(__VA_ARGS__)

// #define RUN_VARGS_(_5, _4, _3, _2, _1, N, ...) N 
// #define RUN_VARGS(...) RUN_VARGS_(__VA_ARGS__,1,0)

// #define EAGLEEYE_OPENCL_KERNEL_RUN_1(kernel, work_dims, global_size, local_size, tag) \ 
//     size_t* kernel##_global_size = global_size;    \
//     size_t* kernel##_local_size = local_size;  \
//     size_t kernel##_work_dims = work_dims;  \
//     clEnqueueNDRangeKernel(OpenCLRuntime::getOpenCLEnv()->m_command_map[tag+#kernel], OpenCLRuntime::getOpenCLEnv()->m_kernel_map[tag+#kernel], kernel##_work_dims, NULL, kernel##_global_size, kernel##_local_size, 0, NULL, NULL); \
//     clFinish(OpenCLRuntime::getOpenCLEnv()->m_command_map[tag+#kernel]);

// #define EAGLEEYE_OPENCL_KERNEL_RUN_0(kernel, work_dims, global_size, local_size) EAGLEEYE_OPENCL_KERNEL_RUN_1(kernel, work_dims, global_size, local_size, std::string("_"))
// #define EAGLEEYE_OPENCL_KERNEL_RUN(...) CONCAT(EAGLEEYE_OPENCL_KERNEL_RUN_, RUN_VARGS(__VA_ARGS__))(__VA_ARGS__)

// #define BUFFER_ARG_VARGS_(_4, _3, _2, _1, N, ...) N 
// #define BUFFER_ARG_VARGS(...) BUFFER_ARG_VARGS_(__VA_ARGS__,1,0)

// #define EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG_1(kernel, index, buff, tag) \ 
//     clSetKernelArg(OpenCLRuntime::getOpenCLEnv()->m_kernel_map[tag+#kernel], index, sizeof(cl_mem), &kernel##_##buff);
// #define EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG_0(kernel, index, buff) EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG_1(kernel, index, buff, std::string("_"))
// #define EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(...) CONCAT(EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG_, BUFFER_ARG_VARGS(__VA_ARGS__))(__VA_ARGS__)

// #define ARG_VARGS_(_5,_4, _3, _2, _1, N, ...) N 
// #define ARG_VARGS(...) ARG_VARGS_(__VA_ARGS__,1,0)
// #define EAGLEEYE_OPENCL_KERNEL_SET_ARG_1(kernel, index, size, data, tag) \ 
//     clSetKernelArg(OpenCLRuntime::getOpenCLEnv()->m_kernel_map[tag+#kernel], index, size, &data);
// #define EAGLEEYE_OPENCL_KERNEL_SET_ARG_0(kernel, index, size, data) EAGLEEYE_OPENCL_KERNEL_SET_ARG_1(kernel, index, size, data, std::string("_"))
// #define EAGLEEYE_OPENCL_KERNEL_SET_ARG(...) CONCAT(EAGLEEYE_OPENCL_KERNEL_SET_ARG_, ARG_VARGS(__VA_ARGS__))(__VA_ARGS__)

std::string DtToCLDt(const EagleeyeType dt);
std::string DtToCLCMDDt(const EagleeyeType dt);

class OpenCLKernelGroupManager{
public:
    virtual ~OpenCLKernelGroupManager();
    static std::shared_ptr<OpenCLKernelGroupManager> getInstance();
    
    bool add(std::string name, OpenCLKernelGroup* k);
    bool release(std::string name);
    void clear();
    OpenCLKernelGroup* kgroup(std::string name);

private:
    OpenCLKernelGroupManager();
    std::map<std::string, OpenCLKernelGroup*> m_kernel_map;
    static std::shared_ptr<OpenCLKernelGroupManager> m_kernel_manager;
};
}
#else
namespace eagleeye{
class OpenCLObject{
public:
    OpenCLObject(){}
    virtual ~OpenCLObject(){};
};

// enum OpenCLMemStatus{
//     EAGLEEYE_CL_MEM_READ,
//     EAGLEEYE_CL_MEM_WRITE,
//     EAGLEEYE_CL_MEM_READ_WRITE,
//     EAGLEEYE_CL_MEM_READ_WRITE_PINNED,
// };

// class OpenCLMem{
// public:
//     OpenCLMem(OpenCLMemStatus mem_status, std::string name, unsigned int size){};
//     virtual ~OpenCLMem(){};
// };
// class OpenCLImage{
// public:
//     OpenCLImage(OpenCLMemStatus mem_status,std::string name, unsigned int rows, unsigned int cols, unsigned int channels, EagleeyeType pixel_type){};
//     virtual ~OpenCLImage(){};
// };
}
#endif    
#endif