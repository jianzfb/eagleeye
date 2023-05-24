#ifndef _EAGLEEYE_BASE_H_
#define _EAGLEEYE_BASE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/engine/nano/dataflow/meta.hpp"
#include <vector>
#include <map>
#include <string>
#define OP_SUPPORT(DEVICE) \
    if (std::string(#DEVICE) == "CPU"){ \
        this->m_support_cpu = true; \
    } \
    else if(std::string(#DEVICE) == "GPU"){ \
        this->m_support_gpu = true; \
    }

namespace eagleeye{
namespace dataflow{
typedef	int64_t 	index_t;

template<typename T, std::size_t IN, std::size_t OUT>
class BaseOp{
public:
    typedef T                           Type;
    typedef make_index_sequence<IN>     INS;
    typedef make_index_sequence<OUT>    OUTS;

    /**
     * @brief Construct a new Base Op object
     * 
     */
    BaseOp()
        :m_input_num(IN),m_output_num(OUT){
            this->m_input_shape.resize(m_input_num);
            this->m_output_shape.resize(m_output_num);
            this->m_support_cpu = false;
            this->m_support_gpu = false;
            this->m_outputs.resize(OUT);
        };

    /**
     * @brief Destroy the Base Op object
     * 
     */
    virtual ~BaseOp(){};

    /**
     * @brief init
     * 
     * @param data 
     */
    virtual int init(std::map<std::string, std::vector<float>> params)=0;

    /**
     * @brief run on cpu
     * 
     * @param output 
     * @param input 
     */
    virtual int runOnCpu(const std::vector<T>& input)=0;

    /**
     * @brief run on gpu
     * 
     * @param output 
     * @param input 
     */
    virtual int runOnGpu(const std::vector<T>& input)=0;

    /**
     * @brief check whether support implementation
     * 
     * @return true 
     * @return false 
     */
    bool isSupportCPU(){return m_support_cpu;}
    bool isSupportGPU(){return m_support_gpu;}

    /**
     * @brief Get the Output Num object
     * 
     * @return int 
     */
    int getOutputNum(){
        return this->m_output_num;
    }

    /**
     * @brief Get the Input Num object
     * 
     * @return int 
     */
    int getInputNum(){
        return this->m_input_num;
    }    

    /**
     * @brief Get the Output Shape object
     * 
     * @param index 
     * @return std::vector<int64_t> 
     */
    std::vector<int64_t> getOutputShape(int index){
        return this->m_output_shape[index];
    }

    /**
     * @brief Get the Input Shape object
     * 
     * @param index 
     * @return std::vector<int64_t> 
     */
    std::vector<int64_t> getInputShape(int index){
        return this->m_input_shape[index];
    }

    /**
     * @brief Get the Output Tensor object
     * 
     * @param index 
     * @return T& 
     */
    virtual T& getOutput(int index){
        return this->m_outputs[index];
    }

    virtual int getOutputSize(int index){
        if(index >= this->m_output_num){
            EAGLEEYE_LOGD("Index out of bounds.");
            return 0;
        }

        return this->m_outputs[index].blobsize();
    }

    virtual int update(T data, int index){return -1;}

    /**
     * @brief use cpu data, update
     */
    virtual int update(void* data, std::vector<int64_t> shape, int index){return -1;}

    /**
     * @brief get cpu data, from 
     */
    virtual int fetch(void*& data, std::vector<int64_t>& shape, EagleeyeType type, int index, bool block){
        if(!block){
            this->getOutput(index).transfer(EagleeyeRuntime(EAGLEEYE_CPU));
            return -1;
        }
        else{
            data = this->getOutput(index).cpu();
            shape = this->getOutput(index).dims().data();
            type = this->getOutput(index).type();
            return 0;
        }        
    }

    /**
     * @brief clear resource
     * 
     */
    virtual void clear(){}

protected:
    int m_input_num;
    int m_output_num;

    std::vector<std::vector<int64_t>> m_output_shape;
    std::vector<std::vector<int64_t>> m_input_shape;
    std::vector<T> m_outputs;

    bool m_support_cpu;
    bool m_support_gpu;
};
}
}
#endif