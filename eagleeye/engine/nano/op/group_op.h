#ifndef _EAGLEEYE_GROUP_OP_
#define _EAGLEEYE_GROUP_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/basic/Dim.h"
#include "eagleeye/basic/type.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include "eagleeye/common/EagleeyeLog.h"
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

    virtual int init(std::map<std::string, std::vector<float>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){
        // 内部算子链接关系
        for(int op_i=0; op_i<m_ops.size(); ++op_i){
            std::string op_i_name = std::to_string(op_i);
            if(params.find(op_i_name) != params.end()){
                // add input_ctx, output_ctx
                m_relations.push_back(params[op_i_name]);
            }
            else{
                // add empty
                m_relations.push_back(std::vector<std::string>());
            }
        }
        return 0;
    }
    virtual int init(std::map<std::string, void*> params){return 0;}

    virtual int runOnCpu(const std::vector<Tensor>& input){
        if(this->m_ops.size() == 0){
            EAGLEEYE_LOGE("group ops zero, return.");
            return 0;
        }

        std::map<std::string, Tensor> inner_data_dict;
        for(int op_i=0; op_i<m_ops.size(); ++op_i){
            std::string input_info = m_relations[op_i][0];
            std::string output_info = m_relations[op_i][1];

            std::string separator = ",";
            std::vector<std::string> input_terms = split(input_info, separator);
            std::vector<std::string> output_terms = split(output_info, separator);

            std::vector<Tensor> op_input;
            for(int op_input_i=0; op_input_i<input_terms.size(); ++op_input_i){
                std::string op_input_i_name = input_terms[op_input_i];
                if(inner_data_dict.find(op_input_i_name) == inner_data_dict.end()){
                    op_input.push_back(input[tof<int>(op_input_i_name)]);
                }
                else{
                    op_input.push_back(inner_data_dict[op_input_i_name]);
                }
            }

            this->m_ops[op_i]->runOnCpu(op_input);

            for(int op_output_i=0; op_output_i<output_terms.size(); ++op_output_i){
                inner_data_dict[output_terms[op_output_i]] = this->m_ops[op_i]->getOutput(op_output_i);
            }
        }

        for(int output_i=0; output_i<OUT; ++output_i){
            this->m_outputs[output_i] = inner_data_dict[std::to_string(output_i)];
        }

        return 0;
    }
    virtual int runOnGpu(const std::vector<Tensor>& input){return 0;}

protected:
    std::vector<Base*> m_ops;
    std::vector<std::vector<std::string>> m_relations;
};
}
}
#endif