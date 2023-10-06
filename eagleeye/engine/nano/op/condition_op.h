#ifndef _EAGLEEYE_CONDITION_OP_
#define _EAGLEEYE_CONDITION_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/basic/Dim.h"
#include "eagleeye/basic/type.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class ConditionOp:public BaseOp<1, 1>{
public:
    using BaseOp<1, 1>::init;
    ConditionOp(){}
    virtual ~ConditionOp(){}

    virtual int init(std::map<std::string, std::vector<float>> params){
        if(params.find("init_val") != params.end()){
            this->m_init_val = (bool)(params["init_val"][0]);
        }
        return 0;
    }
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){
        this->m_first_call = true;
        return 0;
    }

    virtual int runOnCpu(const std::vector<Tensor>& input){
        if(this->m_outputs[0].empty()){
            this->m_outputs[0] = Tensor(
                std::vector<int64_t>{1},
                EAGLEEYE_BOOL,
                DataFormat::AUTO,
                CPU_BUFFER
            );
        }

        bool* ptr = this->m_outputs[0].cpu<bool>();
        if(this->m_first_call){
            this->m_first_call = false;
            ptr[0] = this->m_init_val;
        }
        else{
            if(input.size() == 0 || input[0].empty()){
                ptr[0] = this->m_init_val;
            }
            else{
                ptr[0] = *(input[0].cpu<bool>());
            }
        }
        return 0;
    }

    virtual int runOnGpu(const std::vector<Tensor>& input){
        return 0;
    }

private:
    bool m_init_val;
    bool m_first_call;
};
}
}

#endif