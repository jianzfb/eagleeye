#include "eagleeye/processnode/TemplateMatchingNode.h"
#include "eagleeye/common/EagleeyeOpenCL.h"
#include "eagleeye/basic/MatrixMath.h"
namespace eagleeye{
TemplateMatchingNode::TemplateMatchingNode(){
    // 设置输出端口（拥有2个输出端口）
    this->setNumberOfOutputSignals(1);
    this->setOutputPort(new ImageSignal<float>, OUTPUT_PORT_SCORE);

    // 设置输入端口（拥有1个输入端口）
	this->setNumberOfInputSignals(1);

#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
    const char* source_code = "\n" \
    "#define BLOCK_SIZE 26                                                  \n" \
    "#define TEMPLATE_SIZE 32                                               \n" \
    "#define BLOCK_AND_TEMPLATE_SIZE 58                                     \n" \
    " __kernel void template_matching(__read_only image2d_t src_img,        \n" \
    "                                 __constant float4* tmpl_arr,          \n" \
    "                                 __global float* score_arr,            \n" \
    "                                 const int org_w,                      \n" \
    "                                 const int org_h,                      \n" \
    "                                 const int tmpl_w,                     \n" \
    "                                 const int tmpl_h)                     \n" \
    "{                                                                      \n" \
    "   int i = get_group_id(0);                                            \n" \
    "   int j = get_group_id(1);                                            \n" \
    "   int idX = get_local_id(0);                                          \n" \
    "   int idY = get_local_id(1);                                          \n" \
    "   int ii = i*BLOCK_SIZE + idX;                                        \n" \
    "   int jj = j*BLOCK_SIZE + idY;                                        \n" \
    "   if(ii>=org_w || ii+TEMPLATE_SIZE>=org_w || jj>=org_h || jj+TEMPLATE_SIZE>=org_h){                   \n" \
    "       return;                                                         \n" \
    "   }                                                                   \n" \
    "   __local uchar4 P[BLOCK_AND_TEMPLATE_SIZE][BLOCK_AND_TEMPLATE_SIZE]; \n" \
    "   const sampler_t smp = CLK_NORMALIZED_COORDS_FALSE|CLK_ADDRESS_CLAMP|CLK_FILTER_NEAREST;             \n" \
    "   int2 coords = {ii,jj};                                              \n" \
    "   P[idY][idX] = convert_uchar4(read_imageui(src_img, smp, coords));   \n" \
    "   if ((idX<TEMPLATE_SIZE) && (ii+BLOCK_SIZE<org_w) && (jj<org_h)){    \n" \
    "       coords.x = ii+BLOCK_SIZE;                                       \n" \
    "       coords.y = jj;                                                  \n" \
    "       P[idY][idX+BLOCK_SIZE] = convert_uchar4(read_imageui(src_img, smp, coords));                    \n" \  
    "       coords.x = ii+BLOCK_SIZE;                                       \n" \
    "       coords.y = jj+BLOCK_SIZE;                                       \n" \
    "       P[idY+BLOCK_SIZE][idX+BLOCK_SIZE] = convert_uchar4(read_imageui(src_img, smp, coords));         \n" \    
    "       if(idY+BLOCK_SIZE<TEMPLATE_SIZE){                               \n" \
    "           coords.x = ii+BLOCK_SIZE;                                   \n" \
    "           coords.y = jj+2*BLOCK_SIZE;                                 \n" \
    "           P[idY+2*BLOCK_SIZE][idX+BLOCK_SIZE] = convert_uchar4(read_imageui(src_img, smp, coords));   \n" \    
    "       }                                                               \n" \
    "   }                                                                   \n" \
    "   if ((idX+BLOCK_SIZE<TEMPLATE_SIZE) && (ii+2*BLOCK_SIZE<org_w) && (jj<org_h)){                       \n" \
    "       coords.x = ii+2*BLOCK_SIZE;                                     \n" \
    "       coords.y = jj;                                                  \n" \
    "       P[idY][idX+2*BLOCK_SIZE] = convert_uchar4(read_imageui(src_img, smp, coords));                  \n" \  
   "        coords.x = ii+2*BLOCK_SIZE;                                     \n" \
    "       coords.y = jj+BLOCK_SIZE;                                       \n" \
    "       P[idY+BLOCK_SIZE][idX+2*BLOCK_SIZE] = convert_uchar4(read_imageui(src_img, smp, coords));       \n" \
    "       if(idY+BLOCK_SIZE<TEMPLATE_SIZE){                               \n" \
    "           coords.x = ii+2*BLOCK_SIZE;                                 \n" \
    "           coords.y = jj+2*BLOCK_SIZE;                                 \n" \
    "           P[idY+2*BLOCK_SIZE][idX+2*BLOCK_SIZE] = convert_uchar4(read_imageui(src_img, smp, coords)); \n" \    
    "       }                                                               \n" \    
    "   }                                                                   \n" \
    "   if ((idY<TEMPLATE_SIZE) && (ii<org_w) &&(jj+BLOCK_SIZE<org_h)){     \n" \
    "       coords.x = ii;                                                  \n" \
    "       coords.y = jj+BLOCK_SIZE;                                       \n" \
    "       P[idY+BLOCK_SIZE][idX] = convert_uchar4(read_imageui(src_img, smp, coords));                    \n" \  
    "   }                                                                   \n" \
    "   if ((idY+BLOCK_SIZE<TEMPLATE_SIZE) && (ii<org_w) &&(jj+2*BLOCK_SIZE<org_h)){                        \n" \
    "       coords.x = ii;                                                  \n" \
    "       coords.y = jj+2*BLOCK_SIZE;                                     \n" \
    "       P[idY+2*BLOCK_SIZE][idX] = convert_uchar4(read_imageui(src_img, smp, coords));                  \n" \  
    "   }                                                                   \n" \
    "   barrier(CLK_LOCAL_MEM_FENCE);                                       \n" \
    "   float4 sum = {0,0,0,0};                                             \n" \
    "   float4 sum2 = {0,0,0,0};                                            \n" \  
    "   int loop_count = TEMPLATE_SIZE*TEMPLATE_SIZE;                       \n" \
    "   #pragma unroll                                                      \n" \
    "   for(int index = 0; index < loop_count; index++){                    \n" \
    "       int tmpl_y = index / TEMPLATE_SIZE;                             \n" \
    "       int tmpl_x = index - tmpl_y * TEMPLATE_SIZE;                    \n" \
    "       int tmpl_index = tmpl_y*TEMPLATE_SIZE+tmpl_x;                   \n" \
    "       float4 tmpl_r = tmpl_arr[tmpl_index];                           \n" \
    "       float4 org_r = convert_float4(P[idY+tmpl_y][idX+tmpl_x]);       \n" \
    "       sum = sum + tmpl_r*org_r;                                       \n" \
    "       sum2 = sum2 + org_r*org_r;                                      \n" \
    "   }                                                                   \n" \
    "   sum = sum / (sum2+0.00000001);                                      \n" \
    "   score_arr[jj*org_w+ii] = sum.x+sum.y+sum.z;                         \n" \
    "}";
    OpenCLRuntime::getOpenCLEnv()->registerProgramSource("template_matching", source_code);
    EAGLEEYE_OPENCL_KERNEL_GROUP(TM, template_matching, template_matching);
#endif
    this->m_tmpl_update = false;
    EAGLEEYE_MONITOR_VAR(int,setTMMode,getTMMode,"TMMODE","0","1");
}   

TemplateMatchingNode::~TemplateMatchingNode(){

}

void TemplateMatchingNode::executeNodeInfo(){
    ImageSignal<Array<unsigned char, 3>>* input_img_sig = dynamic_cast<ImageSignal<Array<unsigned char, 3>>*>(this->m_input_signals[0]);
    Matrix<Array<unsigned char, 3>> img = input_img_sig->getData();
    int img_rows = img.rows();
    int img_cols = img.cols();

#ifdef EAGLEEYE_OPENCL_OPTIMIZATION
    EAGLEEYE_OPENCL_CREATE_READ_IMAGE(TM, target, img_rows, img_cols, 4, EAGLEEYE_UCHAR);
    EAGLEEYE_OPENCL_CREATE_WRITE_BUFFER(TM, score, sizeof(float)*img_rows*img_cols);

    if(this->m_tmpl_update){
        cl_float4* tmpl_buff = new cl_float4[32*32];
        for(int i=0; i<32; ++i){
            unsigned char* mat_ptr = (unsigned char*)this->m_tmpl.row(i);
            for(int j=0; j<32; ++j){
                int index = i*32+j;
                tmpl_buff[index].s[0] = mat_ptr[j*3];
                tmpl_buff[index].s[1] = mat_ptr[j*3+1];
                tmpl_buff[index].s[2] = mat_ptr[j*3+2];
                tmpl_buff[index].s[3] = 0;
            }
        }
        EAGLEEYE_OPENCL_CREATE_READ_BUFFER(TM, tmpl, sizeof(cl_float4)*32*32);
        EAGLEEYE_OPENCL_COPY_TO_DEVICE(TM, tmpl, tmpl_buff);
        delete[] tmpl_buff;
    }

    EAGLEEYE_TIME_START(preprocess);
    if(m_rgba_img.rows() != img_rows || m_rgba_img.cols() != img_cols){
        m_rgba_img = Matrix<Array<unsigned char, 4>>(img_rows, img_cols);
    }

    for(int i=0; i<img_rows; ++i){
        unsigned char* img_ptr = (unsigned char*)img.row(i);
        unsigned char* rgba_img_ptr = (unsigned char*)m_rgba_img.row(i);
        for(int j=0; j<img_cols; ++j){
            int rgba_index = j*4; int rgb_index = j*3;
            rgba_img_ptr[rgba_index] = img_ptr[rgb_index];
            rgba_img_ptr[rgba_index+1] = img_ptr[rgb_index+1];
            rgba_img_ptr[rgba_index+2] = img_ptr[rgb_index+2];
            rgba_img_ptr[rgba_index+3] = 0;
        }
    }
    EAGLEEYE_TIME_END(preprocess);

    EAGLEEYE_TIME_START(copytodevice);
    EAGLEEYE_OPENCL_COPY_TO_DEVICE(TM, target, m_rgba_img.dataptr());
    EAGLEEYE_TIME_END(copytodevice);

    EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(TM, template_matching, 0, target);
    EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(TM, template_matching, 1, tmpl);
    EAGLEEYE_OPENCL_KERNEL_SET_BUFFER_ARG(TM, template_matching, 2, score);
    EAGLEEYE_OPENCL_KERNEL_SET_ARG(TM, template_matching, 3, img_cols);
    EAGLEEYE_OPENCL_KERNEL_SET_ARG(TM, template_matching, 4, img_rows);
    EAGLEEYE_OPENCL_KERNEL_SET_ARG(TM, template_matching, 5, 32);
    EAGLEEYE_OPENCL_KERNEL_SET_ARG(TM, template_matching, 6, 32);

    size_t work_dims = 2;
    size_t global_size[2];
    global_size[0] = EAGLEEYE_OPECNCL_GLOBAL_SIZE(img_cols-32+1, 26);
    global_size[1] = EAGLEEYE_OPECNCL_GLOBAL_SIZE(img_rows-32+1, 26);
    EAGLEEYE_LOGD("TM global size %d %d", global_size[0], global_size[1]);
    size_t local_size[] = {26,26};
    EAGLEEYE_LOGD("TM workgroup %d %d", global_size[0]/local_size[0], global_size[1]/local_size[1]);
    EAGLEEYE_TIME_START(RUNTM);
    EAGLEEYE_OPENCL_KERNEL_RUN(TM, template_matching, work_dims, global_size, local_size);
    EAGLEEYE_TIME_END(RUNTM);

    if(m_matching_score.rows() != img_rows || m_matching_score.cols() != img_cols){
        m_matching_score = Matrix<float>(img_rows, img_cols);
    }
    EAGLEEYE_TIME_START(copytohost);
    EAGLEEYE_OPENCL_COPY_TO_HOST(TM, score, m_matching_score.dataptr());
    EAGLEEYE_TIME_END(copytohost);

    ImageSignal<float>* score_sig = dynamic_cast<ImageSignal<float>*>(this->m_output_signals[0]);
    score_sig->setData(m_matching_score);
#endif
}

void TemplateMatchingNode::setTemplate(Matrix<Array<unsigned char,3>> mat){
    // template size must be 32 x 32
    assert(mat.rows() == 32 && mat.cols() == 32);
    this->m_tmpl = mat;
    this->m_tmpl_update = true;
}

void TemplateMatchingNode::setTMMode(int tm_mode){
    this->m_mode = tm_mode;
}
void TemplateMatchingNode::getTMMode(int& tm_mode){
    tm_mode = m_mode;
}
}