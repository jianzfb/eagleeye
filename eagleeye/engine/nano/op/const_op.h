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

    ConstOp(T value, std::vector<int64_t> shape, std::string device="CPU")
        :m_device(device){
        this->m_outputs[0] = Tensor(
            shape,
            TypeTrait<T>::type,
            DataFormat::AUTO,
            CPU_BUFFER
        );

        int64_t out_total = this->m_outputs[0].numel();
        T* data_ptr = (T*)this->m_outputs[0].cpu();
        for(int i=0; i<out_total; ++i){
            data_ptr[i] = value;
        }
    }
    
    ConstOp(std::vector<T> value,  std::string device="CPU")
        :m_device(device){
        int64_t num = value.size();
        this->m_outputs[0] = Tensor(
            std::vector<int64_t>{num},
             TypeTrait<T>::type,
            DataFormat::AUTO,
            CPU_BUFFER
        );

        T* data_ptr = (T*)this->m_outputs[0].cpu();
        for(int i=0; i<num; ++i){
            data_ptr[i] = value[i];
        }
    }
    
    virtual ~ConstOp(){};

    virtual int init(std::map<std::string, std::vector<float>> params){
        if(params.find("value") == params.end()){
            return 0;
        }

        std::vector<float> data = params["value"];
        int num = data.size();
        this->m_outputs[0] = Tensor(
            std::vector<int64_t>{num},
             TypeTrait<T>::type,
            DataFormat::AUTO,
            CPU_BUFFER
        );

        T* data_ptr = (T*)this->m_outputs[0].cpu();
        for(int i=0; i<num; ++i){
            data_ptr[i] = T(data[i]);
        }

        return 0;
    }
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){
        if(params.find("value") == params.end()){
            return 0;
        }

        std::vector<std::vector<float>> data = params["value"];
        int64_t dim_0_num = data.size();
        if(dim_0_num == 0){
            this->m_outputs[0] = Tensor(
                std::vector<int64_t>{0,0},
                TypeTrait<T>::type,
                DataFormat::AUTO,
                CPU_BUFFER
            );
            return 0;
        }
        else{
            int64_t dim_1_num = data[0].size();
            this->m_outputs[0] = Tensor(
                std::vector<int64_t>{dim_0_num, dim_1_num},
                TypeTrait<T>::type,
                DataFormat::AUTO,
                CPU_BUFFER
            );

            if(dim_0_num > 0 && dim_1_num > 0){
                for(int i=0; i<dim_0_num; ++i){
                    T* ptr = this->m_outputs[0].template cpu<T>() + i * dim_1_num;
                    memcpy(ptr, data[i].data(), sizeof(T)*dim_1_num);
                }
            }
        }
        return 0;
    };
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}

    virtual int runOnCpu(const std::vector<Tensor>& input){
        return 0;
    }

    virtual int runOnGpu(const std::vector<Tensor>& input){
        return 0;
    }

protected:
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