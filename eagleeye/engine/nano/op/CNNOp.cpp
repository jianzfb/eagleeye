#include "eagleeye/engine/nano/op/FixedCNNOp.h"

namespace eagleeye
{
namespace nano{	
FixedCNNOp::FixedCNNOp(int input_data_num, int output_data_num, int layer_flag, std::string op_name):
	input_data_num_(),
	output_data_num_(output_data_num),
	m_op_name(op_name),
	layer_flag_(layer_flag){
}

FixedCNNOp::~FixedCNNOp(){

}

void FixedCNNOp::MakeExpandData_8Bit(FixedConvType *dst_data, FixedConvType *src_data, int channel, int src_wd, int src_ht, int pad_w_left, int pad_h_top, int pad_w_right, int pad_h_bottom, int const_value)
{
	int i = 0, j = 0, k = 0;
	FixedConvType *dst_ptr = dst_data;
	FixedConvType *src_ptr = src_data;
	int dst_wd = src_wd + pad_w_left + pad_w_right;
	int dst_ht = src_ht + pad_h_top + pad_h_bottom;
	for (i = 0; i < channel; ++i)
	{
		for (j = 0; j < pad_h_top; ++j)
		{
			memset(dst_ptr, const_value, sizeof(FixedConvType)*dst_wd);
			dst_ptr += dst_wd;
		}
		for (; j < src_ht + pad_h_top; ++j)
		{
			for (k = 0; k < pad_w_left; ++k)
			{
				dst_ptr[k] = const_value;
			}
			for (k = 0; k < pad_w_right; ++k)
				dst_ptr[k + pad_w_left + src_wd] = const_value;
			memcpy(dst_ptr + pad_w_left, src_ptr, sizeof(FixedConvType)*src_wd);

			src_ptr += src_wd;
			dst_ptr += dst_wd;
		}
		for (; j < dst_ht; ++j)
		{
			memset(dst_ptr, const_value, sizeof(FixedConvType)*dst_wd);
			dst_ptr += dst_wd;
		}
	}
}    

int FixedCNNOp::getInputNum(){
    return m_input_shape.size();
}

std::vector<int64_t> FixedCNNOp::getInputShape(int index){
    return this->m_input_shape[index];
}

int FixedCNNOp::getOutputNum(){
    return m_output_shape.size();
}

std::vector<int64_t> FixedCNNOp::getOutputShape(int index){
    return this->m_output_shape[index];
}

std::string FixedCNNOp::getLayerName(){
    return m_op_name;
}

// void FixedCNNOp::getOuput(Tensor<float>& tensor, Tensor<FixedType>& fixed_tensor){
// 	// 1.step get ptr
// 	FixedType* fixed_data = (FixedType*)fixed_tensor.cpu();
// 	float* data = (float*)tensor.cpu();
// 	// 2.step inverse to float
	
// }
}
} // namespace eagleeye
