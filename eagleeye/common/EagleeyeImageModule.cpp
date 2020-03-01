#include "eagleeye/common/EagleeyeImageModule.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/MetaOperation.h"
#include "eagleeye/basic/Variable.h"
#include "eagleeye/common/EagleeyeVisTool.h"

namespace eagleeye
{
void drawMaskOnRGBA(unsigned char* rgba, unsigned char* mask, int h, int w, int label_num){
    Matrix<Array<unsigned char,4>> rgba_mat(h,w,rgba);
    Matrix<unsigned char> mask_mat(h,w,mask);

    drawMaskOnImage(rgba_mat, mask_mat, label_num);
}
} // namespace eagleeye
