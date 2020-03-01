#ifndef _EAGLEEYE_EAGLEEYEIMAGEMODULE_H_
#define _EAGLEEYE_EAGLEEYEIMAGEMODULE_H_
#include "eagleeye/common/EagleeyeMacro.h"
namespace eagleeye{
/**
 * @brief draw image and mask
 * 
 * @param rgba 
 * @param mask 
 * @param h 
 * @param w 
 * @param num 
 * @param channel
 */
void drawMaskOnRGBA(unsigned char* rgba, unsigned char* mask, int h, int w, int label_num);

}
#endif