#ifndef _EAGLEEYE_X86_INTERPOLATE_H_
#define _EAGLEEYE_X86_INTERPOLATE_H_
#include <string>
#include <vector>

namespace eagleeye{
namespace math{
namespace x86{
void bilinear_rgb_8u_3d_interp(
                    const unsigned char * pSrcImg, 
                    unsigned char * pDesImg, 
                    int srcWidth, 
                    int srcHeight, 
                    int srcOffsetX, 
                    int srcOffsetY, 
                    int srcStride, 
                    int desWidth, 
                    int desHeight);

void bilinear_gray_8u_1d_interp(
                    const unsigned char * pSrcImg, 
                    unsigned char * pDesImg, 
                    int srcWidth, 
                    int srcHeight, 
                    int srcOffsetX, 
                    int srcOffsetY, 
                    int srcStride, 
                    int desWidth, 
                    int desHeight);
}    
}
}
#endif