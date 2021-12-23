#ifndef _EAGLEEYE_IMAGEPROCESS_H_
#define _EAGLEEYE_IMAGEPROCESS_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/MatrixMath.h"
#include <vector>

namespace eagleeye
{
#ifdef EAGLEEYE_NEON_OPTIMIZATION    
Matrix<float> medianFilterK5(Matrix<float> data);
#endif    
} // namespace eagleeye

#endif