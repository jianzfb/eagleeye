#ifndef _EAGLEEYE_GROUP_OP_
#define _EAGLEEYE_GROUP_OP_
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
class GroupOp:public BaseOp<IN,OUT>{
public:
    using BaseOp<IN,OUT>::init; 
    GroupOp(){}
    virtual ~GroupOp(){
        for(int op_i=0; op_i<m_ops.size(); ++op_i){
            delete m_ops[op_i];
        }
        this->m_ops.clear();
    }

    virtual int init(std::map<std::string, std::vector<float>> params){
        for(int op_i=0; op_i<m_ops.size(); ++op_i){
            m_ops[op_i]->init(params);
        }
        return 0;
    };
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){
        for(int op_i=0; op_i<m_ops.size(); ++op_i){
            m_ops[op_i]->init(params);
        }
        return 0;
    };
    virtual int init(std::map<std::string, std::vector<std::string>> params){
        for(int op_i=0; op_i<m_ops.size(); ++op_i){
            m_ops[op_i]->init(params);
        }
        return 0;
    }
    virtual int init(std::map<std::string, void*> params){
        for(int op_i=0; op_i<m_ops.size(); ++op_i){
            m_ops[op_i]->init(params);
        }
        return 0;
    }

    virtual int runOnCpu(const std::vector<Tensor>& input){
        if(this->m_ops.size() == 0){
            EAGLEEYE_LOGE("group ops zero, return.");
            return 0;
        }

        int ops_num = this->m_ops.size();
        this->m_ops[0]->runOnCpu(input);
        for(int op_i=1; op_i<ops_num; ++op_i){
            this->m_ops[op_i]->runOnCpu(this->m_ops[op_i-1]->m_outputs);
        }

        this->m_outputs = this->m_ops[ops_num-1]->m_outputs;
        return 0;
    }
    virtual int runOnGpu(const std::vector<Tensor>& input){return 0;}

protected:
    std::vector<Base*> m_ops;

};
}
}
#endif