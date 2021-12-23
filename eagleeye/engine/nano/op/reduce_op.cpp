#include "eagleeye/engine/nano/op/reduce_op.h"
#include "eagleeye/engine/math/arm/reduce_max.h"
#include "eagleeye/engine/math/arm/reduce_min.h"
#include "eagleeye/engine/math/arm/reduce_mean.h"
#include "eagleeye/engine/math/arm/reduce_sum.h"
#include "eagleeye/engine/math/arm/reduce_prod.h"

namespace eagleeye{
namespace dataflow{
ReduceOp::ReduceOp(ReduceOpType op_type, std::vector<int64_t> axis, bool keep_axis)
    :m_op_type(op_type), m_axis(axis), m_keep_axis(keep_axis){
    m_reduce_n = false;
    m_reduce_c = false;
    m_reduce_h = false;
    m_reduce_w = false;
    m_reduce_nchw = false;
    m_reduce_hw = false;
    m_reduce_ch = false;
    m_reduce_nc = false;
    OP_SUPPORT(CPU);
}
ReduceOp::ReduceOp(const ReduceOp& op)
    :m_op_type(op.m_op_type),
        m_axis(op.m_axis),
        m_keep_axis(op.m_keep_axis){
    m_reduce_n = false;
    m_reduce_c = false;
    m_reduce_h = false;
    m_reduce_w = false;
    m_reduce_nchw = false;
    m_reduce_hw = false;
    m_reduce_ch = false;
    m_reduce_nc = false;
    OP_SUPPORT(CPU);
}

ReduceOp::~ReduceOp(){

}

int ReduceOp::init(std::map<std::string, std::vector<float>> params){
    if(m_axis.size() == 1){
        int axis_0 = m_axis[0];
        if(axis_0 == 0){
             m_reduce_n = true;
        }
        else if(axis_0 == 1){
            m_reduce_c = true;
        }
        else if(axis_0 == 2){
            m_reduce_h = true; 
        }
        else if(axis_0 == 3){
            m_reduce_w = true;
        }
        else{
            EAGLEEYE_LOGD("Dont support reduce axis.");
            return -1;
        }
    }
    else if(m_axis.size() == 2){
        int axis_0 = (int)m_axis[0];
        int axis_1 = (int)m_axis[1];
        if(axis_0 == 2 && axis_1 == 3){
            m_reduce_hw = true;
        }
        else if(axis_0 == 1 && axis_1 == 2){
            m_reduce_ch = true;           
        }
        else if(axis_0 == 0 && axis_1 == 1){
            m_reduce_nc = true;               
        }
        else{
            EAGLEEYE_LOGD("Dont support reduce axis.");
            return -1;
        }
    }
    else if(m_axis.size() == 4){
        m_reduce_nchw = true;           
    }
    else{
        EAGLEEYE_LOGD("Dont support reduce axis.");
        return -1;
    }  
    return 0;
}

int ReduceOp::runOnCpu(std::vector<Tensor> input){
    Tensor x = input[0];
    // 合法性判断
    if(x.type() != EAGLEEYE_FLOAT){
        EAGLEEYE_LOGE("x type only support float.");
        return -1;
    }
    Dim x_dim = x.dims();
    if(!(x_dim.size() == 2 || x_dim.size() == 4)){
        EAGLEEYE_LOGE("x dim must be 2 or 4.");
        return -1;
    }

    // here, start computing
    Dim out_dim = this->m_outputs[0].dims();
    std::vector<int64_t> reduced_x_shape(x_dim.size());
    for(int i=0; i<x_dim.size(); ++i){
        reduced_x_shape[i] = x_dim[i];
    }
    for(int i=0; i<m_axis.size(); ++i){
        reduced_x_shape[m_axis[i]] = 1;
    }
    Dim needed_out_dim(reduced_x_shape);
    if(out_dim.size() == 0 || out_dim.production() != needed_out_dim.production()){
        if(!m_keep_axis){
            std::vector<int64_t> no_keep_reduced_x_shape;
            for(int i=0; i<reduced_x_shape.size(); ++i){
                bool finding = false;
                for(int j=0; j<m_axis.size(); ++j){
                    if(i == m_axis[j]){
                        finding = true;
                    }
                }
                if(!finding){
                    no_keep_reduced_x_shape.push_back(reduced_x_shape[i]);
                }
            }

            needed_out_dim = Dim(no_keep_reduced_x_shape);
        }

        this->m_outputs[0] =             
            Tensor(needed_out_dim.data(),
                    x.type(),
                    DataFormat::AUTO,
                    CPU_BUFFER); 
    }
    float* x_data = (float*)x.cpu();
    float* out_data = (float*)this->m_outputs[0].cpu();

    int x_b = x_dim[0];
    int x_c = x_dim[1];
    int x_h,x_w;
    if(x_dim.size() == 2){
        x_h = 1; x_w = 1;
    }
    else{
        x_h = x_dim[2]; x_w = x_dim[3];
    }

    if(this->m_reduce_n){
        switch (this->m_op_type){
        case REDUCE_MIN:
            math::arm::reduce_min_n<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        case REDUCE_MAX:
            math::arm::reduce_n<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        case REDUCE_MEAN:
            math::arm::reduce_mean_n<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        case REDUCE_SUM:
            math::arm::reduce_sum_n<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        case REDUCE_PROD:
            math::arm::reduce_prod_n<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        default:
            break;
        }
    }
    else if(this->m_reduce_c){
        switch (this->m_op_type){
        case REDUCE_MIN:
            math::arm::reduce_min_c<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        case REDUCE_MAX:
            math::arm::reduce_c<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        case REDUCE_MEAN:
            math::arm::reduce_mean_c<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        case REDUCE_SUM:
            math::arm::reduce_sum_c<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        case REDUCE_PROD:
            math::arm::reduce_prod_c<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        default:
            break;
        }
    }
    else if(this->m_reduce_h){
        switch (this->m_op_type){
        case REDUCE_MIN:
            math::arm::reduce_min_h<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        case REDUCE_MAX:
            math::arm::reduce_h<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        case REDUCE_MEAN:
            math::arm::reduce_mean_h<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        case REDUCE_SUM:
            math::arm::reduce_sum_h<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        case REDUCE_PROD:
            math::arm::reduce_prod_h<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        default:
            break;
        }
    }
    else if(this->m_reduce_w){
        switch (this->m_op_type){
        case REDUCE_MIN:
            math::arm::reduce_min_w<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        case REDUCE_MAX:
            math::arm::reduce_w<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        case REDUCE_MEAN:
            math::arm::reduce_mean_w<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        case REDUCE_SUM:
            math::arm::reduce_sum_w<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        case REDUCE_PROD:
            math::arm::reduce_prod_w<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        default:
            break;
        }
    }
    else if(this->m_reduce_nchw){
        switch (this->m_op_type){
        case REDUCE_MIN:
            math::arm::reduce_min_all<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        case REDUCE_MAX:
            math::arm::reduce_all<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        case REDUCE_MEAN:
            math::arm::reduce_mean_all<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        case REDUCE_SUM:
            math::arm::reduce_sum_all<float>(x_data, out_data, x_b*x_c*x_h*x_w);
            break;
        case REDUCE_PROD:
            math::arm::reduce_prod_all<float>(x_data, out_data, x_b*x_c*x_h*x_w);
            break;
        default:
            break;
        }
    }
    else if(this->m_reduce_hw){
        switch (this->m_op_type){
        case REDUCE_MIN:
            math::arm::reduce_min_hw<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        case REDUCE_MAX:
            math::arm::reduce_hw<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        case REDUCE_MEAN:
            math::arm::reduce_mean_hw<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        case REDUCE_SUM:
            math::arm::reduce_sum_hw<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        case REDUCE_PROD:
            math::arm::reduce_prod_hw<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        default:
            break;
        }
    }
    else if(this->m_reduce_ch){
        switch (this->m_op_type){
        case REDUCE_MIN:
            math::arm::reduce_min_ch<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        case REDUCE_MAX:
            math::arm::reduce_ch<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        case REDUCE_MEAN:
            math::arm::reduce_mean_ch<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        case REDUCE_SUM:
            math::arm::reduce_sum_ch<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        case REDUCE_PROD:
            math::arm::reduce_prod_ch<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        default:
            break;
        }
    }
    else if(this->m_reduce_nc){
        switch (this->m_op_type){
        case REDUCE_MIN:
            math::arm::reduce_min_nc<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        case REDUCE_MAX:
            math::arm::reduce_nc<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        case REDUCE_MEAN:
            math::arm::reduce_mean_nc<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        case REDUCE_SUM:
            math::arm::reduce_sum_nc<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        case REDUCE_PROD:
            math::arm::reduce_prod_nc<float>(x_data, out_data, x_b, x_c, x_h, x_w);
            break;
        default:
            break;
        }
    }

    return 0;
}

int ReduceOp::runOnGpu(std::vector<Tensor> input){
    EAGLEEYE_LOGE("Dont implement (GPU)");
    return -1;
}
}
}