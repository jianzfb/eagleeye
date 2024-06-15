#include "eagleeye/engine/nano/util/quantization.h"
#if defined (__ARM_NEON) || defined (__ARM_NEON__)
#include <arm_neon.h>
#endif


namespace eagleeye
{
namespace nano
{
void quant_sym_32f_16b(float* data_in, 
                        FixedType* data_out,
                        int H, 
                        int W, 
                        int channel, 
                        float* scale){
    // format 1xCxHxW
    int offset = 0;
    int HW = H*W;
    int search_start = 0;
	int search_end = HW;

    for(int c=0; c<channel; ++c){
        offset = c * HW;
        // int adjust_value = int(QUANTIZER_MAX_NORM_VALUE / QUANTIZER_DATA_NORM_VALUE * QUANTIZER_DATA_NORM_VALUE + 0.5f);
        int adjust_value = 1.0f/scale[c] * QUANTIZER_DATA_NORM_VALUE;
        float *ptr_in = data_in + offset;
        FixedType *ptr_out = data_out + offset;
        for (int i=0; i < search_end; ++i){
            ptr_out[i] = (FixedType)(ptr_in[i] * adjust_value);
        }
    }
}

void quant_sym_16b_8b(FixedType* data_in, 
                    FixedOpType* data_out, 
                    int H,
                    int W,
                    int channel){
    // format 1xCxHxW
    int offset = 0;
    int HW = H*W;
    int search_start = 0;
	int search_end = HW;
    int search_8_end = (search_end - search_start) / 8 * 8 + search_start;

    for(int c=0; c<channel; ++c){
        offset = c * HW;
        // int adjust_value = int(QUANTIZER_MAX_NORM_VALUE / QUANTIZER_DATA_NORM_VALUE * QUANTIZER_DATA_NORM_VALUE + 0.5f);
        int adjust_value = QUANTIZER_MAX_NORM_VALUE;
        FixedType *ptr_in = data_in + offset;
        FixedOpType *ptr_out = data_out + offset;

#if defined (__ARM_NEON) || defined (__ARM_NEON__)
        int32x4_t mul_neon;
        int32x4_t half_value_neon = vdupq_n_u32(QUANTIZER_HALF_DATA_NORM_VALUE);
        int16x8_t in_neon, out_neon;
        int8x8_t out_neon_s8;
        int16x4_t in_neon_0, in_neon_1;
        int16x4_t out_neon_0, out_neon_1;
        int i = search_start;
        for (i = search_start; i < search_8_end; i += 8){
            in_neon = vld1q_s16(ptr_in);
            in_neon_0 = vget_low_s16(in_neon);
            in_neon_1 = vget_high_s16(in_neon);
            mul_neon = vmull_n_s16(in_neon_0, adjust_value);
            mul_neon = vaddq_s32(mul_neon, half_value_neon);
            out_neon_0 = vshrn_n_s32(mul_neon, QUANTIZER_DATA_NORM_MOVE);
            mul_neon = vmull_n_s16(in_neon_1, adjust_value);
            mul_neon = vaddq_s32(mul_neon, half_value_neon);
            out_neon_1 = vshrn_n_s32(mul_neon, QUANTIZER_DATA_NORM_MOVE);
            out_neon = vcombine_s16(out_neon_0, out_neon_1);
            out_neon_s8 = vmovn_s16(out_neon);
            vst1_s8(ptr_out, out_neon_s8);
            ptr_in += 8;
            ptr_out += 8;
        }

        for (; i < search_end; ++i){
            ptr_out[i] = ((ptr_in[i] * adjust_value + QUANTIZER_HALF_DATA_NORM_VALUE) >> QUANTIZER_DATA_NORM_MOVE);
        }
#endif
    }
}



// void dequant_sym_8_16(signed char* data_in, 
//                       short* data_out,                     
//                       int H,
//                       int W,
//                       int channel, 
//                       float* scale,
//                       int data_norm_value,
//                       int data_norm_move,
//                       int norm_half_add_value){
//     // format 1xCxHxW
//     int offset = 0;
//     int HW = H*W;
//     int search_start = 0;
// 	int search_end = HW;
//     int search_8_end = (search_end - search_start) / 8 * 8 + search_start;

//     for(int c=0; c<channel; ++c){
//         offset = c * HW;
//         int adjust_value = int(scale[c]* data_norm_value + 0.5f);
//         signed char *ptr_in = data_in + offset;
//         short *ptr_out = data_out + offset;

// #if defined (__ARM_NEON) || defined (__ARM_NEON__)
//         if(adjust_multi_value < data_norm_value){
//             int32x4_t mul_neon;
//             int32x4_t half_value_neon = vdupq_n_s32(norm_half_add_value);
//             int16x8_t in_neon, out_neon;
//             int8x8_t out_neon_s8;
//             int16x4_t in_neon_0, in_neon_1;
//             int16x4_t out_neon_0, out_neon_1;
//             int i = search_start;
//             for (i = search_start; i < search_8_end; i += 8){
//                 in_neon = vld1q_s16(ptr_in);
//                 in_neon_0 = vget_low_s16(in_neon);
//                 in_neon_1 = vget_high_s16(in_neon);
//                 mul_neon = vmull_n_s16(in_neon_0, adjust_value);
//                 mul_neon = vaddq_s32(mul_neon, half_value_neon);
//                 out_neon_0 = vshrn_n_s32(mul_neon, data_norm_move);
//                 mul_neon = vmull_n_s16(in_neon_1, adjust_value);
//                 mul_neon = vaddq_s32(mul_neon, half_value_neon);
//                 out_neon_1 = vshrn_n_s32(mul_neon, data_norm_move);
//                 out_neon = vcombine_s16(out_neon_0, out_neon_1);
//                 out_neon_s8 = vmovn_s16(out_neon);
//                 vst1_s8(ptr_out, out_neon_s8);
//                 ptr_in += 8;
//                 ptr_out += 8;
//             }

//             for (; i < search_end; ++i){
//                 ptr_out[i] = ((ptr_in[i] * adjust_value + norm_half_add_value) >> data_norm_move);
//             }
//         }
//         else{
//             for (i=0; i < search_end; ++i){
//                 ptr_out[i] = ((ptr_in[i] * adjust_value + norm_half_add_value) >> data_norm_move);
//             }
//         }
// #endif
//     }
// }
} // namespace nano
    
} // namespace eagleeye
