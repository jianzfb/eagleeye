#ifndef _EAGLEEYE_TENSOR_OP_
#define _EAGLEEYE_TENSOR_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class TensorInt32Op: public BaseOp<1, 1>,DynamicCreator<TensorInt32Op>{
public:
    using BaseOp<1, 1>::init;
    TensorOp(){
        
    }
    virtual ~TensorOp(){}

    virtual int init(std::map<std::string, std::vector<float>> params){
        if(params.find("init_val") != params.end() && params.find("init_shape") != params.end()){
            std::vector<float> init_shape = params["init_shape"];
            std::vector<float> init_val = params["init_val"];

            int64_t num = 1;
            std::vector<int64_t> tensor_shape;
            for(int i=0; i<init_shape.size(); ++i){
                tensor_shape.push_back(int64_t(init_shape[i]));
                num *= int64_t(init_shape[i]);
            }
            if(num != init_val.size()){
                EAGLEEYE_LOGE("Init tensor size not consistent.");
                return;
            }
            m_latest_val = Tensor(
                tensor_shape,
                EAGLEEYE_INT32,
                DataFormat::AUTO,
                CPU_BUFFER
            );

            int* init_val_ptr = m_latest_val.cpu<int>();
            for(int i=0; i<num; ++i){
                init_val_ptr[i] = int(init_val[i]);
            }
        }
        return 0;
    };
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input){
        if(m_first_call){
            this->m_first_call = false;
            this->m_outputs[0] = m_latest_val;
        }
        else{
            if(input.size() == 0 || input[0].empty()){
                this->m_outputs[0] = m_latest_val;
            }
            else{
                if(input[0].type() != EAGLEEYE_INT32){
                    EAGLEEYE_LOGE("Input Tensor type not consistent.");
                    return 0;
                }
                this->m_outputs[0] = input[0];
                m_latest_val = input[0];
            }
        }
        return 0;
    }
    virtual int runOnGpu(const std::vector<Tensor>& input){
        return 0;
    }

private:
    bool m_first_call;
    Tensor m_latest_val;
};

class TensorFloat32Op: public BaseOp<1, 1>,DynamicCreator<TensorFloat32Op>{
public:
    using BaseOp<1, 1>::init;
    TensorOp(){
        
    }
    virtual ~TensorOp(){}

    virtual int init(std::map<std::string, std::vector<float>> params){
        if(params.find("init_val") != params.end() && params.find("init_shape") != params.end()){
            std::vector<float> init_shape = params["init_shape"];
            std::vector<float> init_val = params["init_val"];

            int64_t num = 1;
            std::vector<int64_t> tensor_shape;
            for(int i=0; i<init_shape.size(); ++i){
                tensor_shape.push_back(int64_t(init_shape[i]));
                num *= int64_t(init_shape[i]);
            }
            if(num != init_val.size()){
                EAGLEEYE_LOGE("Init tensor size not consistent.");
                return;
            }
            m_latest_val = Tensor(
                tensor_shape,
                EAGLEEYE_FLOAT32,
                DataFormat::AUTO,
                CPU_BUFFER
            );

            float* init_val_ptr = m_latest_val.cpu<float>();
            for(int i=0; i<num; ++i){
                init_val_ptr[i] = init_val[i];
            }
        }
        return 0;
    };
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input){
        if(m_first_call){
            this->m_first_call = false;
            this->m_outputs[0] = m_latest_val;
        }
        else{
            if(input.size() == 0 || input[0].empty()){
                this->m_outputs[0] = m_latest_val;
            }
            else{
                if(input[0].type() != EAGLEEYE_FLOAT32){
                    EAGLEEYE_LOGE("Input Tensor type not consistent.");
                    return 0;
                }
                this->m_outputs[0] = input[0];
                m_latest_val = input[0];
            }
        }
        return 0;
    }
    virtual int runOnGpu(const std::vector<Tensor>& input){
        return 0;
    }

private:
    bool m_first_call;
    Tensor m_latest_val;
};


class TensorFloat64Op: public BaseOp<1, 1>,DynamicCreator<TensorFloat64Op>{
public:
    using BaseOp<1, 1>::init;
    TensorOp(){}
    virtual ~TensorOp(){}

    virtual int init(std::map<std::string, std::vector<float>> params){
        if(params.find("init_val") != params.end() && params.find("init_shape") != params.end()){
            std::vector<float> init_shape = params["init_shape"];
            std::vector<float> init_val = params["init_val"];

            int64_t num = 1;
            std::vector<int64_t> tensor_shape;
            for(int i=0; i<init_shape.size(); ++i){
                tensor_shape.push_back(int64_t(init_shape[i]));
                num *= int64_t(init_shape[i]);
            }
            if(num != init_val.size()){
                EAGLEEYE_LOGE("Init tensor size not consistent.");
                return;
            }
            m_latest_val = Tensor(
                tensor_shape,
                EAGLEEYE_DOUBLE,
                DataFormat::AUTO,
                CPU_BUFFER
            );

            double* init_val_ptr = m_latest_val.cpu<double>();
            for(int i=0; i<num; ++i){
                init_val_ptr[i] = double(init_val[i]);
            }
        }
        return 0;
    };
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input){
        if(m_first_call){
            this->m_first_call = false;
            this->m_outputs[0] = m_latest_val;
        }
        else{
            if(input.size() == 0 || input[0].empty()){
                this->m_outputs[0] = m_latest_val;
            }
            else{
                if(input[0].type() != EAGLEEYE_DOUBLE){
                    EAGLEEYE_LOGE("Input Tensor type not consistent.");
                    return 0;
                }
                this->m_outputs[0] = input[0];
                m_latest_val = input[0];
            }
        }
        return 0;
    }
    virtual int runOnGpu(const std::vector<Tensor>& input){
        return 0;
    }

private:
    bool m_first_call;
    Tensor m_latest_val;
};
}
}
#endif