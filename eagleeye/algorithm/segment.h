#ifndef _EAGLEEYE_SEGMENT_H_
#define _EAGLEEYE_SEGMENT_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/Array.h"
#include <algorithm>
#include <map>

namespace eagleeye{
/**
 * @brief Returns a color image representing the segmentation.
 * 
 * @param _src3f image to segment.
 * @param pImgInd index of each components, [0, colors.size() -1]
 * @param sigma to smooth the image.
 * @param c constant for threshold function.
 * @param min_size minimum component size (enforced by post-processing stage).
 * @return int 
 */
int segmentImage(Matrix<Array<float,3>> &_src3f, Matrix<int> &pImgInd, float sigma, float c, int min_size);
}
#endif