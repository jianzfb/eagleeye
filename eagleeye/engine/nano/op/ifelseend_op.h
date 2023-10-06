#ifndef _EAGLEEYE_IFELSEEND_OP_
#define _EAGLEEYE_IFELSEEND_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/basic/Dim.h"
#include "eagleeye/basic/type.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{

template<std::size_t IN, std::size_t OUT>
class IfelseendOp:public BaseOp<IN,OUT>,DynamicCreator<IfelseendOp<IN,OUT>>{
public:
    using BaseOp<IN,OUT>::init;
    IfelseendOp(){
        m_true_func = NULL;
        m_false_func = NULL;
    }
    IfelseendOp(const IfelseendOp& op){
        m_true_func = op.m_true_func;
        m_false_func = op.m_false_func;
    }
    virtual ~IfelseendOp(){
        if(m_true_func){
            delete m_true_func;
        }
        if(m_false_func){
            delete m_false_func;
        }
    }

    virtual int init(std::map<std::string, std::vector<float>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    virtual int init(std::map<std::string, void*> params){
        this->m_true_func = (Base*)(params["true_func"]);
        this->m_false_func = (Base*)(params["false_func"]);

        this->m_input_num = this->m_true_func->getInputNum() + 1;
        this->m_output_num = this->m_true_func->getOutputNum();
        this->m_input_shape.resize(this->m_input_num);
        this->m_output_shape.resize(this->m_output_num);
        this->m_support_cpu = false;
        this->m_support_gpu = false;
        this->m_outputs.resize(this->m_output_num);
        return 0;
    }

    virtual int runOnCpu(const std::vector<Tensor>& input){
        const bool* condition_ptr = input[0].cpu<bool>();
        if(condition_ptr[0]){
            this->m_true_func->runOnCpu(std::vector<Tensor>(input.begin() + 1, input.end()));
            for(int output_i=0; output_i<this->getOutputNum(); ++output_i){
                this->m_outputs[output_i] = this->m_true_func->getOutput(output_i);
            }
        }
        else{
            this->m_false_func->runOnCpu(std::vector<Tensor>(input.begin() + 1, input.end()));
            for(int output_i=0; output_i<this->getOutputNum(); ++output_i){
                this->m_outputs[output_i] = this->m_false_func->getOutput(output_i);
            }
        }
        return 0;
    }
    virtual int runOnGpu(const std::vector<Tensor>& input){return 0;}

private:
    Base* m_true_func;
    Base* m_false_func;
};
}    
}

#endif