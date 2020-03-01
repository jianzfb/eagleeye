#ifndef _EAGLEEYE_FIXEDCONVOP_H_
#define _EAGLEEYE_FIXEDCONVOP_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/FixedCNNOp.h"

namespace eagleeye{
class FixedConvOp:public FixedCNNOp{
public:
    /**
     * @brief Construct a new Fixed Conv Op object
     * 
     * @param input_data_num 
     * @param output_data_num 
     * @param op_name 
     */
    FixedConvOp(int input_data_num, int output_data_num, std::string op_name);
    
    /**
     * @brief Destroy the Fixed Conv Op object
     * 
     */
    virtual ~FixedConvOp();

    /**
     * @brief run fixed conv on cpu
     * 
     * @param output 
     * @param input 
     */
    void run_on_cpu(std::vector<Tensor<FixedType>>& output, std::vector<Tensor<FixedType>>& input);
    
    /**
     * @brief run fixed conv on gpu
     * 
     * @param output 
     * @param input 
     */
    void run_on_gpu(std::vector<Tensor<FixedType>>& output, std::vector<Tensor<FixedType>>& input);

    /**
     * @brief run fixed conv on dsp
     * 
     * @param output 
     * @param input 
     */
    void run_on_dsp(std::vector<Tensor<FixedType>>& output, std::vector<Tensor<FixedType>>& input);

    /**
     * @brief initialize FixedConvOp
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

protected:
    bool Forward_SingleGroup(FixedType *in_data, 
                             FixedType *out_data, 
                             FixedConvType *weight_data, 
                             float *weight_rate,
                             FixedBiasType *fixed_bias, 
                             int input_channel, 
                             int input_width,
                             int input_height,
                             int output_channel, 
                             int output_width, 
                             int output_height);

private:
	int kernel_size_;
	int half_kernel_size_;
    int pad_size_;
	int w_stride_;
    int h_stride_;
	int dilation_;
	int bias_size_;
	int bias_term_;
	int group_;
	int bool_16Bit_32Bit_Add_;//0 for 32bit add, 1 for 16bit add
	float *weight_group_rate_;
	FixedConvType *fixed_weight_;
	FixedBiasType *fixed_bias_value_;

    int input_channel_;
	int output_channel_;
};

bool RunConvForward_KernelLoop(FixedConvType *expand_input_data, int input_channel, int expand_width, int expand_height, FixedConvType *fixed_weight,
                               FixedType *net_output_data, int output_channel, int output_width, int output_height, int kernel_size, int pad_size, int w_stride_, int h_stride_,
                               FixedBiasType *fixed_bias_value, int *adjust_div_value, int norm_half_add_value, int bool_16Bit_32Bit_Add);

bool RunConvForward_ImageLoop(FixedConvType *expand_input_data, int input_channel, int expand_width, int expand_height, FixedConvType *fixed_weight,
                              FixedType *net_output_data, int output_channel, int output_width, int output_height, int kernel_size, int pad_size, int w_stride_, int h_stride_,
                              FixedBiasType *fixed_bias_value, int *adjust_div_value, int norm_half_add_value, int bool_16Bit_32Bit_Add);
}
#endif