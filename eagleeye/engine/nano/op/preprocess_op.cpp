#include "eagleeye/engine/nano/op/preprocess_op.h"
#include <stdint.h>
#if defined(__ANDROID__) || defined(ANDROID)  
#include "eagleeye/engine/math/arm/preprocess.h"
#endif
namespace eagleeye{
namespace dataflow{
PreprocessOp::PreprocessOp(std::vector<float> mean_v, std::vector<float> scale_v, bool reverse_channel)
    :m_mean_v(mean_v),
        m_scale_v(scale_v),
        m_reverse_channel(reverse_channel){

}

PreprocessOp::PreprocessOp(const PreprocessOp& op)
    :m_mean_v(op.m_mean_v),
        m_scale_v(op.m_scale_v),
        m_reverse_channel(op.m_reverse_channel){

}

PreprocessOp::~PreprocessOp(){

}

int PreprocessOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}


// void PreprocessOp::bgrToTensorCHW(const uint8_t* src,
//                        float* output,
//                        int width,
//                        int height,
//                        float* means,
//                        float* scales) {
//   math::arm::bgrToTensorCHW(src, output, width, height, means, scales);
// }

// void PreprocessOp::bgrToRgbTensorCHW(const uint8_t* src,
//                        float* output,
//                        int width,
//                        int height,
//                        float* means,
//                        float* scales) {
//   math::arm::bgrToRgbTensorCHW(src, output, width, height, means, scales);
// }

int PreprocessOp::runOnCpu(const std::vector<Tensor>& input){
    const Tensor x = input[0];
    Dim x_dim = x.dims();
        
    if(x.type() != EAGLEEYE_UCHAR){
        EAGLEEYE_LOGE("PreprocessOp only support EAGLEEYE_UCHAR.");
        return -1;
    }

    if(x.format() != DataFormat::NHWC){
        EAGLEEYE_LOGE("PreprocessOp only support NHWC.");
        return -1;
    }

    if(x_dim[3] != 3){
        EAGLEEYE_LOGE("PreprocessOp only support channel=3.");
        return -1;
    }

    if(this->m_outputs[0].numel() != input[0].numel()){
        // 分配空间
        std::vector<int64_t> shape(4);
        shape[0] = x_dim[0];
        shape[1] = x_dim[3];
        shape[2] = x_dim[1];
        shape[3] = x_dim[2];
        this->m_outputs[0] =          
                Tensor(shape,
                        EAGLEEYE_FLOAT,
                        DataFormat::NCHW,
                        CPU_BUFFER); 
    }

    uint8_t* src_ptr = (uint8_t*)(x.cpu());
    float* output_ptr = (float*)(this->m_outputs[0].cpu());
    if(this->m_reverse_channel){
#if defined(__ANDROID__) || defined(ANDROID)        
        math::arm::bgrToRgbTensorCHW(
            src_ptr,
            output_ptr,
            x_dim[2],
            x_dim[1],
            (float*)(m_mean_v.data()),
            (float*)(m_scale_v.data())
        );
#endif
    }
    else{
#if defined(__ANDROID__) || defined(ANDROID)        
        math::arm::bgrToTensorCHW(
            src_ptr,
            output_ptr,
            x_dim[2],
            x_dim[1],
            (float*)(m_mean_v.data()),
            (float*)(m_scale_v.data())
        );
#endif
    }
    return 0;
}

int PreprocessOp::runOnGpu(const std::vector<Tensor>& input){
    return 0;
}
} // namespace dataflow
    
} // namespace eagleeye
