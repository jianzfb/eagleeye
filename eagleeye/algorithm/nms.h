#ifndef _EAGLEEYE_NMS_H_
#define _EAGLEEYE_NMS_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/MatrixMath.h"
#include <vector>
namespace eagleeye{
/**
 * @brief weight non maximum suppression
 * 
 * @param dets 
 * @param thresh 
 * @param prob_thresh 
 * @param final_boxes 
 * @return std::vector<unsigned int> 
 */
std::vector<unsigned int> wnms(Matrix<float> dets,
							   float thresh, 
							   float prob_thresh, 
							   std::vector<std::vector<float>>& final_boxes);  

/**
 * @brief non maximum suppresion
 * 
 * @param dets 
 * @param thresh 
 * @param prob_thresh 
 * @return std::vector<unsigned int> 
 */
std::vector<unsigned int> nms(Matrix<float> dets, float thresh, float prob_thresh);  
}
#endif