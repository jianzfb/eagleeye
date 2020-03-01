#ifndef _EAGLEEYE_LLSP_H_
#define _EAGLEEYE_LLSP_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/algorithm/qr_solver.h"
#include "eagleeye/basic/Matrix.h"

namespace eagleeye{
/**
 * @brief Ax = b
 * 
 * @param a 
 * @param b 
 * @param y 
 * @param tol 
 * @param rsd
 * @return Matrix<float> x
 */
Matrix<float> linearLSP(Matrix<float> a, Matrix<float> b,float tol, Matrix<float>& rsd);
}
#endif