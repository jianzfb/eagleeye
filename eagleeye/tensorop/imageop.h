#ifndef _EAGLEEYE_TENSOROP_H_
#define _EAGLEEYE_TENSOROP_H_

#include "eagleeye/basic/Tensor.h"
namespace eagleeye{
/**
 * 
 */
Tensor<float> crop_and_resize(Tensor<float> image, Tensor<float> bbox, std::vector<int> bbox_ind, std::vector<int> crop_size);	
}

#endif