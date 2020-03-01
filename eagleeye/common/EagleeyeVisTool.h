#ifndef _EAGLEEYE_EAGLEEYEVISTOOL_H_
#define _EAGLEEYE_EAGLEEYEVISTOOL_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/Array.h"
namespace eagleeye{

/**
 * @brief draw Mask on RGB image
 * 
 * @param rgb 
 * @param mask 
 * @param label_num 
 */
void drawMaskOnImage(Matrix<Array<unsigned char,3>>& rgb, Matrix<unsigned char> mask, int label_num);

/**
 * @brief draw Mask on RGBA image
 * 
 * @param rgb 
 * @param mask 
 * @param label_num 
 */
void drawMaskOnImage(Matrix<Array<unsigned char,4>>& rgb, Matrix<unsigned char> mask, int label_num);

}
#endif