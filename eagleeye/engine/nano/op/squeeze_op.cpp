#include "eagleeye/engine/nano/squeeze_op.h"

namespace eagleeye{
namespace dataflow{
SqueezeOp::SqueezeOp(size_t axis){
    this->m_axis = axis;
}

SqueezeOp(const SqueezeOp& op)
    :m_axis(op.axis){
}
SqueezeOp::SqueezeOp(){

}

int SqueezeOp::init(std::map<std::string, std::vector<float>> params){
    if(params.size() == 0){
        return 0;
    }

    if(params.find("axis") != params.end()){
        this->m_axis = (size_t)(params.get("axis")[0]);
    }
    return 0;
}

int SqueezeOp::runOnCpu(const std::vector<Tensor>& input){
    // 仅调整shape形状
    this->m_outputs[0] = input[0];
    this->m_outputs[0].squeeze(this->m_axis);
    return 0;
}

int SqueezeOp::runOnGpu(const std::vector<Tensor>& input){
    return 0;
}

///////////////////////

UnSqueezeOp::UnSqueezeOp(size_t axis){
    this->m_axis = axis;
}

UnSqueezeOp(const UnSqueezeOp& op)
    :m_axis(op.axis){
}
UnSqueezeOp::UnSqueezeOp(){

}

int UnSqueezeOp::init(std::map<std::string, std::vector<float>> params){
    if(params.size() == 0){
        return 0;
    }

    if(params.find("axis") != params.end()){
        this->m_axis = (size_t)(params.get("axis")[0]);
    }
    return 0;
}

int UnSqueezeOp::runOnCpu(const std::vector<Tensor>& input){
    // 仅调整shape形状
    this->m_outputs[0] = input[0];
    this->m_outputs[0].unsqueeze(this->m_axis);
    return 0;
}

int UnSqueezeOp::runOnGpu(const std::vector<Tensor>& input){
    return 0;
}


}    
}