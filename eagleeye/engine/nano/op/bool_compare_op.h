#ifndef _EAGLEEYE_BOOLEQUALCOMPARE_OP_
#define _EAGLEEYE_BOOLEQUALCOMPARE_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/basic/Dim.h"
#include "eagleeye/basic/type.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class BoolEqualCompare1Op:public BaseOp<1,1>, DynamicCreator<BoolEqualCompare1Op>{
public:
    using BaseOp<1,1>::init;
    BoolEqualCompare1Op(double init_val){
        m_init_val = init_val;
    }
    BoolEqualCompare1Op(){

    }
    virtual ~BoolEqualCompare1Op(){}

    virtual int init(std::map<std::string, std::vector<float>> params){
        if(params.find("init_val") != params.end()){
            this->m_init_val = (double)(params["init_val"][0]);
        }
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
        if(this->m_outputs[0].empty()){
            this->m_outputs[0] = Tensor(
                std::vector<int64_t>{1},
                EAGLEEYE_BOOL,
                DataFormat::AUTO,
                CPU_BUFFER
            );
        }
        bool* output_ptr =  this->m_outputs[0].cpu<bool>();

        if(a.type() == EAGLEEYE_DOUBLE){
            double* a_ptr = a.cpu<double>();
            if(a_ptr[0] == this->m_init_val){
                output_ptr[0] = true;
            }
            else{
                output_ptr[1] = false;
            }
        }
        else if(a.type() == EAGLEEYE_FLOAT){
            float* a_ptr = a.cpu<float>();
            if(a_ptr[0] == (float)(m_init_val)){
                output_ptr[0] = true;
            }
            else{
                output_ptr[1] = false;
            }
        }
        else if(a.type() == EAGLEEYE_CHAR){
            char* a_ptr = a.cpu<char>();
            if(a_ptr[0] == (char)(m_init_val)){
                output_ptr[0] = true;
            }
            else{
                output_ptr[1] = false;
            }
        }
        else if(a.type() == EAGLEEYE_BOOL){
            bool* a_ptr = a.cpu<bool>();
            if(a_ptr[0] == (bool)(m_init_val)){
                output_ptr[0] = true;
            }
            else{
                output_ptr[1] = false;
            }
        }
        else if(a.type() == EAGLEEYE_UCHAR){
            unsigned char* a_ptr = a.cpu<unsigned char>();
            if(a_ptr[0] == ){
                output_ptr[0] = true;
            }
            else{
                output_ptr[1] = false;
            }
        }
        else if(a.type() == EAGLEEYE_INT){
            int* a_ptr = a.cpu<int>();
            int* b_ptr = b.cpu<int>();
            if(a_ptr[0] == b_ptr[1]){
                output_ptr[0] = true;
            }
            else{
                output_ptr[1] = false;
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

private:
    double m_init_val;
};

class BoolEqualCompare2Op:public BaseOp<2,1>, DynamicCreator<BoolEqualCompare2Op>{
public:
    using BaseOp<2,1>::init;
    BoolEqualCompare2Op(){}
    virtual ~BoolEqualCompare2Op(){}

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
                EAGLEEYE_BOOL,
                DataFormat::AUTO,
                CPU_BUFFER
            );
        }
        bool* output_ptr =  this->m_outputs[0].cpu<bool>();

        if(a.type() == EAGLEEYE_DOUBLE){
            double* a_ptr = a.cpu<double>();
            double* b_ptr = b.cpu<double>();
            if(a_ptr[0] == b_ptr[1]){
                output_ptr[0] = true;
            }
            else{
                output_ptr[1] = false;
            }
        }
        else if(a.type() == EAGLEEYE_FLOAT){
            float* a_ptr = a.cpu<float>();
            float* b_ptr = b.cpu<float>();
            if(a_ptr[0] == b_ptr[1]){
                output_ptr[0] = true;
            }
            else{
                output_ptr[1] = false;
            }
        }
        else if(a.type() == EAGLEEYE_CHAR){
            char* a_ptr = a.cpu<char>();
            char* b_ptr = b.cpu<char>();
            if(a_ptr[0] == b_ptr[1]){
                output_ptr[0] = true;
            }
            else{
                output_ptr[1] = false;
            }
        }
        else if(a.type() == EAGLEEYE_BOOL){
            bool* a_ptr = a.cpu<bool>();
            bool* b_ptr = b.cpu<bool>();
            if(a_ptr[0] == b_ptr[1]){
                output_ptr[0] = true;
            }
            else{
                output_ptr[1] = false;
            }
        }
        else if(a.type() == EAGLEEYE_UCHAR){
            unsigned char* a_ptr = a.cpu<unsigned char>();
            unsigned char* b_ptr = b.cpu<unsigned char>();
            if(a_ptr[0] == b_ptr[1]){
                output_ptr[0] = true;
            }
            else{
                output_ptr[1] = false;
            }
        }
        else if(a.type() == EAGLEEYE_INT){
            int* a_ptr = a.cpu<int>();
            int* b_ptr = b.cpu<int>();
            if(a_ptr[0] == b_ptr[1]){
                output_ptr[0] = true;
            }
            else{
                output_ptr[1] = false;
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