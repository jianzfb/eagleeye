#ifndef _EAGLEEYE_BASE_H_
#define _EAGLEEYE_BASE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/engine/nano/dataflow/meta.hpp"
#include <vector>

namespace eagleeye{
namespace dataflow{
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
            std::cout<<"IN "<<m_input_num<<std::endl;
            std::cout<<"OUT "<<m_output_num<<std::endl;
            this->m_input_shape.resize(m_input_num);
            this->m_output_shape.resize(m_output_num);
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
    virtual void init(void* data)=0;

    /**
     * @brief run on cpu
     * 
     * @param output 
     * @param input 
     */
    virtual void runOnCpu(std::vector<T> output, std::vector<T> input=std::vector<T>{})=0;

    /**
     * @brief ru on gpu
     * 
     * @param output 
     * @param input 
     */
    virtual void runOnGpu(std::vector<T> output, std::vector<T> input=std::vector<T>{})=0;

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

protected:
    int m_input_num;
    int m_output_num;

    std::vector<std::vector<int64_t>> m_output_shape;
    std::vector<std::vector<int64_t>> m_input_shape;
};
}
}
#endif