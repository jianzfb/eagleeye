#include "eagleeye/engine/nano/op/pad2d_op.h"
#if defined (__ARM_NEON) || defined (__ARM_NEON__)  
#include "eagleeye/engine/math/arm/pad2d.h"
#endif
namespace eagleeye{
namespace dataflow{
Pad2dOp::Pad2dOp(Pad2dOpType pad_type, std::vector<int64_t> pad_c, std::vector<int64_t> pad_h, std::vector<int64_t> pad_w, float pad_value){
    m_pad_value = pad_value;
    m_pad_type = pad_type;
    m_pad_h = pad_h;
    m_pad_w = pad_w;
    m_pad_c = pad_c;
    OP_SUPPORT(CPU);
}   
Pad2dOp::Pad2dOp(const Pad2dOp& op)
    :m_pad_value(op.m_pad_value),
        m_pad_type(op.m_pad_type),
        m_pad_h(op.m_pad_h),
        m_pad_w(op.m_pad_w),
        m_pad_c(op.m_pad_c){
    OP_SUPPORT(CPU);
}

Pad2dOp::~Pad2dOp(){

} 

int Pad2dOp::init(std::map<std::string, std::vector<float>> params){
    if(m_pad_c.size() > 0 && m_pad_c[0] > 0 && m_pad_c[1] > 0){
        EAGLEEYE_LOGE("Ignore padding c.");
    }
    return 0;
}

int Pad2dOp::runOnCpu(const std::vector<Tensor>& input){
    const Tensor x = input[0];
    // 合法性判断
    if(x.type() != EAGLEEYE_FLOAT){
        EAGLEEYE_LOGE("x type only support float.");
        return -1;
    }

    Dim x_dim = x.dims(); 
    Dim out_dim = this->m_outputs[0].dims();
    std::vector<int64_t> needed_shape = x_dim.data();
    int pad_top = 0; int pad_bottom = 0;
    int pad_left = 0; int pad_right = 0;
    if(this->m_pad_h.size() > 0){
        pad_top = this->m_pad_h[0];
        pad_bottom = this->m_pad_h[1];
        needed_shape[2] += (pad_top + pad_bottom);
    }
    if(this->m_pad_w.size() > 0){
        pad_left = this->m_pad_w[0];
        pad_right = this->m_pad_w[1];
        needed_shape[3] += (pad_left + pad_right);
    }

    Dim needed_out_dim(needed_shape);
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

    // nchw
    int on = out_dim[0];
    int oc = out_dim[1];
    int oh = out_dim[2];
    int ow = out_dim[3];
#if defined (__ARM_NEON) || defined (__ARM_NEON__)  
  if (m_pad_type == PAD2D_CONSTANT) {
    math::arm::pad_constant(din,
                 dout,
                 on,
                 oc,
                 oh,
                 ow,
                 m_pad_h[0],
                 m_pad_h[1],
                 m_pad_w[0],
                 m_pad_w[1],
                 m_pad_value);
  } else if (m_pad_type == PAD2D_REFLECT) {
    math::arm::pad_reflect(din,
                dout,
                on,
                oc,
                oh,
                ow,
                m_pad_h[0],
                m_pad_h[1],
                m_pad_w[0],
                m_pad_w[1],
                m_pad_value);
  } else if (m_pad_type == PAD2D_EDGE) {
    math::arm::pad_edge(din,
             dout,
             on,
             oc,
             oh,
             ow,
             m_pad_h[0],
             m_pad_h[1],
             m_pad_w[0],
             m_pad_w[1],
             m_pad_value);
  } else {
    EAGLEEYE_LOGE("Unkown pad mode.");
  }

#endif
    return 0;
}

int Pad2dOp::runOnGpu(const std::vector<Tensor>& input){
    EAGLEEYE_LOGE("Dont implement (GPU)");
    return -1;
}
}    
}