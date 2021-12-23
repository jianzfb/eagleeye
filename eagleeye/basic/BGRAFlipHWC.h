#include "eagleeye/common/EagleeyeMacro.h"

namespace eagleeye
{
//x: flip_num = 1 y: flip_num = -1 xy: flip_num = 0;
void bgra_flip_hwc(const unsigned char* src, unsigned char* dst, int w_in, int h_in, int flip_num);
} // namespace eagleeye
