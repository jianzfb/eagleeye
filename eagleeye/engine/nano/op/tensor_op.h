#ifndef _EAGLEEYE_TENSOR_OP_
#define _EAGLEEYE_TENSOR_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class TensorOp: public BaseOp<1, 1>,DynamicCreator<TensorOp>{
public:
    using BaseOp<1, 1>::init;
    TensorOp(){
        OP_SUPPORT(CPU);
        OP_SUPPORT(GPU);
    }
    TensorOp(const TensorOp& op){
        OP_SUPPORT(CPU);
        OP_SUPPORT(GPU);
    }

    virtual ~TensorOp(){}

    virtual int init(std::map<std::string, std::vector<float>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input){
        if(input.size() > 0){
            this->m_outputs[0] = input[0];
        }
        return 0;
    }
    virtual int runOnGpu(const std::vector<Tensor>& input){
        if(input.size() > 0){
            this->m_outputs[0] = input[0];
        }
        return 0;
    }

protected:

};
}    
}
#endif