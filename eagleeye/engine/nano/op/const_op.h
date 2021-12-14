#ifndef _EAGLEEYE_CONST_OP_
#define _EAGLEEYE_CONST_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/basic/Dim.h"
#include "eagleeye/basic/type.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{

template<typename T>
class ConstOp:public BaseOp<Tensor, 0, 1>{
public:
    ConstOp(){}

    ConstOp(T val, std::vector<int64_t> shape, std::string device="CPU"){
        m_type = TypeTrait<T>::type;
        if(!(m_type == EAGLEEYE_FLOAT || m_type == EAGLEEYE_INT)){
            EAGLEEYE_LOGE("Data type dont support.");
            return;
        }
        m_dims.ConstructFrom(shape);
        m_shape = shape;
        m_device = device;
        m_val = val;
    }
    
    ConstOp(std::vector<T> value, std::vector<int64_t> shape, std::string device="CPU"){
        m_type = TypeTrait<T>::type;
        if(!(m_type == EAGLEEYE_FLOAT || m_type == EAGLEEYE_INT)){
            EAGLEEYE_LOGE("Data type dont support.");
            return;
        }
        m_dims.ConstructFrom(shape);
        m_shape = shape;
        m_device = device;
        m_vals = value;
    }
    
    virtual ~ConstOp(){};

    virtual int init(std::map<std::string, std::vector<float>> params){
        if(m_device == "CPU"){
            // CPU
            this->m_outputs[0] = Tensor(
                    m_shape,
                    m_type,
                    DataFormat::NONE,
                    CPU_BUFFER
                );

            if(m_vals.size() > 0){
                memcpy(this->m_outputs[0].cpu(), m_vals.data(), sizeof(T)*m_dims.production());
            }
            else{
                T* data_ptr = (T*)this->m_outputs[0].cpu();
                for(int i=0; i<m_dims.production(); ++i){
                    data_ptr[i] = m_val;
                }
            }
        }
        else{
            // GPU
            this->m_outputs[0] = Tensor(
                    m_shape,
                    m_type,
                    DataFormat::NONE,
                    GPU_BUFFER
                );
            // 赋值
        }

        return 0;
    }

    virtual int runOnCpu(std::vector<Tensor> input=std::vector<Tensor>()){
        return 0;
    }

    virtual int runOnGpu(std::vector<Tensor> input=std::vector<Tensor>()){
        return 0;
    }

protected:
    Dim m_dims;
    std::vector<int64_t> m_shape;
    std::vector<T> m_vals;
    T m_val;
    EagleeyeType m_type;
    std::string m_device;
};


template<typename T>
class ZerosOp:public ConstOp<T>{
public:
    ZerosOp(std::vector<int64_t> shape, std::string device):
        ConstOp<T>(0, shape, device){
        }

    virtual ~ZerosOp(){}
};

template<typename T>
class OnesOp:public ConstOp<T>{
public:
    OnesOp(std::vector<int64_t> shape, std::string device):
        ConstOp<T>(1, shape, device){
        }
      
    virtual ~OnesOp(){};
};
} // namespace dataflow
} // namespace eagleeye


#endif