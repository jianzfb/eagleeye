#include "eagleeye/processnode/SegNode.h"
#include "eagleeye/basic/MatrixMath.h"
#include "eagleeye/common/EagleeyeTime.h"
#include "eagleeye/port/env.h"
#include "eagleeye/processnode/ImageWriteNode.h"

namespace eagleeye{
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


SegNode::SegNode(std::string model_name, 
                 std::string device, 
                 std::string input_node, 
                 std::vector<int> input_size,
                 std::string output_node,
                 std::vector<int> output_size,
                 std::vector<std::string> snpe_special_nodes,
                 SegResizeMode resized_mode,
                 bool need_softmax,
                 float seg_thres){
    // 设置输出端口（拥有1个输出端口）
    this->setNumberOfOutputSignals(1);
    // 设置输出端口(端口0)及携带数据类型(TargetT)
    this->setOutputPort(new ImageSignal<float>, OUTPUT_PORT_MASK);

    // 设置输入端口（拥有1个输入端口）
	this->setNumberOfInputSignals(1);

    // set writable path
    EAGLEEYE_MONITOR_VAR(std::string, setModelPath, getModelPath, "model","","");

    this->m_model_h = 512;
    this->m_model_w = 512;
    this->m_mean_r = 128;
    this->m_mean_g = 128;
    this->m_mean_b = 128;

    this->m_var_r = 255.0f;
    this->m_var_g = 255.0f;
    this->m_var_b = 255.0f;

    this->m_model_h = input_size[1];
    this->m_model_w = input_size[2];

    this->m_output_h = output_size[1];
    this->m_output_w = output_size[2];
    this->m_class_num = output_size[3];
    this->m_resized_mode = resized_mode;

    this->m_frame_width = -1;
    this->m_frame_height = -1;
    this->m_need_softmax = need_softmax;

    // target model
    this->m_seg_model = new ModelRun(model_name,
                                          device,
                                          std::vector<std::string>{input_node},
                                          std::vector<std::vector<int64_t>>{std::vector<int64_t>{1,m_model_h,m_model_w,3}},
                                          std::vector<std::string>{output_node},
                                          std::vector<std::vector<int64_t>>{std::vector<int64_t>{1,m_output_h,m_output_w,m_class_num}});
    
    std::map<std::string, std::string> output_name_map;
    output_name_map[output_node] = snpe_special_nodes[0];
    this->m_seg_model->setOutputNameMap(output_name_map);

    std::map<std::string, std::string> output_name_map2;
    output_name_map2[output_node] = snpe_special_nodes[1];
    this->m_seg_model->setOutputNameMap2(output_name_map2);

    this->m_input_node = input_node;
    this->m_output_node = output_node;
    m_temp_ptr = (unsigned char*)malloc(this->m_model_h*this->m_model_w*sizeof(unsigned char)*3);

    this->m_frame_width = -1;
    this->m_frame_height = -1;
    this->m_seg_thres = seg_thres;

// #ifdef EAGLEEYE_OPENCL_OPTIMIZATION
//     EAGLEEYE_OPENCL_ADD_CUSTOME_CODE(SEGNODE, preprocess_kernel_code);
//     EAGLEEYE_OPENCL_KERNEL_GROUP(segpreprocess, SEGNODE, preprocess_func);

//     EAGLEEYE_OPENCL_CREATE_READ_WRITE_PINNED_BUFFER(segpreprocess, output, sizeof(float)*this->m_model_h*this->m_model_w*3);
//     EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(segpreprocess, preprocess_func, 1, output);
// #endif    
}   

SegNode::~SegNode(){
    delete m_seg_model;
    free(m_temp_ptr);
// #ifdef EAGLEEYE_OPENCL_OPTIMIZATION
//     EAGLEEYE_OPENCL_RELEASE_KERNEL_GROUP(segpreprocess);
// #endif

} 

void SegNode::runSeg(const Matrix<Array<unsigned char,3>>& frame, int label, Matrix<float>& label_map){
    // resize to standard size
    EAGLEEYE_TIME_START(preprocess);    
    if(m_resized_mode == DEFAULT_RESIZE){
        EAGLEEYE_LOGD("use default resize mode");
        int roi_rows = frame.rows();
        int roi_cols = frame.cols();

// #ifdef EAGLEEYE_OPENCL_OPTIMIZATION
//         if(this->m_frame_width == -1 || this->m_frame_height == -1){
//             unsigned int offset_r, offset_c, layout_h, layout_w;
//             frame.layout(offset_r, offset_c, layout_h, layout_w);
//             EAGLEEYE_OPENCL_CREATE_READ_WRITE_PINNED_IMAGE(segpreprocess, input, layout_h, layout_w, 4, EAGLEEYE_FLOAT);
//             EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(segpreprocess, preprocess_func, 0, input);
//             m_frame_width = layout_h;
//             m_frame_height = layout_w;
//         }

//         float height_scale = (float)(roi_rows)/this->m_output_h;
//         float width_scale = (float)(roi_cols)/this->m_output_w;    
//         // update input
//         size_t row_pitch = 0;
//         EAGLEEYE_TIME_START(MAP);
//         float* input_ptr = (float*)EAGLEEYE_OPENCL_MAP_IMAGE(segpreprocess,input, &row_pitch);

//         for(int i=0; i<roi_rows; ++i){
//             const unsigned char* roi_ptr = (unsigned char*)frame.row(i);
//             for(int j=0; j<roi_cols; ++j){
//                 input_ptr[i*row_pitch/4+j*4] = roi_ptr[j*3];
//                 input_ptr[i*row_pitch/4+j*4+1] = roi_ptr[j*3+1];
//                 input_ptr[i*row_pitch/4+j*4+2] = roi_ptr[j*3+2];
//             }
//         }
//         EAGLEEYE_OPENCL_UNMAP(segpreprocess, input);
//         EAGLEEYE_TIME_END(MAP);

//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 2, height_scale);
//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 3, width_scale);
//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 4, roi_rows);
//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 5, roi_cols);
//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 6, m_model_h);
//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 7, m_model_w);
//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 8, 0);
//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 9, m_model_h);
//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 10, 0);
//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 11, m_model_w);
//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 12, m_mean_r);
//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 13, m_mean_g);
//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 14, m_mean_b);
//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 15, m_var_r);
//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 16, m_var_g);
//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 17, m_var_b);

//         size_t work_dims = 2;
//         size_t global_size[2] = {EAGLEEYE_OPECNCL_GLOBAL_SIZE((size_t)m_model_h,32),EAGLEEYE_OPECNCL_GLOBAL_SIZE((size_t)m_model_w,32)};
//         size_t local_size[2] = {32,32};

//         EAGLEEYE_TIME_START(RUN);
//         EAGLEEYE_OPENCL_KERNEL_RUN(segpreprocess, preprocess_func, work_dims, global_size, local_size);   
//         EAGLEEYE_TIME_END(RUN);

//         EAGLEEYE_TIME_START(UNMAP);
//         float* out_ptr = (float*)EAGLEEYE_OPENCL_MAP_BUFFER(segpreprocess, output);
//         memcpy((void*)m_model_input_f.dataptr(), out_ptr, sizeof(float)*3*m_model_h*m_model_w);
//         EAGLEEYE_OPENCL_UNMAP(segpreprocess, output);
//         EAGLEEYE_TIME_END(UNMAP);
// #else
        resize(frame,m_temp_ptr,m_model_h, m_model_w, BILINEAR_INTERPOLATION);
        Matrix<Array<unsigned char,3>> standard_img(m_model_h, m_model_w, m_temp_ptr);
        for(int r=0; r<this->m_model_h; ++r){
            float* model_input_f_ptr = (float*)m_model_input_f.row(r);
            unsigned char* standard_img_ptr = (unsigned char*)standard_img.row(r);
            for(int c=0; c<this->m_model_w; ++c){
                model_input_f_ptr[c*3] = ((float)(standard_img_ptr[c*3]) - m_mean_r)/m_var_r;
                model_input_f_ptr[c*3+1] = ((float)(standard_img_ptr[c*3+1]) - m_mean_g)/m_var_g;
                model_input_f_ptr[c*3+2] = ((float)(standard_img_ptr[c*3+2]) - m_mean_b)/m_var_b;
            }
        }
// #endif        
    }
    else{ 
        EAGLEEYE_LOGD("use same scale resize mode");
        int roi_rows = frame.rows();
        int roi_cols = frame.cols();
        int new_height = 0, new_width = 0;
        float ratio_rows_cols = 1.0f * roi_cols / roi_rows;
        if (ratio_rows_cols > 1.0f) {
            new_width = this->m_model_w;
            new_height = this->m_model_h / ratio_rows_cols;
        } else {
            new_width = this->m_model_w * ratio_rows_cols;
            new_height = this->m_model_h;
        }

        int start_r = (this->m_model_h - new_height) / 2;
        int end_r = start_r + new_height;
        int start_c = (this->m_model_w - new_width) / 2;
        int end_c = start_c + new_width;

// #ifdef EAGLEEYE_OPENCL_OPTIMIZATION
//         if(this->m_frame_width == -1 || this->m_frame_height == -1){
//             unsigned int offset_r, offset_c, layout_h, layout_w;
//             frame.layout(offset_r, offset_c, layout_h, layout_w);
//             EAGLEEYE_OPENCL_CREATE_READ_WRITE_PINNED_IMAGE(segpreprocess, input, layout_h, layout_w, 4, EAGLEEYE_FLOAT);
//             EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(segpreprocess, preprocess_func, 0, input);
//             m_frame_width = layout_h;
//             m_frame_height = layout_w;
//         }

//         float height_scale = (float)(roi_rows)/new_height;
//         float width_scale = (float)(roi_cols)/new_width;    
//         // update input
//          EAGLEEYE_TIME_START(MAP);
//         size_t row_pitch = 0;
//         float* input_ptr = (float*)EAGLEEYE_OPENCL_MAP_IMAGE(segpreprocess,input, &row_pitch);
//         for(int i=0; i<roi_rows; ++i){
//             const unsigned char* roi_ptr = (unsigned char*)frame.row(i);
//             for(int j=0; j<roi_cols; ++j){
//                 input_ptr[i*row_pitch/4+j*4] = roi_ptr[j*3];
//                 input_ptr[i*row_pitch/4+j*4+1] = roi_ptr[j*3+1];
//                 input_ptr[i*row_pitch/4+j*4+2] = roi_ptr[j*3+2];
//             }
//         }
//         EAGLEEYE_OPENCL_UNMAP(segpreprocess, input);
//         EAGLEEYE_TIME_END(MAP);

//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 2, height_scale);
//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 3, width_scale);
//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 4, roi_rows);
//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 5, roi_cols);
//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 6, m_model_h);
//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 7, m_model_w);
//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 8, start_r);
//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 9, end_r);
//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 10, start_c);
//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 11, end_c);
//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 12, m_mean_r);
//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 13, m_mean_g);
//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 14, m_mean_b);
//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 15, m_var_r);
//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 16, m_var_g);
//         EAGLEEYE_OPENCL_KERNEL_SET_ARG(segpreprocess, preprocess_func, 17, m_var_b);

//         size_t work_dims = 2;
//         size_t global_size[2] = {EAGLEEYE_OPECNCL_GLOBAL_SIZE((size_t)m_model_h,32),EAGLEEYE_OPECNCL_GLOBAL_SIZE((size_t)m_model_w,32)};
//         size_t local_size[2] = {32,32};

//         EAGLEEYE_TIME_START(RUN);
//         EAGLEEYE_OPENCL_KERNEL_RUN(segpreprocess, preprocess_func, work_dims, global_size, local_size);   
//         EAGLEEYE_TIME_END(RUN);

//         EAGLEEYE_TIME_START(UNMAP);
//         float* out_ptr = (float*)EAGLEEYE_OPENCL_MAP_BUFFER(segpreprocess, output);
//         memcpy((void*)m_model_input_f.dataptr(), out_ptr, sizeof(float)*3*m_model_h*m_model_w);
//         EAGLEEYE_OPENCL_UNMAP(segpreprocess, output);
//         EAGLEEYE_TIME_END(UNMAP);        
// #else
        EAGLEEYE_TIME_START(preprocess_resize);
        resize(frame,m_temp_ptr,new_height, new_width, BILINEAR_INTERPOLATION);
        Matrix<Array<unsigned char, 3>> tmp(new_height, new_width, this->m_temp_ptr);
        EAGLEEYE_TIME_END(preprocess_resize);

        EAGLEEYE_TIME_START(preprocess_subdiv);    
        for(int r=start_r; r<end_r; ++r){
            float* model_input_f_ptr = (float*)m_model_input_f.row(r);
            unsigned char* tmp_ptr = (unsigned char*)tmp.row(r-start_r);
            for(int c=start_c; c<end_c; ++c){
                int index = (c-start_c) * 3;
                int ii = c*3;
                model_input_f_ptr[ii] = (tmp_ptr[index] - m_mean_r)/m_var_r;
                model_input_f_ptr[ii+1] = (tmp_ptr[index + 1] - m_mean_g)/m_var_g;
                model_input_f_ptr[ii+2] = (tmp_ptr[index + 2] - m_mean_b)/m_var_b;
            }   
        }
        EAGLEEYE_TIME_END(preprocess_subdiv);
// #endif
    }
    EAGLEEYE_TIME_END(preprocess);

    // run model
    std::map<std::string, unsigned char*> inputs;
    std::map<std::string, unsigned char*> outputs;
    EAGLEEYE_TIME_START(segmodel);
    bool status = this->m_seg_model->run(inputs, outputs);
    EAGLEEYE_TIME_END(segmodel);

    float *softmax_score_ptr = (float*)outputs[this->m_output_node];
    Matrix<float> softmax_score_mat;
    if(this->m_need_softmax){
        Matrix<float> dd(this->m_model_h*this->m_model_w,m_class_num, softmax_score_ptr);
        softmax_score_mat = msoftmax(dd);
        softmax_score_ptr = softmax_score_mat.dataptr();
    }

    for (int i = 0; i < this->m_model_h; ++i) {
        float *label_map_ptr = label_map.row(i);
        for (int j = 0; j < this->m_model_w; ++j) {
            label_map_ptr[j] = softmax_score_ptr[(i * this->m_model_w + j) * m_class_num + label];
            label_map_ptr[j] = label_map_ptr[j] > this->m_seg_thres ? label_map_ptr[j] : 0;
        }
    }
}

void SegNode::setModelPath(std::string path){
    // 1.step initialize model
    this->m_seg_model->setWritablePath(path);
    this->m_seg_model->initialize();

    // 2.step get model input pointer
    m_model_input_f = Matrix<Array<float,3>>(this->m_model_h, 
                                                this->m_model_w, 
                                                this->m_seg_model->getInputPtr(this->m_input_node));
}

void SegNode::getModelPath(std::string& path){
    path = this->m_seg_model->getWritablePath();
}
}