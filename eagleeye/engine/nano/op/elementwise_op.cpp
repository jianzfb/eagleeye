#include "eagleeye/engine/nano/op/elementwise_op.h"
#if defined(__ANDROID__) || defined(ANDROID)
#include "eagleeye/engine/math/arm/elementwise.h"
#endif
#include "eagleeye/common/EagleeyeLog.h"

namespace eagleeye{
namespace dataflow{
ElementwiseOp::ElementwiseOp(){
}
ElementwiseOp::ElementwiseOp(const ElementwiseOp& op){
}

ElementwiseOp::~ElementwiseOp(){
}

int ElementwiseOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

int ElementwiseOp::runOnCpu(std::vector<Tensor> input){
    Tensor x = input[0];
    Tensor y = input[1];
    // 合法性判断
    if(x.type() != EAGLEEYE_FLOAT || y.type() != EAGLEEYE_FLOAT){
        EAGLEEYE_LOGE("x,y type only support float.");
        return -1;
    }

    Dim x_dim = x.dims();
    Dim y_dim = y.dims();
    if(!((x_dim.size() == 2 && y_dim.size() == 2) || (x_dim.size() == 4 && y_dim.size() == 4))){
        EAGLEEYE_LOGE("x,y size not consistent.");
        return -1;
    }
    if(x_dim.size() == 2){
        if(!((x_dim[0] == y_dim[0] && x_dim[1] == y_dim[1]) || 
            ((x_dim[0] == 1 || y_dim[0] == 1) && (x_dim[1] == y_dim[1])) ||
            ((x_dim[0] == y_dim[0]) && (x_dim[1] == 1 || y_dim[1] == 1)))){
                EAGLEEYE_LOGE("x,y dim not consistent.");
                return -1;
        }
    }
    else if(x_dim.size() == 4){
        if(!((x_dim[0] == y_dim[0] && x_dim[1] == y_dim[1] && x_dim[2] == y_dim[2] && x_dim[3] == y_dim[3]) || 
            ((x_dim[1] == y_dim[1] && ((x_dim[0] == 1 && x_dim[2] == 1 && x_dim[3] == 1) || (y_dim[0] == 1 &&y_dim[2] == 1&& y_dim[3] == 1)))))){
                EAGLEEYE_LOGE("x,y dim not consistent.");
                return -1;
        }
    }

    // here, start computing
    Dim out_dim = this->m_outputs[0].dims();
    Dim needed_out_dim = x_dim | y_dim;
    if(out_dim.size() == 0 || out_dim.production() != needed_out_dim.production()){
        this->m_outputs[0] =             
            Tensor(needed_out_dim.data(),
                    x.type(),
                    x.format(),
                    CPU_BUFFER); 
    }

    if(x_dim.size() == 2){
        this->processNCOnCpu(x,y);
    }
    else{
        this->processNCHWOnCpu(x,y);
    }
    return 0;
}

int ElementwiseOp::runOnGpu(std::vector<Tensor> input){
    EAGLEEYE_LOGE("Dont implement (GPU)");
    return -1;
}

void ElementwiseOp::processNCOnCpu(Tensor x, Tensor y){
    Dim x_dim = x.dims();
    float* x_data = (float*)x.cpu();
    Dim y_dim = y.dims();
    float* y_data = (float*)y.cpu();

    Dim out_dim = this->m_outputs[0].dims();
    float* out_data = (float*)(this->m_outputs[0].cpu());

    bool is_broadcast = false;
    if(x_dim[0] == 1 || x_dim[1] == 1){
        is_broadcast = true;
        float* tmp = x_data;
        x_data = y_data;
        y_data = tmp;

        Dim tmp_dim = x_dim;
        x_dim = y_dim;
        y_dim = tmp_dim;
    }
    else if(y_dim[0] == 1|| y_dim[1] == 1){
        is_broadcast = true;
    }

    switch(this->m_op_type){
    case ELEMENTWISE_ADD:
        if(!is_broadcast){
#if defined(__ANDROID__) || defined(ANDROID)            
            math::arm::elementwise_add<float>(x_data, y_data, out_data, out_dim[0]*out_dim[1]);
#endif
        }
        else{
            if(y_dim[0] == 1){
#if defined(__ANDROID__) || defined(ANDROID)                
                math::arm::elementwise_add_broadcast_1c<float>(x_data, y_data, out_data, out_dim[0], out_dim[1]);
#endif
            }
            else{
#if defined(__ANDROID__) || defined(ANDROID)                
                math::arm::elementwise_add_broadcast_n1<float>(x_data, y_data, out_data, out_dim[0], out_dim[1]);
#endif
            }
        }
        break;
    case ELEMENTWISE_SUB:
        if(!is_broadcast){
#if defined(__ANDROID__) || defined(ANDROID)            
            math::arm::elementwise_sub<float>(x_data, y_data, out_data, out_dim[0]*out_dim[1]);
#endif
        }
        else{
            if(y_dim[0] == 1){
#if defined(__ANDROID__) || defined(ANDROID)                
                math::arm::elementwise_sub_broadcast_1c<float>(x_data, y_data, out_data, out_dim[0], out_dim[1]);
#endif
            }
            else{
#if defined(__ANDROID__) || defined(ANDROID)
                math::arm::elementwise_sub_broadcast_n1<float>(x_data, y_data, out_data, out_dim[0], out_dim[1]);
#endif
            }
        }
        break;
    case ELEMENTWISE_MUL:
        if(!is_broadcast){
#if defined(__ANDROID__) || defined(ANDROID)            
            math::arm::elementwise_mul<float>(x_data, y_data, out_data, out_dim[0]*out_dim[1]);            
#endif
        }
        else{
            if(y_dim[0] == 1){
#if defined(__ANDROID__) || defined(ANDROID)                
                math::arm::elementwise_mul_broadcast_1c<float>(x_data, y_data, out_data, out_dim[0], out_dim[1]);
#endif
            }
            else{
#if defined(__ANDROID__) || defined(ANDROID)                
                math::arm::elementwise_mul_broadcast_n1<float>(x_data, y_data, out_data, out_dim[0], out_dim[1]);
#endif
            }
        }
        break;
    case ELEMENTWISE_DIV:
        if(!is_broadcast){
#if defined(__ANDROID__) || defined(ANDROID)            
            math::arm::elementwise_div<float>(x_data, y_data, out_data, out_dim[0]*out_dim[1]);
#endif
        }
        else{
            if(y_dim[0] == 1){
#if defined(__ANDROID__) || defined(ANDROID)                
                math::arm::elementwise_div_broadcast_1c<float>(x_data, y_data, out_data, out_dim[0], out_dim[1]);
#endif
            }
            else{
#if defined(__ANDROID__) || defined(ANDROID)                
                math::arm::elementwise_div_broadcast_n1<float>(x_data, y_data, out_data, out_dim[0], out_dim[1]);
#endif
            }            
        }        
        break;
    case ELEMENTWISE_POW:
         if(!is_broadcast){
#if defined(__ANDROID__) || defined(ANDROID)             
            math::arm::elementwise_pow<float>(x_data, y_data, out_data, out_dim[0]*out_dim[1]);
#endif
        }
        else{
            if(y_dim[0] == 1){
#if defined(__ANDROID__) || defined(ANDROID)                
                math::arm::elementwise_pow_broadcast_1c<float>(x_data, y_data, out_data, out_dim[0], out_dim[1]);
#endif
            }
            else{
#if defined(__ANDROID__) || defined(ANDROID)                
                math::arm::elementwise_pow_broadcast_n1<float>(x_data, y_data, out_data, out_dim[0], out_dim[1]);
#endif
            }                
        }    
        break;
    default:
        EAGLEEYE_LOGE("Dont support Elementwise Type");
        break;
    }    
}
void ElementwiseOp::processNCHWOnCpu(Tensor x, Tensor y){
    Dim x_dim = x.dims();
    float* x_data = (float*)x.cpu();
    Dim y_dim = y.dims();
    float* y_data = (float*)y.cpu();

    Dim out_dim = this->m_outputs[0].dims();
    float* out_data = (float*)(this->m_outputs[0].cpu());

    bool is_broadcast = false;
    if(x_dim[0] == 1 && x_dim[2] == 1 && x_dim[3] == 1){
        is_broadcast = true;
        float* tmp = x_data;
        x_data = y_data;
        y_data = tmp;
    }
    else if(y_dim[0] == 1 && y_dim[2] == 1 && y_dim[3] == 1){
        is_broadcast = true;
    }

    switch (this->m_op_type){
    case ELEMENTWISE_ADD:
        if(!is_broadcast){
#if defined(__ANDROID__) || defined(ANDROID)            
            math::arm::elementwise_add<float>(x_data, y_data, out_data, out_dim[0]*out_dim[1]*out_dim[2]*out_dim[3]);
#endif
        }
        else{
#if defined(__ANDROID__) || defined(ANDROID)            
            math::arm::elementwise_add_broadcast<float>(x_data, y_data, out_data, out_dim[0], out_dim[1], out_dim[2]*out_dim[3]);
#endif
        }
        break;
    case ELEMENTWISE_SUB:
        if(!is_broadcast){
#if defined(__ANDROID__) || defined(ANDROID)            
            math::arm::elementwise_sub<float>(x_data, y_data, out_data, out_dim[0]*out_dim[1]*out_dim[2]*out_dim[3]);
#endif
        }
        else{
#if defined(__ANDROID__) || defined(ANDROID)
            math::arm::elementwise_sub_broadcast<float>(x_data, y_data, out_data, out_dim[0], out_dim[1], out_dim[2]*out_dim[3]);
#endif
        }
        break;
    case ELEMENTWISE_MUL:
        if(!is_broadcast){
#if defined(__ANDROID__) || defined(ANDROID)            
            math::arm::elementwise_mul<float>(x_data, y_data, out_data, out_dim[0]*out_dim[1]*out_dim[2]*out_dim[3]);
#endif
        }
        else{
#if defined(__ANDROID__) || defined(ANDROID)            
            math::arm::elementwise_mul_broadcast<float>(x_data, y_data, out_data, out_dim[0], out_dim[1], out_dim[2]*out_dim[3]);
#endif
        }
        break;
    case ELEMENTWISE_DIV:
        if(!is_broadcast){
#if defined(__ANDROID__) || defined(ANDROID)            
            math::arm::elementwise_div<float>(x_data, y_data, out_data, out_dim[0]*out_dim[1]*out_dim[2]*out_dim[3]);
#endif
        }
        else{
#if defined(__ANDROID__) || defined(ANDROID)            
            math::arm::elementwise_div_broadcast<float>(x_data, y_data, out_data, out_dim[0], out_dim[1], out_dim[2]*out_dim[3]);
#endif
        }
        break;
    case ELEMENTWISE_POW:
         if(!is_broadcast){
#if defined(__ANDROID__) || defined(ANDROID)             
            math::arm::elementwise_pow<float>(x_data, y_data, out_data, out_dim[0]*out_dim[1]*out_dim[2]*out_dim[3]);
#endif
        }
        else{
#if defined(__ANDROID__) || defined(ANDROID)
            math::arm::elementwise_pow_broadcast<float>(x_data, y_data, out_data, out_dim[0], out_dim[1], out_dim[2]*out_dim[3]);
#endif
        }
        break;
    default:
        EAGLEEYE_LOGE("Dont support Elementwise Type");
        break;
    }
}
}
}
