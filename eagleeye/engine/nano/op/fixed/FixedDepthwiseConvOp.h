#ifndef _EAGLEEYE_FIXEDDEPTHWISECONVOP_H_
#define _EAGLEEYE_FIXEDDEPTHWISECONVOP_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/FixedCNNOp.h"
#include <vector>

namespace eagleeye{
namespace nano
{
class FixedDepthwiseConvOp:public FixedCNNOp{
public:
    /**
     * @brief Construct a new Fixed DepthwiseConv Op object
     * 
     * @param input_data_num 
     * @param output_data_num 
     * @param op_name 
     */
    FixedDepthwiseConvOp(int input_data_num, int output_data_num, std::string op_name);
    
    /**
     * @brief Destroy the Fixed DepthwiseConv Op object
     * 
     */
    virtual ~FixedDepthwiseConvOp();

    void forward_on_cpu(std::vector<Tensor<FixedType>>& output, std::vector<Tensor<FixedType>>& input);
    void forward_on_dsp(std::vector<Tensor<FixedType>>& output, std::vector<Tensor<FixedType>>& input);
    void forward_on_gpu(std::vector<Tensor<FixedType>>& output, std::vector<Tensor<FixedType>>& input);

    /**
     * @brief initialize FixedDepthwiseConvOp
     * 
     * @param buf 
     * @param in_size 
     */
    virtual bool init(char* buf, int in_size);
    /**
     * @brief Set the Shape object
     * 
     * @param shape 
     * @return int 
     */
    virtual int setShape(std::vector<int64_t> shape);

private:
    std::vector<int> m_dilations;
    std::vector<int> m_strides;
    std::vector<int> m_paddings;
    Tensor<FixedType> m_filter; 
    Padding m_padding;
    Tensor<FixedType> m_bias;
};
} // namespace nano
    
}
#endif