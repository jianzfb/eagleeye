#ifndef _EAGLEEYE_QUANTIZATION_H_
#define _EAGLEEYE_QUANTIZATION_H_
#include "eagleeye/common/EagleeyeMacro.h"
namespace eagleeye{
namespace nano
{
typedef short           FixedType;
typedef signed char     FixedOpType;
typedef unsigned char   FixedUOpType;
// typedef int FixedCalType;
typedef signed char FixedConvType;
// typedef int FixedCalConvType;
typedef signed short FixedBiasType;

#define QUANTIZER_MAX_NORM_VALUE          127
#define QUANTIZER_DATA_NORM_VALUE         32768
#define QUANTIZER_HALF_DATA_NORM_VALUE    16384
#define QUANTIZER_DATA_NORM_MOVE          15

/**
 * @brief quant
 * 
 */
void quant_sym_32f_16b(float* data_in, 
                        FixedType* data_out,
                        int H, 
                        int W, 
                        int channel, 
                        float* scale);
/**
 * @brief quantize 16bit to 8bit
 * 
 * @param data_in 
 * @param data_out 
 * @param H 
 * @param W 
 * @param channel 
 */
void quant_sym_16b_8b(FixedType* data_in, 
                    FixedOpType* data_out, 
                    int H,
                    int W,
                    int channel);
                   

// void dequant_sym_8_16(signed char* indata, short* outdata, int size, int scale);
} // namespace nano
    
}
#endif