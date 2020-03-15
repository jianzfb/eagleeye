#ifndef _EAGLEEYE_FIXEDRELUOP_H_
#define _EAGLEEYE_FIXEDRELUOP_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/FixedCNNOp.h"
namespace eagleeye{
namespace nano{
class FixedReluOp:public FixedCNNOp{
public: 
    /**
     * @brief Construct a new Fixed Relu Op object
     * 
     * @param input_data_num 
     * @param output_data_num 
     * @param op_name 
     */
    FixedReluOp(int input_data_num, int output_data_num, std::string op_name);
    
    /**
     * @brief Destroy the Fixed Relu Op object
     * 
     */
    virtual ~FixedReluOp();

    /**
     * @brief run fixed relu on cpu
     * 
     * @param output 
     * @param input 
     */
    void foward_on_cpu(std::vector<Tensor<FixedType>>& output, std::vector<Tensor<FixedType>>& input);
    
    /**
     * @brief run fixed relu on gpu
     * 
     * @param output 
     * @param input 
     */
    void foward_on_gpu(std::vector<Tensor<FixedType>>& output, std::vector<Tensor<FixedType>>& input);

    /**
     * @brief run fixed relu on dsp
     * 
     * @param output 
     * @param input 
     */
    void foward_on_dsp(std::vector<Tensor<FixedType>>& output, std::vector<Tensor<FixedType>>& input);

    /**
     * @brief initialize Fixed Relu Op
     * 
     * @param buf 
     * @param in_size 
     */
    virtual bool init(char *buf, int in_size);

    /**
     * @brief Set the Shape object
     * 
     * @param shape 
     * @return int 
     */
    virtual int setShape(std::vector<int64_t> shape);    
};    
}
}
#endif