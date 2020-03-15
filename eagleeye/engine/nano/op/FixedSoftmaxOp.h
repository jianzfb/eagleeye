#ifndef _EAGLEEYE_FIXEDSOFTMAXOP_H_
#define _EAGLEEYE_FIXEDSOFTMAXOP_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/FixedCNNOp.h"

namespace eagleeye{
namespace nano
{
class FixedSoftmaxOp:public FixedCNNOp{
public:    
    /**
     * @brief Construct a new Fixed Softmax Op object
     * 
     * @param input_data_num 
     * @param output_data_num 
     * @param op_name 
     */
    FixedSoftmaxOp(int input_data_num, int output_data_num, std::string op_name);
    
    /**
     * @brief Destroy the Fixed Softmax Op object
     * 
     */
    virtual ~FixedSoftmaxOp();

    /**
     * @brief run Fixed Softmax Op on cpu
     * 
     * @param output 
     * @param input 
     */
    void foward_on_cpu(std::vector<Tensor<FixedType>>& output, std::vector<Tensor<FixedType>>& input);
    
    /**
     * @brief run Fixed Softmax Op on gpu
     * 
     * @param output 
     * @param input 
     */
    void foward_on_gpu(std::vector<Tensor<FixedType>>& output, std::vector<Tensor<FixedType>>& input);

    /**
     * @brief run Fixed Softmax Op on dsp
     * 
     * @param output 
     * @param input 
     */
    void foward_on_dsp(std::vector<Tensor<FixedType>>& output, std::vector<Tensor<FixedType>>& input);

    /**
     * @brief initialize FixedSoftmaxOp
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

private:
    

};    
} // namespace nano

}
#endif