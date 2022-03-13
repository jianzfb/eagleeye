#include "eagleeye/engine/nano/op/cast_op.h"
#if defined(__ANDROID__) || defined(ANDROID)
#include "eagleeye/engine/math/arm/type_trans.h"
#endif

namespace eagleeye{
namespace dataflow{
CastOp::CastOp(EagleeyeType data_type, float scale){
    m_data_type = data_type;
    m_scale = scale;
    OP_SUPPORT(CPU);
}    

CastOp::CastOp(const CastOp& op)
    :m_data_type(op.m_data_type),
        m_scale(op.m_scale){
    OP_SUPPORT(CPU);
}

CastOp::~CastOp(){

}

int CastOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

int CastOp::runOnCpu(std::vector<Tensor> input){
    // support float32->uint8
    // support int32->float32,uint8->float32
    Tensor x = input[0];
    if(x.type() == m_data_type){
        // 类型相同
        this->m_outputs[0] = x;
        return 0;
    }   

    // 合法性判断
    if(x.type() == EAGLEEYE_FLOAT){
        if(m_data_type != EAGLEEYE_CHAR && m_data_type != EAGLEEYE_UCHAR){
            EAGLEEYE_LOGE("Only support float->uint8 or float->int8.");
            return -1;
        }
    }
    else if(x.type() == EAGLEEYE_INT || x.type() == EAGLEEYE_CHAR || x.type() == EAGLEEYE_UCHAR){
        if(m_data_type != EAGLEEYE_FLOAT){
            EAGLEEYE_LOGE("Only support int32->float32 or uint8->float32 or int8->float32");
            return -1;
        }
    }
    else{
        EAGLEEYE_LOGE("Dont support cast type.");
        return -1;
    }

    Dim x_dim = x.dims(); 
    Dim out_dim = this->m_outputs[0].dims();
    Dim needed_out_dim = x_dim;
    if(out_dim.production() != needed_out_dim.production()){
        this->m_outputs[0] =             
            Tensor(needed_out_dim.data(),
                    m_data_type,
                    x.format(),
                    CPU_BUFFER);   
    }

    if(m_data_type == EAGLEEYE_CHAR){
        // float -> uchar
        if(x.type() == EAGLEEYE_FLOAT){
            float* din = (float*)x.cpu();
            int8_t* dout = (int8_t*)this->m_outputs[0].cpu();
            std::vector<float> scale({m_scale});
#if defined(__ANDROID__) || defined(ANDROID)            
            math::arm::fp32_to_int8(din, dout, scale.data(), 1,1,x_dim.production());
#endif
        }
    }
    else if(m_data_type == EAGLEEYE_UCHAR){
        // float -> uchar
        if(x.type() == EAGLEEYE_FLOAT){
            float* din = (float*)x.cpu();
            uint8_t* dout = (uint8_t*)this->m_outputs[0].cpu();
            std::vector<float> scale({m_scale});
#if defined(__ANDROID__) || defined(ANDROID)            
            math::arm::fp32_to_uint8(din, dout, scale.data(), 1,1,x_dim.production());
#endif
        }
    }    
    else if(m_data_type == EAGLEEYE_FLOAT){
        // uchar -> float, int -> float
        if(x.type() == EAGLEEYE_CHAR){
            int8_t* din = (int8_t*)x.cpu();
            float* dout = (float*)this->m_outputs[0].cpu();
            std::vector<float> scale({m_scale});
#if defined(__ANDROID__) || defined(ANDROID)            
            math::arm::int8_to_fp32(din, dout, scale.data(), 1, 1, x_dim.production());
#endif
        }
        else if(x.type() == EAGLEEYE_INT){
            int* din = (int*)x.cpu();
            float* dout = (float*)this->m_outputs[0].cpu();
            std::vector<float> scale({m_scale});
#if defined(__ANDROID__) || defined(ANDROID)            
            math::arm::int32_to_dtype<float>(din, dout, scale.data(), 1, 1, x_dim.production());
#endif
        }
    }
    return 0;
}

int CastOp::runOnGpu(std::vector<Tensor> input){
    return -1;
}
}    
}