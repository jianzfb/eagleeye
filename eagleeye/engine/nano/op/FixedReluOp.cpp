#include "eagleeye/engine/nano/op/FixedReluOp.h"
#include "eagleeye/common/EagleeyeMacro.h"
#ifdef EAGLEEYE_NEON_OPTIMIZATION
#include <arm_neon.h>
#endif

namespace eagleeye
{
namespace nano
{
FixedReluOp::FixedReluOp(int input_data_num, int output_data_num, std::string op_name)
             :FixedCNNOp(input_data_num, output_data_num, FIXED_RELU, op_name){


}

FixedReluOp::~FixedReluOp(){

}

void FixedReluOp::foward_on_cpu(std::vector<Tensor<FixedType>>& output, std::vector<Tensor<FixedType>>& input){
    int input_data_size = input[0].size();
    int output_data_size = output[0].size();

    FixedType* net_input = (FixedType*)input[0].cpu();
    FixedType* net_output = (FixedType*)output[0].cpu();

    // input[0].dim(0) == 1
    int C = input[0].dim(1);
    int H = input[0].dim(2);
    int W = input[0].dim(3);   
    int total_size = C*H*W; 
#ifdef EAGLEEYE_NEON_OPTIMIZATION
        int i = 0;
    	int search_x_end = total_size / 8 * 8;
		int16x8_t value;
		int16x8_t zero_value = vdupq_n_s16(0);
		for(i = 0;i < search_x_end; i += 8)
		{
			value = vld1q_s16(net_input);
			value = vmaxq_s16(value, zero_value);
			vst1q_s16(net_output, value);
			net_output += 8;
			net_input += 8;
		}
		for(;i < total_size; ++i)
		{
			*net_output = eagleeye_max(*net_input, 0);
			net_output++;
			net_input ++;
		}
#endif
}

void FixedReluOp::foward_on_gpu(std::vector<Tensor<FixedType>>& output, std::vector<Tensor<FixedType>>& input){

}

void FixedReluOp::foward_on_dsp(std::vector<Tensor<FixedType>>& output, std::vector<Tensor<FixedType>>& input){

}

bool FixedReluOp::init(char *buf, int in_size){
    return true;
}

int FixedReluOp::setShape(std::vector<int64_t> shape){
    return 0;
}
} // namespace nano
    
} // namespace eagleeye
