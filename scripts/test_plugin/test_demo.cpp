#include "test_plugin.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/test/nano/FixedResizedOpTest.h"
#include <iostream>
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/util/conv_pool_2d_util.h"
#include "eagleeye/port/env.h"
#include "eagleeye/common/EagleeyeThreadPool.h"
#include "eagleeye/common/EagleeyeOpenCL.h"
#include "eagleeye/basic/MatrixMath.h"
#include <opencv2/core/core.hpp>
#include <opencv2/video/tracking.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <functional>
#include <list>
using namespace eagleeye;

const char* preprocess_kernel_code = "\n" \
    "__kernel void preprocess_func(__read_only image2d_t input,       \n" \
    "                               __global float* output,             \n" \
    "                               const float height_scale,           \n" \
    "                               const float width_scale,            \n" \
    "                               const int in_height,                \n" \
    "                               const int in_width,                 \n" \
    "                               const int out_height,               \n" \
    "                               const int out_width,                \n" \
    "                               const int start_r,                  \n" \
    "                               const int end_r,                    \n" \
    "                               const int start_c,                  \n" \
    "                               const int end_c,                    \n" \
    "                               const float r_mean,                 \n" \
    "                               const float g_mean,                 \n" \
    "                               const float b_mean,                 \n" \
    "                               const float r_var,                  \n" \
    "                               const float g_var,                  \n" \
    "                               const float b_var){                 \n" \
    "   int y = get_global_id(0);                                           \n" \
    "   int x = get_global_id(1);                                           \n" \
    "   if(y >= out_height || x >= out_width){                              \n" \
    "       return;                                                         \n" \
    "   }                                                                   \n" \
    "   if(y < start_r || y >= end_r || x < start_c || x >= end_c){         \n" \
    "       output[y*out_width*3+x*3] = 0;                                  \n" \
    "       output[y*out_width*3+x*3+1] = 0;                                \n" \
    "       output[y*out_width*3+x*3+2] = 0;                                \n" \
    "       return;                                                         \n" \
    "   }                                                                   \n" \  
    "   const float h_in = (y-start_r) * height_scale;                      \n" \
    "   const float w_in = (x-start_c) * width_scale;                       \n" \
    "   const int h_lower = max(0, (int) floor(h_in));                      \n" \
    "   const int h_upper = min(in_height - 1, h_lower + 1);                \n" \
    "   const int w_lower = max(0, (int) floor(w_in));                      \n" \
    "   const int w_upper = min(in_width - 1, w_lower + 1);                 \n" \
    "   const float h_lerp = h_in - h_lower;                                \n" \
    "   const float w_lerp = w_in - w_lower;                                \n" \
    "   const sampler_t smp = CLK_NORMALIZED_COORDS_FALSE|CLK_ADDRESS_CLAMP_TO_EDGE|CLK_FILTER_NEAREST;  \n" \
    "   float4 top_left = read_imagef(input, smp, (int2)(w_lower, h_lower));       \n" \
    "   float4 top_right = read_imagef(input, smp,(int2)(w_upper, h_lower));       \n" \
    "   float4 bottom_left = read_imagef(input, smp,(int2)(w_lower, h_upper));     \n" \
    "   float4 bottom_right = read_imagef(input, smp, (int2)(w_upper, h_upper));   \n" \
    "   float4 top = mad((top_right - top_left), w_lerp, top_left);                 \n" \
    "   float4 bottom = mad((bottom_right - bottom_left), w_lerp, bottom_left);     \n" \
    "   float4 out = mad((bottom - top), h_lerp, top);                              \n" \
    "   output[y*out_width*3+x*3] = (out.x - r_mean)/r_var;                         \n" \
    "   output[y*out_width*3+x*3+1] = (out.y - g_mean)/g_var;                       \n" \
    "   output[y*out_width*3+x*3+2] = (out.z - b_mean)/b_var;                       \n" \
    "}";   

#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
EAGLEEYE_OPENCL_DECLARE_KERNEL_GROUP(Test);
#endif

int main(int argc, char** argv){

// #ifdef EAGLEEYE_OPENCL_OPTIMIZATION
//     EAGLEEYE_OPENCL_ADD_CUSTOME_CODE(BB, preprocess_kernel_code);
//     EAGLEEYE_OPENCL_KERNEL_GROUP(Test, BB, preprocess_func);
    
//     cv::Mat img = cv::imread("/data/local/tmp/bb.jpeg");
//     int origin_rows = img.rows;
//     int origin_cols = img.cols;

//     // int rows = 384;
//     // int cols = 384;
//     img = img(cv::Range(730,2310),cv::Range(432,1389));
//     img = img.clone();
//     int rows = img.rows;
//     int cols = img.cols;

//     std::cout<<"A"<<std::endl;
//     EAGLEEYE_OPENCL_CREATE_READ_WRITE_PINNED_IMAGE(Test, input, origin_rows, origin_cols, 4, EAGLEEYE_FLOAT);
//     size_t row_pitch = 0;
//     float* input_ptr = (float*)EAGLEEYE_OPENCL_MAP_IMAGE(Test,input, &row_pitch);
//     std::cout<<"row pitch"<<std::endl;
//     std::cout<<row_pitch<<std::endl;
//     if(input_ptr == NULL){
//         std::cout<<"error"<<std::endl;
//     }

//     unsigned char* img_data = (unsigned char*)img.data;
//     for(int i=0; i<rows; ++i){
//         for(int j=0; j<cols; ++j){
//             unsigned char r = img_data[i*cols*3+j*3];
//             unsigned char g = img_data[i*cols*3+j*3+1];
//             unsigned char b = img_data[i*cols*3+j*3+2];
//             input_ptr[i*row_pitch/4+j*4] = r;
//             input_ptr[i*row_pitch/4+j*4+1] = g;
//             input_ptr[i*row_pitch/4+j*4+2] = b;
//         }
//     }
//     EAGLEEYE_OPENCL_UNMAP(Test,input);
    
//     EAGLEEYE_OPENCL_CREATE_READ_WRITE_PINNED_BUFFER(Test, output, sizeof(float)*384*384*3);

//     int new_height = 0, new_width = 0;
//     float ratio_rows_cols = 1.0f * cols / rows;
//     if (ratio_rows_cols > 1.0f) {
//         new_width = 384;
//         new_height = 384 / ratio_rows_cols;
//     } else {
//         new_width = 384 * ratio_rows_cols;
//         new_height = 384;
//     }

//     int start_r = (384 - new_height) / 2;
//     int end_r = start_r + new_height;
//     int start_c = (384 - new_width) / 2;
//     int end_c = start_c + new_width;

//     float height_scales = (float)(rows)/new_height;
//     float width_scales = (float)(cols)/new_width;
//     EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(Test, preprocess_func, 0, input);
//     EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(Test, preprocess_func, 1, output);
//     EAGLEEYE_OPENCL_KERNEL_SET_ARG(Test, preprocess_func, 2, height_scales);
//     EAGLEEYE_OPENCL_KERNEL_SET_ARG(Test, preprocess_func, 3, width_scales);
//     EAGLEEYE_OPENCL_KERNEL_SET_ARG(Test, preprocess_func, 4, rows);
//     EAGLEEYE_OPENCL_KERNEL_SET_ARG(Test, preprocess_func, 5, cols);
//     EAGLEEYE_OPENCL_KERNEL_SET_ARG(Test, preprocess_func, 6, 384);
//     EAGLEEYE_OPENCL_KERNEL_SET_ARG(Test, preprocess_func, 7, 384);
//     EAGLEEYE_OPENCL_KERNEL_SET_ARG(Test, preprocess_func, 8, start_r);
//     EAGLEEYE_OPENCL_KERNEL_SET_ARG(Test, preprocess_func, 9, end_r);
//     EAGLEEYE_OPENCL_KERNEL_SET_ARG(Test, preprocess_func, 10, start_c);
//     EAGLEEYE_OPENCL_KERNEL_SET_ARG(Test, preprocess_func, 11, end_c);
//     EAGLEEYE_OPENCL_KERNEL_SET_ARG(Test, preprocess_func, 12, 0.0f);
//     EAGLEEYE_OPENCL_KERNEL_SET_ARG(Test, preprocess_func, 13, 0.0f);
//     EAGLEEYE_OPENCL_KERNEL_SET_ARG(Test, preprocess_func, 14, 0.0f);
//     EAGLEEYE_OPENCL_KERNEL_SET_ARG(Test, preprocess_func, 15, 1.0f);
//     EAGLEEYE_OPENCL_KERNEL_SET_ARG(Test, preprocess_func, 16, 1.0f);
//     EAGLEEYE_OPENCL_KERNEL_SET_ARG(Test, preprocess_func, 17, 1.0f);
    
//     size_t work_dims = 2;
//     size_t global_size[2] = {EAGLEEYE_OPECNCL_GLOBAL_SIZE(384,32),EAGLEEYE_OPECNCL_GLOBAL_SIZE(384,32)};
//     size_t local_size[2] = {32,32};
//     for(int i=0; i<10; ++i){
//         EAGLEEYE_TIME_START(RUN);
//         EAGLEEYE_OPENCL_KERNEL_RUN(Test, preprocess_func, work_dims, global_size, local_size);   
//         EAGLEEYE_TIME_END(RUN);
//     }

//     EAGLEEYE_TIME_START(UNMAP);
//     float* out_ptr = (float*)EAGLEEYE_OPENCL_MAP_BUFFER(Test, output);
//     Matrix<Array<unsigned char,3>> out_img(384,384);
//     for(int i=0; i<384; ++i){
//         for(int j=0; j<384; ++j){
//             out_img.at(i,j)[0] = out_ptr[i*384*3+j*3];
//             out_img.at(i,j)[1] = out_ptr[i*384*3+j*3+1];
//             out_img.at(i,j)[2] = out_ptr[i*384*3+j*3+2];
//         }
//     }
//     cv::Mat ss(384,384,CV_8UC3, out_img.dataptr());
//     cv::imwrite("./out.png", ss);
//     EAGLEEYE_OPENCL_UNMAP(Test,output);
//     EAGLEEYE_TIME_END(UNMAP)


    // EAGLEEYE_OPENCL_CREATE_READ_WRITE_BUFFER(Test, A, sizeof(float)*384*384*4);
    // EAGLEEYE_OPENCL_CREATE_READ_WRITE_BUFFER(Test, B, sizeof(float)*384*384*4);
    // EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(Test, softmax_2c_cl, 0, A);
    // EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(Test, softmax_2c_cl, 1, B);
    // EAGLEEYE_OPENCL_KERNEL_SET_ARG(Test, softmax_2c_cl, 2, 384*384);
    // for(int i=0; i<10; ++i){
    //     EAGLEEYE_TIME_START(SOFTMAX);
    //     EAGLEEYE_OPENCL_KERNEL_RUN(Test, softmax_2c_cl, work_dims, global_size, local_size);   
    //     EAGLEEYE_TIME_END(SOFTMAX);
    // }

// #endif
    // 1.step initialize test module
    const char* config_folder = NULL;   // test module configure folder
    eagleeye_test_initialize(config_folder);
    
    // 2.step set module pipeline parameter
    // char* node_name = "data_source";     // NODE NAME in pipeline
    // char* param_name = "";    // PARAMETER NAME of NODE in pipeline
    // void* value = NULL;       // PARAMETER VALUE
    // bool is_ok = eagleeye_test_set_param(node_name, param_name, value);
    // if(is_ok){
    //     EAGLEEYE_LOGD("success to set parameter %s of node %s",param_name,node_name);
    // }
    // else{
    //     EAGLEEYE_LOGE("fail to set parameter %s of node %s",param_name,node_name);
    // }

    // 3.step set input data 
    Matrix<float> aa(10,10);
    aa = 1.0f;
    void* data = aa.dataptr(); // YOUR IMAGE DATA POINTER;
    int data_size[] = {10,10,1}; // {IMAGE HEIGHT, IMAGE WIDTH, IMAGE CHANNEL};
    int data_type = 6;  // float 
    bool is_ok = eagleeye_test_set_input("data_source", data, data_size, 3, data_type);
    if(is_ok){
        EAGLEEYE_LOGD("success to set data for pipeline input node");
    }
    else{
        EAGLEEYE_LOGE("fail to set data for pipeline input node");
    }

    // 4.step refresh module pipeline
    eagleeye_test_run();

    is_ok = eagleeye_test_set_input("data_source", data, data_size, 3, data_type);
    eagleeye_test_run();


    // 5.step get output data of test module
    void* out_data;         // RESULT DATA POINTER
    int out_data_size[3];   // RESULT DATA SHAPE (IMAGE HEIGHT, IMAGE WIDTH, IMAGE CHANNEL)
    int out_data_dims=0;    // 3
    int out_data_type=0;    // RESULT DATA TYPE 
    is_ok = eagleeye_test_get_output("b/0",out_data, out_data_size, out_data_dims, out_data_type);   
    if(is_ok){
        EAGLEEYE_LOGD("success to get data for pipeline output node");
    }
    else{
        EAGLEEYE_LOGE("fail to set data for pipeline output node");
    }
    Matrix<float> bb(out_data_size[0], out_data_size[1], out_data);
    std::cout<<bb;

    // 6.step (optional) sometimes, could call this function to reset all intermedianl state in pipeline
    is_ok = eagleeye_test_reset();
    if(is_ok){
        EAGLEEYE_LOGD("success to reset pipeline");
    }
    else{
        EAGLEEYE_LOGE("fail to reset pipeline");
    }

    // 7.step release test module
    eagleeye_test_release();

    return 1;
}