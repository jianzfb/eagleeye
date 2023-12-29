#include "eagleeye/engine/nano/op/switch_op.h"
namespace eagleeye{
namespace dataflow{
SwitchOp::SwitchOp(){

}
SwitchOp::~SwitchOp(){

}

int SwitchOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

int SwitchOp::runOnCpu(const std::vector<Tensor>& input){
    const int* state_info_ptr = input[0].cpu<int>();
    int state_val = state_info_ptr[0];

    this->m_outputs[0] = input[state_val + 1];
    return 0;
}

int SwitchOp::runOnGpu(const std::vector<Tensor>& input){
    return 0;
}
}    
}