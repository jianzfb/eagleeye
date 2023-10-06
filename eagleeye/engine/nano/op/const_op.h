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
class ConstOp:public BaseOp<0, 1>{
public:
    using BaseOp<0, 1>::init;
    ConstOp(){}

    ConstOp(T val, std::vector<int64_t> shape, std::string device="CPU")
        :m_shape(shape),
         m_device(device),
         m_vals(std::vector<T>{val}){
        m_type = TypeTrait<T>::type;
        if(!(m_type == EAGLEEYE_FLOAT || m_type == EAGLEEYE_INT)){
            EAGLEEYE_LOGE("Data type dont support.");
            return;
        }
    }
    
    ConstOp(std::vector<T> value, std::vector<int64_t> shape, std::string device="CPU")
        :m_shape(shape),
         m_device(device),
         m_vals(value){
        m_type = TypeTrait<T>::type;
        if(!(m_type == EAGLEEYE_FLOAT || m_type == EAGLEEYE_INT)){
            EAGLEEYE_LOGE("Data type dont support.");
            return;
        }
    }
    
    virtual ~ConstOp(){};

    virtual int init(std::map<std::string, std::vector<float>> params){
        this->m_outputs[0] = Tensor(
            m_shape,
            m_type,
            DataFormat::AUTO,
            CPU_BUFFER
        );

        int64_t out_total = this->m_outputs[0].numel();
        int64_t vals_num = m_vals.size();
        T* data_ptr = (T*)this->m_outputs[0].cpu();
        for(int i=0; i<out_total; ++i){
            data_ptr[i] = m_vals[i%vals_num];
        }
        return 0;
    }
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}

    virtual int runOnCpu(const std::vector<Tensor>& input){
        return 0;
    }

    virtual int runOnGpu(const std::vector<Tensor>& input){
        return 0;
    }

protected:
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