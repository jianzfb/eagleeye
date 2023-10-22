#ifndef _EAGLEEYE_BGRA_ROTATE_HWC_H_
#define _EAGLEEYE_BGRA_ROTATE_HWC_H_
#include "eagleeye/common/EagleeyeMacro.h"

namespace eagleeye
{
void bgra_rotate_hwc(const unsigned char* src, unsigned char* dst, int w_in, int h_in, int angle);
} // namespace eagleeye

#endif