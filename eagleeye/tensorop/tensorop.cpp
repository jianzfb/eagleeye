#include "eagleeye/tensorop/tensorop.h"
namespace eagleeye{
// Tensor<float> resize(Tensor<float> tensor, int after_r, int after_c, InterpMethod interp_method){
// 	assert(interp_method==BILINEAR_INTERPOLATION);
// 	assert(tensor.ndim == 4);
// 	std::vector<int64_t> shape = tensor.shape();

// 	int64_t batch_num = shape[0];
// 	Tensor<float> resized_tensor(std::vector<int64_t>{shape[0], after_r, after_c, shape[3]});

// 	int64_t offset = shape[1]*shape[2]*shape[3];
// 	float* tensor_ptr = tensor.dataptr();
// 	float* resized_tensor_ptr = resized_tensor.dataptr();
// 	for(int b_i=0; b_i<batch_num; ++b_i){
// 		float* b_tensor_ptr = tensor_ptr+b_i*offset;

// 		for(int i=0; i<after_r; ++i){
// 			for(int j=0; j<after_c; ++j){
// 				> 
// 			}
// 		}
// 	}

// 	return resized_tensor;
// }	
}