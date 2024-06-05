#include "eagleeye/engine/nano/op/clip_op.h"
#if defined (__ARM_NEON) || defined (__ARM_NEON__)
#include "eagleeye/engine/math/arm/clip.h"
#endif
namespace eagleeye{
namespace dataflow{
ClipOp::ClipOp(float min_v, float max_v){
    this->m_min_v = min_v;
    this->m_max_v = max_v;    
}    

ClipOp::ClipOp(const ClipOp& op)
    :m_min_v(op.m_min_v),
        m_max_v(op.m_max_v){

}

ClipOp::~ClipOp(){

}

int ClipOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

int ClipOp::runOnCpu(const std::vector<Tensor>& input){
    const Tensor x = input[0];
    // 合法性判断
    if(x.type() != EAGLEEYE_FLOAT){
        EAGLEEYE_LOGE("x type only support float.");
        return -1;
    }

    Dim x_dim = x.dims(); 
    if(x_dim.size() == 0 || x_dim[0] == 0){
        return -1;
    }

    Dim out_dim = this->m_outputs[0].dims();
    Dim needed_out_dim = x_dim;
    if(out_dim.size() == 0 || out_dim.production() != needed_out_dim.production()){
        this->m_outputs[0] =             
            Tensor(needed_out_dim.data(),
                    x.type(),
                    x.format(),
                    CPU_BUFFER);            
    }

    float* din = (float*)x.cpu();
    float* dout = (float*)this->m_outputs[0].cpu();
    out_dim = this->m_outputs[0].dims();

#if defined (__ARM_NEON) || defined (__ARM_NEON__)
    math::arm::clip_fp32(din, out_dim.production(), m_min_v, m_max_v, dout);
#endif
    return 0;
}

int ClipOp::runOnGpu(const std::vector<Tensor>& input){
    return 0;
}
}    
}