#ifndef _EAGLEEYE_SCALAR_OP_
#define _EAGLEEYE_SCALAR_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/basic/Dim.h"
#include "eagleeye/basic/type.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class ScalarInt32Op:public BaseOp<1,1>,DynamicCreator<ScalarInt32Op>{
public:
    using BaseOp<1, 1>::init;
    ScalarInt32Op(){
        this->m_init_val = 0;
        this->m_latest_val = 0;
    }
    virtual ~ScalarInt32Op(){}

    virtual int init(std::map<std::string, std::vector<float>> params){
        if(params.find("init_val") != params.end()){
            this->m_init_val = (int)(params["init_val"][0]);
        }
        this->m_first_call = true;
        return 0;
    }

    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){
        this->m_first_call = true;
        return 0;
    };
    virtual int init(std::map<std::string, std::vector<std::string>> params){
        this->m_first_call = true;
        return 0;
    }

    virtual int runOnCpu(const std::vector<Tensor>& input){
        if(this->m_outputs[0].empty()){
            this->m_outputs[0] = Tensor(
                std::vector<int64_t>{1},
                EAGLEEYE_INT32,
                DataFormat::AUTO,
                CPU_BUFFER
            );
        }

        int* ptr = this->m_outputs[0].cpu<int>();
        if(this->m_first_call){
            this->m_first_call = false;
            ptr[0] = this->m_init_val;
            m_latest_val = this->m_init_val;
        }
        else{
            if(input.size() == 0 || input[0].empty()){
                ptr[0] = m_latest_val;
            }
            else{
                ptr[0] = *(input[0].cpu<int>());
                m_latest_val = ptr[0];
            }
        }

        return 0;
    }
    virtual int runOnGpu(const std::vector<Tensor>& input){
        return 0;
    }

private:
    int m_init_val;
    int m_latest_val;
    bool m_first_call;
};

class ScalarFloat32Op:public BaseOp<1,1>,DynamicCreator<ScalarFloat32Op>{
public:
    using BaseOp<1, 1>::init;
    ScalarFloat32Op(){
        this->m_init_val = 0.0f;
        this->m_latest_val = 0.0f;
    }
    virtual ~ScalarFloat32Op(){}

    virtual int init(std::map<std::string, std::vector<float>> params){
        if(params.find("init_val") != params.end()){
            this->m_init_val = (float)(params["init_val"][0]);
        }
        this->m_first_call = true;
        return 0;
    }

    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){
        this->m_first_call = true;
        return 0;
    };
    virtual int init(std::map<std::string, std::vector<std::string>> params){
        this->m_first_call = true;
        return 0;
    }

    virtual int runOnCpu(const std::vector<Tensor>& input){
        if(this->m_outputs[0].empty()){
            this->m_outputs[0] = Tensor(
                std::vector<int64_t>{1},
                EAGLEEYE_INT32,
                DataFormat::AUTO,
                CPU_BUFFER
            );
        }

        float* ptr = this->m_outputs[0].cpu<float>();
        if(this->m_first_call){
            this->m_first_call = false;
            ptr[0] = this->m_init_val;
            m_latest_val = this->m_init_val;
        }
        else{
            if(input.size() == 0 || input[0].empty()){
                ptr[0] = m_latest_val;
            }
            else{
                ptr[0] = *(input[0].cpu<float>());
                m_latest_val = ptr[0];
            }
        }

        return 0;
    }
    virtual int runOnGpu(const std::vector<Tensor>& input){
        return 0;
    }

private:
    float m_init_val;
    float m_latest_val;
    bool m_first_call;
};

class ScalarFloat64Op:public BaseOp<1,1>,DynamicCreator<ScalarFloat64Op>{
public:
    using BaseOp<1, 1>::init;
    ScalarFloat64Op(){
        this->m_init_val = 0.0;
        this->m_latest_val = 0.0;
    }
    virtual ~ScalarFloat64Op(){}

    virtual int init(std::map<std::string, std::vector<float>> params){
        if(params.find("init_val") != params.end()){
            this->m_init_val = (float)(params["init_val"][0]);
        }
        this->m_first_call = true;
        return 0;
    }

    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){
        this->m_first_call = true;
        return 0;
    };
    virtual int init(std::map<std::string, std::vector<std::string>> params){
        this->m_first_call = true;
        return 0;
    }

    virtual int runOnCpu(const std::vector<Tensor>& input){
        if(this->m_outputs[0].empty()){
            this->m_outputs[0] = Tensor(
                std::vector<int64_t>{1},
                EAGLEEYE_INT32,
                DataFormat::AUTO,
                CPU_BUFFER
            );
        }

        double* ptr = this->m_outputs[0].cpu<double>();
        if(this->m_first_call){
            this->m_first_call = false;
            ptr[0] = this->m_init_val;
            m_latest_val = this->m_init_val;
        }
        else{
            if(input.size() == 0 || input[0].empty()){
                ptr[0] = m_latest_val;
            }
            else{
                ptr[0] = *(input[0].cpu<double>());
                m_latest_val = ptr[0];
            }
        }

        return 0;
    }
    virtual int runOnGpu(const std::vector<Tensor>& input){
        return 0;
    }

private:
    double m_init_val;
    double m_latest_val;
    bool m_first_call;
};
}    
}

#endif