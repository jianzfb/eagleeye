#ifndef _EAGLEEYE_IMAGEPROCESS_H_
#define _EAGLEEYE_IMAGEPROCESS_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/MatrixMath.h"
#include <vector>

namespace eagleeye
{
#if defined (__ARM_NEON) || defined (__ARM_NEON__)    
Matrix<float> medianFilterK5(Matrix<float> data);
#endif    
} // namespace eagleeye

#endif