#include "eagleeye/engine/nano/op/identity_op.h"
namespace eagleeye{
namespace dataflow{
IdentityOp::IdentityOp(){
    OP_SUPPORT(CPU);
    OP_SUPPORT(GPU);
}    

IdentityOp::IdentityOp(const IdentityOp& op){
    OP_SUPPORT(CPU);
    OP_SUPPORT(GPU);
}

int IdentityOp::runOnCpu(const std::vector<Tensor>& input){
    this->m_outputs[0] = input[0];
    return 0;
}
int IdentityOp::runOnGpu(const std::vector<Tensor>& input){
    this->m_outputs[0] = input[0];
    return 0;
}
} // namespace dataflow
} // namespace eagleeye
