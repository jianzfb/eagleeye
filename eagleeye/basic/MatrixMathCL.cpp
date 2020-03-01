#include "eagleeye/basic/MatrixMath.h"
#include "eagleeye/common/EagleeyeOpenCL.h"
namespace eagleeye
{
const char* softmax_2c_cl_code="\n" \    
            "__kernel void softmax_2c_cl(__global float* input, __global float* output, int count){    \n" \
            "   int i = get_global_id(0);                                                           \n" \
            "   if(i < count){                                                                      \n" \
            "       output[i*2] = 1.0f / (exp(input[i*2+1]-input[i*2]) + 1.0f);                     \n" \
            "       output[i*2+1] = 1.0f -  output[i*2];                                            \n" \
            "}}                                                                                     ";

#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
OpenCLKernelGroup* getMatrixMathCLKernel(const char* module){
    if(strcmp(module, "softmax_2c_cl") == 0){
        OpenCLEnv::getOpenCLEnv()->addCustomSource("softmax_2c_cl", softmax_2c_cl_code);
        EAGLEEYE_OPENCL_DECLARE_KERNEL_GROUP(softmax_2c);
        EAGLEEYE_OPENCL_KERNEL_GROUP(softmax_2c, softmax_2c_cl, softmax_2c_cl);
        return softmax_2c_kernels;
    }
    return NULL;
}

void freeMatrixMathCLKernel(OpenCLKernelGroup*& group){
    if(group != NULL){
        delete group;
    }
    group = NULL;
}

void softmax_2c_cl(OpenCLKernelGroup* group, const Matrix<float>& x, Matrix<float>& y){
    int rows = x.rows();
    int cols = x.cols();
    assert(cols == 2);
    const float* x_data = x.dataptr();
    float* softmax_x_data = y.dataptr();

    group->createDeviceMem("input", rows*cols*sizeof(float), EAGLEEYE_CL_MEM_READ);
    group->createDeviceMem("output", rows*cols*sizeof(float), EAGLEEYE_CL_MEM_WRITE);
    group->copyToDevice("input", (void*)x_data);
    group->setKernelArg("softmax_2c_cl", 0, std::string("input"));
    group->setKernelArg("softmax_2c_cl", 1, std::string("output"));
    group->setKernelArg("softmax_2c_cl", 2, rows);

    size_t work_dims = 1;
    size_t global_size[1];
    global_size[0] = EAGLEEYE_OPECNCL_GLOBAL_SIZE(rows, 512);
    size_t local_size[] = {512};
    group->run("softmax_2c_cl", work_dims, global_size, local_size);
    group->copyToHost("output", softmax_x_data);
}    
#endif
} // namespace eagleeye
