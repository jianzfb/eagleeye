#ifndef _EAGLEEYE_STATE_DYNAMIC_SELECT_OP_
#define _EAGLEEYE_STATE_DYNAMIC_SELECT_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/basic/Dim.h"
#include "eagleeye/basic/type.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class StateDynamicSelectOp:public BaseOp<2,1>, DynamicCreator<StateDynamicSelectOp>{
public:
    using BaseOp<2,1>::init;
    StateDynamicSelectOp(){

    }
    virtual ~StateDynamicSelectOp(){

    }

    virtual int init(std::map<std::string, std::vector<float>> params){
        return 0;
    }
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){
        return 0;
    }
    virtual int init(std::map<std::string, std::vector<std::string>> params){
        return 0;
    }

    virtual int runOnCpu(const std::vector<Tensor>& input){
        Tensor a = input[0];
        Tensor b = input[1];

        if(this->m_outputs[0].empty()){
            this->m_outputs[0] = Tensor(
                std::vector<int64_t>{1},
                EAGLEEYE_INT32,
                DataFormat::AUTO,
                CPU_BUFFER
            );
        }
        int* output_ptr =  this->m_outputs[0].cpu<int>();

        if(a.type() == EAGLEEYE_DOUBLE){
            double* a_ptr = a.cpu<double>();
            double* b_ptr = b.cpu<double>();

            if(a_ptr[0] >= b_ptr[0]){
                output_ptr[0] = 1;
            }
            else{
                output_ptr[0] = 0;
            }
        }
        else if(a.type() == EAGLEEYE_FLOAT){
            float* a_ptr = a.cpu<float>();
            float* b_ptr = b.cpu<float>();

            if(a_ptr[0] >= b_ptr[0]){
                output_ptr[0] = 1;
            }
            else{
                output_ptr[0] = 0;
            }
        }
        else if(a.type() == EAGLEEYE_CHAR){
            char* a_ptr = a.cpu<char>();
            char* b_ptr = b.cpu<char>();

            if(a_ptr[0] >= b_ptr[0]){
                output_ptr[0] = 1;
            }
            else{
                output_ptr[0] = 0;
            }
        }
        else if(a.type() == EAGLEEYE_UCHAR){
            unsigned char* a_ptr = a.cpu<unsigned char>();
            unsigned char* b_ptr = b.cpu<unsigned char>();

            if(a_ptr[0] >= b_ptr[0]){
                output_ptr[0] = 1;
            }
            else{
                output_ptr[0] = 0;
            }
        }
        else if(a.type() == EAGLEEYE_INT){
            int* a_ptr = a.cpu<int>();
            int* b_ptr = b.cpu<int>();

            if(a_ptr[0] >= b_ptr[0]){
                output_ptr[0] = 1;
            }
            else{
                output_ptr[0] = 0;
            }
        }
        else{
            EAGLEEYE_LOGE("tensor data type abnormal.");
        }

        return 0;
    }

    virtual int runOnGpu(const std::vector<Tensor>& input){
        return 0;
    }
};
}
}
#endif