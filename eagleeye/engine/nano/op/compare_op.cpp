#include "eagleeye/engine/nano/op/compare_op.h"
#include "eagleeye/engine/math/common/compare_compute.h"

namespace eagleeye{
namespace dataflow{
CompareOp::CompareOp(CompareOpType compare_op_type)
    :m_compare_op_type(compare_op_type){

}
CompareOp::~CompareOp(){

}

int CompareOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}
int CompareOp::runOnCpu(const std::vector<Tensor>& input){
    const Tensor x = input[0];
    const Tensor y = input[1];
    if(x.type() != EAGLEEYE_FLOAT || y.type() != EAGLEEYE_FLOAT){
        EAGLEEYE_LOGE("x,y only support float.");
        return -1;
    }
    if(x.dims().size() == 0 || x.dims()[0] == 0){
        return -1;
    }

    Tensor out = this->m_outputs[0];
    Dim out_dim = out.dims();
    Dim needed_out_dim = x.numel()>y.numel()?x.dims():y.dims();
    if(out_dim.size() == 0 || out_dim.production() != needed_out_dim.production()){
        this->m_outputs[0] =             
            Tensor(needed_out_dim.data(),
                    EAGLEEYE_BOOL,
                    DataFormat::AUTO,
                    CPU_BUFFER);  
    }

    if(this->m_compare_op_type == EQUAL_COMPARE){   
        math::CompareComputeRun<float, math::_EqualFunctor<float>>(x,y,this->m_outputs[0]);
    }
    else if(this->m_compare_op_type == NOT_EQUAL_COMPARE){
        math::CompareComputeRun<float, math::_NotEqualFunctor<float>>(x,y,this->m_outputs[0]);
    }
    else if(this->m_compare_op_type == LESS_THAN_COMPARE){
        math::CompareComputeRun<float, math::_LessThanFunctor<float>>(x,y,this->m_outputs[0]);
    }
    else if(this->m_compare_op_type == LESS_EQUAL_COMPARE){
        math::CompareComputeRun<float, math::_LessEqualFunctor<float>>(x,y,this->m_outputs[0]);
    }
    else if(this->m_compare_op_type == GREATER_THAN_COMPARE){
        math::CompareComputeRun<float, math::_GreaterThanFunctor<float>>(x,y,this->m_outputs[0]);
    }
    else if(this->m_compare_op_type == GREATER_EQUAL_COMPARE){
        math::CompareComputeRun<float, math::_GreaterEqualFunctor<float>>(x,y,this->m_outputs[0]);
    }
    return 0;
}
int CompareOp::runOnGpu(const std::vector<Tensor>& input){
    return 0;
}
} // namespace dataflow
} // namespace eagleeye
