#include "eagleeye/engine/nano/op/FixedCNNOp.h"
#include "eagleeye/engine/nano/op/FixedConvOp.h"
#include <math.h>

#ifdef EAGLEEYE_NEON_OPTIMIZATION
#include <arm_neon.h>
#endif

namespace eagleeye
{
void GetMultiOutputChannelResult3x3_2_16Bit(FixedConvType *fixed_weight, FixedConvType *expand_input_data, int input_channel, int expand_width, int expand_height, int cur_channel,
                                            int output_height, FixedType *temp_output_data_0, FixedType *temp_output_data_1, int pad_size,
                                            FixedBiasType *fixed_bias_value, int *adjust_div_value, int norm_half_add_value);

void GetOneOutputChannelResult3x3_16Bit(FixedConvType *weight_ptr, FixedConvType *expand_input_data, int input_channel, int expand_width,
                                        int expand_height, int output_height, FixedType *temp_output_data, int pad_size,
                                        FixedBiasType bias_value, int adjust_div_value, int norm_half_add_value);

void GetMultiOutputChannelResult1x1_4_16Bit(FixedConvType *fixed_weight, FixedConvType *expand_input_data, int input_channel, int expand_width, int expand_height, int cur_channel,
                                            int output_height, FixedType *temp_output_data_0, FixedType *temp_output_data_1, FixedType *temp_output_data_2, FixedType *temp_output_data_3, int pad_size,
                                            FixedBiasType *fixed_bias_value, int *adjust_div_value, int norm_half_add_value);

void GetOneOutputChannelResult1x1_16Bit(FixedConvType *weight_ptr, FixedConvType *expand_input_data, int input_channel, int expand_width,
                                        int expand_height, int output_height, FixedType *temp_output_data, int pad_size,
                                        FixedBiasType bias_value, int adjust_div_value, int norm_half_add_value);

void ConvMatrixMulti_16Bit(FixedConvType *fixed_weight, int row1, int col1, FixedConvType *data_col, int row2, int col2, FixedType *net_output_data,
                           int *adjust_div_value, int norm_half_add_value, FixedBiasType *fixed_bias_value);

bool GetOneOutputChannelResult_Stride2_16Bit(FixedConvType *weight_ptr, FixedConvType **input_ptr, int input_channel, int kernel_size_sq, int w_stride_, int h_stride_,
                                             int expand_width, int expand_height, int output_width, int output_height, FixedType *p_in_out_buf, int pad_size, FixedBiasType bias_value,
                                             int adjust_div_value, int norm_half_add_value);
void GetMultiOutputChannelResult3x3_Stride2_2_16Bit(FixedConvType *fixed_weight, FixedConvType *expand_input_data, int input_channel, int expand_width,
                                                    int expand_height, int cur_channel, int output_width, int output_height, FixedType *output_data, int pad_size,
                                                    FixedBiasType *fixed_bias_value, int *adjust_div_value, int norm_half_add_value);
void GetOneOutputChannelResult3x3_Stride2_16Bit(FixedConvType *weight_ptr, FixedConvType *expand_input_data, int input_channel, int expand_width,
                                                int expand_height, int output_width, int output_height, FixedType *output_data, int pad_size,
                                                FixedBiasType bias_value, int adjust_div_value, int norm_half_add_value);

void GetMultiOutputChannelResult3x3_2_32Bit(FixedConvType *fixed_weight, FixedConvType *expand_input_data, int input_channel, int expand_width, int expand_height, int cur_channel,
                                            int output_height, FixedType *temp_output_data_0, FixedType *temp_output_data_1, int pad_size,
                                            FixedBiasType *fixed_bias_value, int *adjust_div_value, int norm_half_add_value)
{
    int i = 0, j = 0, k = 0, m = 0;
    int search_img_start = 0;
    int search_img_end = output_height*expand_width - 2 * pad_size;
    int expand_size = expand_height*expand_width;
    int search_8_end = (search_img_end - search_img_start) / 8 * 8 + search_img_start;
    int new_kernel_size = input_channel * 2 * 9;
    FixedConvType *new_fixed_weight = (FixedConvType *)malloc(sizeof(FixedConvType)*new_kernel_size);
    const int data_norm_move_const = CNN_CONV_DATA_NORM_MOVE;
    int adjust_div_value_0 = adjust_div_value[cur_channel];
    int adjust_div_value_1 = adjust_div_value[cur_channel + 1];

    if (new_fixed_weight == NULL)
        return;
    const FixedConvType* k0 = fixed_weight + cur_channel*input_channel * 9;
    const FixedConvType* k1 = fixed_weight + (cur_channel + 1)*input_channel * 9;
    m = 0;
    for (i = 0; i < input_channel; ++i)
    {
        for (j = 0; j < 9; ++j)
        {
            new_fixed_weight[m] = k0[j];
            m++;
            new_fixed_weight[m] = k1[j];
            m++;
        }
        k0 += 9;
        k1 += 9;
    }

    FixedType *out_buf_0 = temp_output_data_0;
    FixedType *out_buf_1 = temp_output_data_1;

    j = search_img_start;

#ifdef EAGLEEYE_NEON_OPTIMIZATION
    int8x8_t *weight_ptr_neon;
    int8x8_t in_value_neon;
    int16x8_t mul_value_0, mul_value_1, out_value_neon_16Bit;
    int16x4_t out_value_neon_16Bit_0, out_value_neon_16Bit_1;
    int32x4_t out_value_neon0_0, out_value_neon0_1, out_value_neon1_0, out_value_neon1_1;
    int16x8_t fixed_bias_value_neon_0, fixed_bias_value_neon_1;
    int32x4_t norm_half_add_value_neon;
    int input_channel_2 = input_channel/2*2;

    norm_half_add_value_neon = vdupq_n_s32(norm_half_add_value);
    fixed_bias_value_neon_0 = vdupq_n_s16(fixed_bias_value[cur_channel]);
    fixed_bias_value_neon_1 = vdupq_n_s16(fixed_bias_value[cur_channel + 1]);

    weight_ptr_neon = (int8x8_t *)malloc(sizeof(int8x8_t)*new_kernel_size);
    if (weight_ptr_neon == NULL)
        return;
    for (j = 0; j < new_kernel_size; ++j)
        weight_ptr_neon[j] = vdup_n_s8(new_fixed_weight[j]);

    for (j = 0; j < search_8_end; j += 8)
    {
        FixedConvType *ptr_0 = expand_input_data + j;
        FixedConvType *ptr_1 = expand_input_data + j + expand_width;
        FixedConvType *ptr_2 = expand_input_data + j + 2 * expand_width;

        out_value_neon0_0 = vmovq_n_s32(0);
        out_value_neon0_1 = vmovq_n_s32(0);
        out_value_neon1_0 = vmovq_n_s32(0);
        out_value_neon1_1 = vmovq_n_s32(0);

        int8x8_t *weigth_neon_ptr_0 = weight_ptr_neon;
        for (m = 0; m < input_channel_2; m += 2)
        {
            FixedConvType *ptr = ptr_0;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmull_s8(in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            mul_value_1 = vmull_s8(in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmlal_s8(mul_value_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            mul_value_1 = vmlal_s8(mul_value_1, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
            out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
            out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
            out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmull_s8(in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            mul_value_1 = vmull_s8(in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;

            ptr = ptr_1;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmlal_s8(mul_value_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            mul_value_1 = vmlal_s8(mul_value_1, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
            out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
            out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
            out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmull_s8(in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            mul_value_1 = vmull_s8(in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmlal_s8(mul_value_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            mul_value_1 = vmlal_s8(mul_value_1, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
            out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
            out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
            out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));

            ptr = ptr_2;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmull_s8(in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            mul_value_1 = vmull_s8(in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmlal_s8(mul_value_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            mul_value_1 = vmlal_s8(mul_value_1, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
            out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
            out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
            out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmull_s8(in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            mul_value_1 = vmull_s8(in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;

            ptr_0 += expand_size;
            ptr_1 += expand_size;
            ptr_2 += expand_size;

            ptr = ptr_0;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmlal_s8(mul_value_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            mul_value_1 = vmlal_s8(mul_value_1, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
            out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
            out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
            out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmull_s8(in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            mul_value_1 = vmull_s8(in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmlal_s8(mul_value_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            mul_value_1 = vmlal_s8(mul_value_1, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
            out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
            out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
            out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));

            ptr = ptr_1;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmull_s8(in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            mul_value_1 = vmull_s8(in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmlal_s8(mul_value_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            mul_value_1 = vmlal_s8(mul_value_1, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
            out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
            out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
            out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmull_s8(in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            mul_value_1 = vmull_s8(in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;

            ptr = ptr_2;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmlal_s8(mul_value_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            mul_value_1 = vmlal_s8(mul_value_1, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
            out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
            out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
            out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmull_s8(in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            mul_value_1 = vmull_s8(in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmlal_s8(mul_value_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            mul_value_1 = vmlal_s8(mul_value_1, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
            out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
            out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
            out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));

            ptr_0 += expand_size;
            ptr_1 += expand_size;
            ptr_2 += expand_size;
        }
        for (; m < input_channel; m ++)
        {
            FixedConvType *ptr = ptr_0;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmull_s8(in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            mul_value_1 = vmull_s8(in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmlal_s8(mul_value_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            mul_value_1 = vmlal_s8(mul_value_1, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
            out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
            out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
            out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmull_s8(in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            mul_value_1 = vmull_s8(in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;

            ptr = ptr_1;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmlal_s8(mul_value_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            mul_value_1 = vmlal_s8(mul_value_1, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
            out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
            out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
            out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmull_s8(in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            mul_value_1 = vmull_s8(in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmlal_s8(mul_value_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            mul_value_1 = vmlal_s8(mul_value_1, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
            out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
            out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
            out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));

            ptr = ptr_2;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmull_s8(in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            mul_value_1 = vmull_s8(in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmlal_s8(mul_value_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            mul_value_1 = vmlal_s8(mul_value_1, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
            out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
            out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
            out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmull_s8(in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            mul_value_1 = vmull_s8(in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
            out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
            out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
            out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));

            ptr_0 += expand_size;
            ptr_1 += expand_size;
            ptr_2 += expand_size;
        }
        out_value_neon0_0 = vmulq_n_s32(out_value_neon0_0, adjust_div_value_0);
        out_value_neon0_1 = vmulq_n_s32(out_value_neon0_1, adjust_div_value_0);
        out_value_neon0_0 = vaddq_s32(out_value_neon0_0, norm_half_add_value_neon);
        out_value_neon0_1 = vaddq_s32(out_value_neon0_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(out_value_neon0_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(out_value_neon0_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_0);
        vst1q_s16(out_buf_0 + j, out_value_neon_16Bit);

        out_value_neon1_0 = vmulq_n_s32(out_value_neon1_0, adjust_div_value_1);
        out_value_neon1_1 = vmulq_n_s32(out_value_neon1_1, adjust_div_value_1);
        out_value_neon1_0 = vaddq_s32(out_value_neon1_0, norm_half_add_value_neon);
        out_value_neon1_1 = vaddq_s32(out_value_neon1_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(out_value_neon1_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(out_value_neon1_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_1);
        vst1q_s16(out_buf_1 + j, out_value_neon_16Bit);
    }

    if (j < search_img_end&&search_img_end - search_img_start >= 8)
    {
        j = search_img_end - 8;
        FixedConvType *ptr_0 = expand_input_data + j;
        FixedConvType *ptr_1 = expand_input_data + j + expand_width;
        FixedConvType *ptr_2 = expand_input_data + j + 2 * expand_width;

        out_value_neon0_0 = vmovq_n_s32(0);
        out_value_neon0_1 = vmovq_n_s32(0);
        out_value_neon1_0 = vmovq_n_s32(0);
        out_value_neon1_1 = vmovq_n_s32(0);

        int8x8_t *weigth_neon_ptr_0 = weight_ptr_neon;
        for (m = 0; m < input_channel; m ++)
        {
            FixedConvType *ptr = ptr_0;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmull_s8(in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            mul_value_1 = vmull_s8(in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmlal_s8(mul_value_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            mul_value_1 = vmlal_s8(mul_value_1, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
            out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
            out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
            out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmull_s8(in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            mul_value_1 = vmull_s8(in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;

            ptr = ptr_1;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmlal_s8(mul_value_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            mul_value_1 = vmlal_s8(mul_value_1, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
            out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
            out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
            out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmull_s8(in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            mul_value_1 = vmull_s8(in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmlal_s8(mul_value_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            mul_value_1 = vmlal_s8(mul_value_1, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
            out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
            out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
            out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));

            ptr = ptr_2;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmull_s8(in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            mul_value_1 = vmull_s8(in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmlal_s8(mul_value_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            mul_value_1 = vmlal_s8(mul_value_1, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
            out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
            out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
            out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value_0 = vmull_s8(in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            mul_value_1 = vmull_s8(in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
            out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
            out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
            out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));

            ptr_0 += expand_size;
            ptr_1 += expand_size;
            ptr_2 += expand_size;
        }
        out_value_neon0_0 = vmulq_n_s32(out_value_neon0_0, adjust_div_value_0);
        out_value_neon0_1 = vmulq_n_s32(out_value_neon0_1, adjust_div_value_0);
        out_value_neon0_0 = vaddq_s32(out_value_neon0_0, norm_half_add_value_neon);
        out_value_neon0_1 = vaddq_s32(out_value_neon0_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(out_value_neon0_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(out_value_neon0_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_0);
        vst1q_s16(out_buf_0 + j, out_value_neon_16Bit);

        out_value_neon1_0 = vmulq_n_s32(out_value_neon1_0, adjust_div_value_1);
        out_value_neon1_1 = vmulq_n_s32(out_value_neon1_1, adjust_div_value_1);
        out_value_neon1_0 = vaddq_s32(out_value_neon1_0, norm_half_add_value_neon);
        out_value_neon1_1 = vaddq_s32(out_value_neon1_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(out_value_neon1_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(out_value_neon1_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_1);
        vst1q_s16(out_buf_1 + j, out_value_neon_16Bit);
    }
    else
#endif
    {
        for (; j < search_img_end; ++j)
        {
            int value_0 = 0;
            int value_1 = 0;
            FixedConvType *ptr_0 = expand_input_data + j;
            FixedConvType *ptr_1 = expand_input_data + j + expand_width;
            FixedConvType *ptr_2 = expand_input_data + j + 2 * expand_width;
            FixedConvType *weight_0 = new_fixed_weight;

            for (m = 0; m < input_channel; ++m)
            {
                FixedConvType *ptr = ptr_0;
                value_0 += (*ptr)*(*weight_0); weight_0++;
                value_1 += (*ptr)*(*weight_0); weight_0++;
                ptr++;
                value_0 += (*ptr)*(*weight_0); weight_0++;
                value_1 += (*ptr)*(*weight_0); weight_0++;
                ptr++;
                value_0 += (*ptr)*(*weight_0); weight_0++;
                value_1 += (*ptr)*(*weight_0); weight_0++;

                ptr = ptr_1;
                value_0 += (*ptr)*(*weight_0); weight_0++;
                value_1 += (*ptr)*(*weight_0); weight_0++;
                ptr++;
                value_0 += (*ptr)*(*weight_0); weight_0++;
                value_1 += (*ptr)*(*weight_0); weight_0++;
                ptr++;
                value_0 += (*ptr)*(*weight_0); weight_0++;
                value_1 += (*ptr)*(*weight_0); weight_0++;

                ptr = ptr_2;
                value_0 += (*ptr)*(*weight_0); weight_0++;
                value_1 += (*ptr)*(*weight_0); weight_0++;
                ptr++;
                value_0 += (*ptr)*(*weight_0); weight_0++;
                value_1 += (*ptr)*(*weight_0); weight_0++;
                ptr++;
                value_0 += (*ptr)*(*weight_0); weight_0++;
                value_1 += (*ptr)*(*weight_0); weight_0++;

                ptr_0 += expand_size;
                ptr_1 += expand_size;
                ptr_2 += expand_size;
            }

            out_buf_0[j] = ((value_0 * adjust_div_value_0 + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[cur_channel];
            out_buf_1[j] = ((value_1 * adjust_div_value_1 + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[cur_channel + 1];
        }
    }
    EAGLEEYE_SAFEFREE(new_fixed_weight);
#ifdef EAGLEEYE_NEON_OPTIMIZATION
    EAGLEEYE_SAFEFREE(weight_ptr_neon);
#endif
}

void GetOneOutputChannelResult3x3_32Bit(FixedConvType *weight_ptr, FixedConvType *expand_input_data, int input_channel, int expand_width,
                                        int expand_height, int output_height, FixedType *temp_output_data, int pad_size,
                                        FixedBiasType bias_value, int adjust_div_value, int norm_half_add_value)
{
    int i = 0, j = 0, m = 0;
    int search_img_start = 0;
    int search_img_end = output_height*expand_width - 2 * pad_size;
    int search_8_end = (search_img_end - search_img_start) / 8 * 8 + search_img_start;
    FixedType *p_out_buf = temp_output_data;
    int expand_size = expand_height*expand_width;
    const int data_norm_move_const = CNN_CONV_DATA_NORM_MOVE;

    j = search_img_start;

#ifdef EAGLEEYE_NEON_OPTIMIZATION
    int8x8_t *weight_ptr_neon;
    int loop_size = input_channel*9;

    weight_ptr_neon = (int8x8_t *)malloc(sizeof(int8x8_t)*loop_size);
    if (weight_ptr_neon == NULL)
        return;

    for (i = 0; i < loop_size; ++i)
        weight_ptr_neon[i] = vdup_n_s8(weight_ptr[i]);

    int8x8_t in_value_neon;
    int32x4_t out_value_neon_0, out_value_neon_1;
    int16x4_t out_value_neon_16Bit_0, out_value_neon_16Bit_1;
    int16x8_t mul_value, out_value_neon_16Bit, fixed_bias_value_neon;
    int32x4_t norm_half_add_value_neon;
    norm_half_add_value_neon = vdupq_n_s32(norm_half_add_value);
    fixed_bias_value_neon = vdupq_n_s16(bias_value);

    for (j = 0; j < search_8_end; j += 8)
    {
        FixedConvType *ptr_0 = expand_input_data + j;
        FixedConvType *ptr_1 = expand_input_data + j + expand_width;
        FixedConvType *ptr_2 = expand_input_data + j + 2 * expand_width;

        int8x8_t *weigth_neon_ptr = weight_ptr_neon;
        out_value_neon_0 = vdupq_n_s32(0);
        out_value_neon_1 = vdupq_n_s32(0);
        for (m = 0; m < input_channel; m++)
        {
            FixedConvType *ptr = ptr_0;
            in_value_neon = vld1_s8(ptr);
            mul_value = vmull_s8(in_value_neon, *weigth_neon_ptr);
            weigth_neon_ptr++;
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value = vmlal_s8(mul_value, in_value_neon, *weigth_neon_ptr);
            out_value_neon_0 = vaddw_s16(out_value_neon_0, vget_low_s16(mul_value));
            out_value_neon_1 = vaddw_s16(out_value_neon_1, vget_high_s16(mul_value));
            weigth_neon_ptr++;
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value = vmull_s8(in_value_neon, *weigth_neon_ptr);
            weigth_neon_ptr++;

            ptr = ptr_1;
            in_value_neon = vld1_s8(ptr);
            mul_value = vmlal_s8(mul_value, in_value_neon, *weigth_neon_ptr);
            out_value_neon_0 = vaddw_s16(out_value_neon_0, vget_low_s16(mul_value));
            out_value_neon_1 = vaddw_s16(out_value_neon_1, vget_high_s16(mul_value));
            weigth_neon_ptr++;
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value = vmull_s8(in_value_neon, *weigth_neon_ptr);
            weigth_neon_ptr++;
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value = vmlal_s8(mul_value, in_value_neon, *weigth_neon_ptr);
            out_value_neon_0 = vaddw_s16(out_value_neon_0, vget_low_s16(mul_value));
            out_value_neon_1 = vaddw_s16(out_value_neon_1, vget_high_s16(mul_value));
            weigth_neon_ptr++;

            ptr = ptr_2;
            in_value_neon = vld1_s8(ptr);
            mul_value = vmull_s8(in_value_neon, *weigth_neon_ptr);
            weigth_neon_ptr++;
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value = vmlal_s8(mul_value, in_value_neon, *weigth_neon_ptr);
            out_value_neon_0 = vaddw_s16(out_value_neon_0, vget_low_s16(mul_value));
            out_value_neon_1 = vaddw_s16(out_value_neon_1, vget_high_s16(mul_value));
            weigth_neon_ptr++;
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value = vmull_s8(in_value_neon, *weigth_neon_ptr);
            out_value_neon_0 = vaddw_s16(out_value_neon_0, vget_low_s16(mul_value));
            out_value_neon_1 = vaddw_s16(out_value_neon_1, vget_high_s16(mul_value));
            weigth_neon_ptr++;

            ptr_0 += expand_size;
            ptr_1 += expand_size;
            ptr_2 += expand_size;
        }

        out_value_neon_0 = vmulq_n_s32(out_value_neon_0, adjust_div_value);
        out_value_neon_1 = vmulq_n_s32(out_value_neon_1, adjust_div_value);
        out_value_neon_0 = vaddq_s32(out_value_neon_0, norm_half_add_value_neon);
        out_value_neon_1 = vaddq_s32(out_value_neon_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(out_value_neon_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(out_value_neon_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon);
        vst1q_s16(p_out_buf + j, out_value_neon_16Bit);
    }

    if (j < search_img_end&&search_img_end - search_img_start >= 8)
    {
        j = search_img_end - 8;

        FixedConvType *ptr_0 = expand_input_data + j;
        FixedConvType *ptr_1 = expand_input_data + j + expand_width;
        FixedConvType *ptr_2 = expand_input_data + j + 2 * expand_width;
        int8x8_t *weigth_neon_ptr = weight_ptr_neon;

        out_value_neon_0 = vdupq_n_s32(0);
        out_value_neon_1 = vdupq_n_s32(0);
        for (m = 0; m < input_channel; ++m)
        {
            FixedConvType *ptr = ptr_0;
            in_value_neon = vld1_s8(ptr);
            mul_value = vmull_s8(in_value_neon, *weigth_neon_ptr);
            weigth_neon_ptr++;
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value = vmlal_s8(mul_value, in_value_neon, *weigth_neon_ptr);
            out_value_neon_0 = vaddw_s16(out_value_neon_0, vget_low_s16(mul_value));
            out_value_neon_1 = vaddw_s16(out_value_neon_1, vget_high_s16(mul_value));
            weigth_neon_ptr++;
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value = vmull_s8(in_value_neon, *weigth_neon_ptr);
            weigth_neon_ptr++;

            ptr = ptr_1;
            in_value_neon = vld1_s8(ptr);
            mul_value = vmlal_s8(mul_value, in_value_neon, *weigth_neon_ptr);
            out_value_neon_0 = vaddw_s16(out_value_neon_0, vget_low_s16(mul_value));
            out_value_neon_1 = vaddw_s16(out_value_neon_1, vget_high_s16(mul_value));
            weigth_neon_ptr++;
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value = vmull_s8(in_value_neon, *weigth_neon_ptr);
            weigth_neon_ptr++;
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value = vmlal_s8(mul_value, in_value_neon, *weigth_neon_ptr);
            out_value_neon_0 = vaddw_s16(out_value_neon_0, vget_low_s16(mul_value));
            out_value_neon_1 = vaddw_s16(out_value_neon_1, vget_high_s16(mul_value));
            weigth_neon_ptr++;

            ptr = ptr_2;
            in_value_neon = vld1_s8(ptr);
            mul_value = vmull_s8(in_value_neon, *weigth_neon_ptr);
            weigth_neon_ptr++;
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value = vmlal_s8(mul_value, in_value_neon, *weigth_neon_ptr);
            out_value_neon_0 = vaddw_s16(out_value_neon_0, vget_low_s16(mul_value));
            out_value_neon_1 = vaddw_s16(out_value_neon_1, vget_high_s16(mul_value));
            weigth_neon_ptr++;
            ptr++;
            in_value_neon = vld1_s8(ptr);
            mul_value = vmull_s8(in_value_neon, *weigth_neon_ptr);
            out_value_neon_0 = vaddw_s16(out_value_neon_0, vget_low_s16(mul_value));
            out_value_neon_1 = vaddw_s16(out_value_neon_1, vget_high_s16(mul_value));
            weigth_neon_ptr++;

            ptr_0 += expand_size;
            ptr_1 += expand_size;
            ptr_2 += expand_size;
        }
        out_value_neon_0 = vmulq_n_s32(out_value_neon_0, adjust_div_value);
        out_value_neon_1 = vmulq_n_s32(out_value_neon_1, adjust_div_value);
        out_value_neon_0 = vaddq_s32(out_value_neon_0, norm_half_add_value_neon);
        out_value_neon_1 = vaddq_s32(out_value_neon_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(out_value_neon_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(out_value_neon_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon);
        vst1q_s16(p_out_buf + j, out_value_neon_16Bit);
    }
    else
#endif
    {
        for (; j < search_img_end; ++j)
        {
            int value_0 = 0;
            FixedConvType *ptr_0 = expand_input_data + j;
            FixedConvType *ptr_1 = expand_input_data + j + expand_width;
            FixedConvType *ptr_2 = expand_input_data + j + 2 * expand_width;
            FixedConvType *weight_0 = weight_ptr;

            for (m = 0; m < input_channel; ++m)
            {
                FixedConvType *ptr = ptr_0;
                value_0 += (*ptr)*(*weight_0); weight_0++;
                ptr++;
                value_0 += (*ptr)*(*weight_0); weight_0++;
                ptr++;
                value_0 += (*ptr)*(*weight_0); weight_0++;

                ptr = ptr_1;
                value_0 += (*ptr)*(*weight_0); weight_0++;
                ptr++;
                value_0 += (*ptr)*(*weight_0); weight_0++;
                ptr++;
                value_0 += (*ptr)*(*weight_0); weight_0++;

                ptr = ptr_2;
                value_0 += (*ptr)*(*weight_0); weight_0++;
                ptr++;
                value_0 += (*ptr)*(*weight_0); weight_0++;
                ptr++;
                value_0 += (*ptr)*(*weight_0); weight_0++;

                ptr_0 += expand_size;
                ptr_1 += expand_size;
                ptr_2 += expand_size;
            }
            p_out_buf[j] = ((value_0 * adjust_div_value + norm_half_add_value) >> data_norm_move_const) + bias_value;
        }
    }
#ifdef EAGLEEYE_NEON_OPTIMIZATION
    if (weight_ptr_neon)
		free(weight_ptr_neon);
#endif
}

void GetMultiOutputChannelResult1x1_4_32Bit(FixedConvType *fixed_weight, FixedConvType *expand_input_data, int input_channel, int expand_width, int expand_height, int cur_channel,
                                            int output_height, FixedType *temp_output_data_0, FixedType *temp_output_data_1, FixedType *temp_output_data_2, FixedType *temp_output_data_3, int pad_size,
                                            FixedBiasType *fixed_bias_value, int *adjust_div_value, int norm_half_add_value)
{
    int i = 0, j = 0, k = 0, m = 0;
    int search_img_start = 0;
    int search_img_end = output_height*expand_width - 2 * pad_size;
    int expand_size = expand_height*expand_width;
    int search_8_end = (search_img_end - search_img_start) / 8 * 8 + search_img_start;
    int new_kernel_size = input_channel * 4;
    FixedConvType *new_fixed_weight = (FixedConvType *)malloc(sizeof(FixedConvType)*new_kernel_size);
    const int data_norm_move_const = CNN_CONV_DATA_NORM_MOVE;
    int adjust_div_value_0 = adjust_div_value[cur_channel];
    int adjust_div_value_1 = adjust_div_value[cur_channel + 1];
    int adjust_div_value_2 = adjust_div_value[cur_channel + 2];
    int adjust_div_value_3 = adjust_div_value[cur_channel + 3];

    if (new_fixed_weight == NULL)
        return;
    const FixedConvType* k0 = fixed_weight + cur_channel*input_channel;
    const FixedConvType* k1 = fixed_weight + (cur_channel + 1)*input_channel;
    const FixedConvType* k2 = fixed_weight + (cur_channel + 2)*input_channel;
    const FixedConvType* k3 = fixed_weight + (cur_channel + 3)*input_channel;
    m = 0;
    for (k = 0; k < input_channel; ++k)
    {
        for (j = 0; j < 1; ++j)
        {
            new_fixed_weight[m] = k0[j];
            m++;
            new_fixed_weight[m] = k1[j];
            m++;
            new_fixed_weight[m] = k2[j];
            m++;
            new_fixed_weight[m] = k3[j];
            m++;
        }
        k0 ++;
        k1 ++;
        k2 ++;
        k3 ++;
    }

    FixedType *out_buf_0 = temp_output_data_0;
    FixedType *out_buf_1 = temp_output_data_1;
    FixedType *out_buf_2 = temp_output_data_2;
    FixedType *out_buf_3 = temp_output_data_3;

    j = search_img_start;

#ifdef EAGLEEYE_NEON_OPTIMIZATION
    int8x8_t *weight_ptr_neon;
    int8x8_t in_value_neon;
    int16x8_t mul_value_0, mul_value_1, mul_value_2, mul_value_3,out_value_neon_16Bit;
    int16x4_t out_value_neon_16Bit_0, out_value_neon_16Bit_1;
    int32x4_t out_value_neon0_0, out_value_neon0_1, out_value_neon1_0, out_value_neon1_1;
    int32x4_t out_value_neon2_0, out_value_neon2_1, out_value_neon3_0, out_value_neon3_1;
    int16x8_t fixed_bias_value_neon_0, fixed_bias_value_neon_1, fixed_bias_value_neon_2, fixed_bias_value_neon_3;
    int32x4_t norm_half_add_value_neon;
    int input_channel_2 = input_channel/2*2;

    norm_half_add_value_neon = vdupq_n_s32(norm_half_add_value);
    fixed_bias_value_neon_0 = vdupq_n_s16(fixed_bias_value[cur_channel]);
    fixed_bias_value_neon_1 = vdupq_n_s16(fixed_bias_value[cur_channel + 1]);
    fixed_bias_value_neon_2 = vdupq_n_s16(fixed_bias_value[cur_channel + 2]);
    fixed_bias_value_neon_3 = vdupq_n_s16(fixed_bias_value[cur_channel + 3]);

    weight_ptr_neon = (int8x8_t *)malloc(sizeof(int8x8_t)*new_kernel_size);
    if (weight_ptr_neon == NULL)
        return;
    for (j = 0; j < new_kernel_size; ++j)
        weight_ptr_neon[j] = vdup_n_s8(new_fixed_weight[j]);

    for (j = 0; j < search_8_end; j += 8)
    {
        FixedConvType *ptr_0 = expand_input_data + j;

        out_value_neon0_0 = vmovq_n_s32(0);
        out_value_neon0_1 = vmovq_n_s32(0);
        out_value_neon1_0 = vmovq_n_s32(0);
        out_value_neon1_1 = vmovq_n_s32(0);
        out_value_neon2_0 = vmovq_n_s32(0);
        out_value_neon2_1 = vmovq_n_s32(0);
        out_value_neon3_0 = vmovq_n_s32(0);
        out_value_neon3_1 = vmovq_n_s32(0);

        int8x8_t *weigth_neon_ptr_0 = weight_ptr_neon;
        for (m = 0; m < input_channel_2; m += 2)
        {
            in_value_neon = vld1_s8(ptr_0);
            mul_value_0 = vmull_s8(in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            mul_value_1 = vmull_s8(in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            mul_value_2 = vmull_s8(in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            mul_value_3 = vmull_s8(in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;

            ptr_0 += expand_size;

            in_value_neon = vld1_s8(ptr_0);
            mul_value_0 = vmlal_s8(mul_value_0, in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            mul_value_1 = vmlal_s8(mul_value_1, in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            mul_value_2 = vmlal_s8(mul_value_2, in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            mul_value_3 = vmlal_s8(mul_value_3, in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
            out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
            out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
            out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));
            out_value_neon2_0 = vaddw_s16(out_value_neon2_0, vget_low_s16(mul_value_2));
            out_value_neon2_1 = vaddw_s16(out_value_neon2_1, vget_high_s16(mul_value_2));
            out_value_neon3_0 = vaddw_s16(out_value_neon3_0, vget_low_s16(mul_value_3));
            out_value_neon3_1 = vaddw_s16(out_value_neon3_1, vget_high_s16(mul_value_3));

            ptr_0 += expand_size;
        }
        for (; m < input_channel; m ++)
        {
            in_value_neon = vld1_s8(ptr_0);
            mul_value_0 = vmull_s8(in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            mul_value_1 = vmull_s8(in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            mul_value_2 = vmull_s8(in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            mul_value_3 = vmull_s8(in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
            out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
            out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
            out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));
            out_value_neon2_0 = vaddw_s16(out_value_neon2_0, vget_low_s16(mul_value_2));
            out_value_neon2_1 = vaddw_s16(out_value_neon2_1, vget_high_s16(mul_value_2));
            out_value_neon3_0 = vaddw_s16(out_value_neon3_0, vget_low_s16(mul_value_3));
            out_value_neon3_1 = vaddw_s16(out_value_neon3_1, vget_high_s16(mul_value_3));

            ptr_0 += expand_size;
        }
        out_value_neon0_0 = vmulq_n_s32(out_value_neon0_0, adjust_div_value_0);
        out_value_neon0_1 = vmulq_n_s32(out_value_neon0_1, adjust_div_value_0);
        out_value_neon0_0 = vaddq_s32(out_value_neon0_0, norm_half_add_value_neon);
        out_value_neon0_1 = vaddq_s32(out_value_neon0_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(out_value_neon0_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(out_value_neon0_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_0);
        vst1q_s16(out_buf_0 + j, out_value_neon_16Bit);

        out_value_neon1_0 = vmulq_n_s32(out_value_neon1_0, adjust_div_value_1);
        out_value_neon1_1 = vmulq_n_s32(out_value_neon1_1, adjust_div_value_1);
        out_value_neon1_0 = vaddq_s32(out_value_neon1_0, norm_half_add_value_neon);
        out_value_neon1_1 = vaddq_s32(out_value_neon1_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(out_value_neon1_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(out_value_neon1_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_1);
        vst1q_s16(out_buf_1 + j, out_value_neon_16Bit);

        out_value_neon1_0 = vmulq_n_s32(out_value_neon2_0, adjust_div_value_2);
        out_value_neon1_1 = vmulq_n_s32(out_value_neon2_1, adjust_div_value_2);
        out_value_neon1_0 = vaddq_s32(out_value_neon1_0, norm_half_add_value_neon);
        out_value_neon1_1 = vaddq_s32(out_value_neon1_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(out_value_neon1_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(out_value_neon1_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_2);
        vst1q_s16(out_buf_2 + j, out_value_neon_16Bit);

        out_value_neon1_0 = vmulq_n_s32(out_value_neon3_0, adjust_div_value_3);
        out_value_neon1_1 = vmulq_n_s32(out_value_neon3_1, adjust_div_value_3);
        out_value_neon1_0 = vaddq_s32(out_value_neon1_0, norm_half_add_value_neon);
        out_value_neon1_1 = vaddq_s32(out_value_neon1_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(out_value_neon1_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(out_value_neon1_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_3);
        vst1q_s16(out_buf_3 + j, out_value_neon_16Bit);
    }

    if (j < search_img_end&&search_img_end - search_img_start >= 8)
    {
        j = search_img_end - 8;
        FixedConvType *ptr_0 = expand_input_data + j;

        out_value_neon0_0 = vmovq_n_s32(0);
        out_value_neon0_1 = vmovq_n_s32(0);
        out_value_neon1_0 = vmovq_n_s32(0);
        out_value_neon1_1 = vmovq_n_s32(0);
        out_value_neon2_0 = vmovq_n_s32(0);
        out_value_neon2_1 = vmovq_n_s32(0);
        out_value_neon3_0 = vmovq_n_s32(0);
        out_value_neon3_1 = vmovq_n_s32(0);

        int8x8_t *weigth_neon_ptr_0 = weight_ptr_neon;
        for (m = 0; m < input_channel; m ++)
        {
            in_value_neon = vld1_s8(ptr_0);
            mul_value_0 = vmull_s8(in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            mul_value_1 = vmull_s8(in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            mul_value_2 = vmull_s8(in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            mul_value_3 = vmull_s8(in_value_neon, *weigth_neon_ptr_0); weigth_neon_ptr_0++;
            out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
            out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
            out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
            out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));
            out_value_neon2_0 = vaddw_s16(out_value_neon2_0, vget_low_s16(mul_value_2));
            out_value_neon2_1 = vaddw_s16(out_value_neon2_1, vget_high_s16(mul_value_2));
            out_value_neon3_0 = vaddw_s16(out_value_neon3_0, vget_low_s16(mul_value_3));
            out_value_neon3_1 = vaddw_s16(out_value_neon3_1, vget_high_s16(mul_value_3));

            ptr_0 += expand_size;
        }
        out_value_neon0_0 = vmulq_n_s32(out_value_neon0_0, adjust_div_value_0);
        out_value_neon0_1 = vmulq_n_s32(out_value_neon0_1, adjust_div_value_0);
        out_value_neon0_0 = vaddq_s32(out_value_neon0_0, norm_half_add_value_neon);
        out_value_neon0_1 = vaddq_s32(out_value_neon0_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(out_value_neon0_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(out_value_neon0_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_0);
        vst1q_s16(out_buf_0 + j, out_value_neon_16Bit);

        out_value_neon1_0 = vmulq_n_s32(out_value_neon1_0, adjust_div_value_1);
        out_value_neon1_1 = vmulq_n_s32(out_value_neon1_1, adjust_div_value_1);
        out_value_neon1_0 = vaddq_s32(out_value_neon1_0, norm_half_add_value_neon);
        out_value_neon1_1 = vaddq_s32(out_value_neon1_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(out_value_neon1_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(out_value_neon1_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_1);
        vst1q_s16(out_buf_1 + j, out_value_neon_16Bit);

        out_value_neon1_0 = vmulq_n_s32(out_value_neon2_0, adjust_div_value_2);
        out_value_neon1_1 = vmulq_n_s32(out_value_neon2_1, adjust_div_value_2);
        out_value_neon1_0 = vaddq_s32(out_value_neon1_0, norm_half_add_value_neon);
        out_value_neon1_1 = vaddq_s32(out_value_neon1_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(out_value_neon1_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(out_value_neon1_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_2);
        vst1q_s16(out_buf_2 + j, out_value_neon_16Bit);

        out_value_neon1_0 = vmulq_n_s32(out_value_neon3_0, adjust_div_value_3);
        out_value_neon1_1 = vmulq_n_s32(out_value_neon3_1, adjust_div_value_3);
        out_value_neon1_0 = vaddq_s32(out_value_neon1_0, norm_half_add_value_neon);
        out_value_neon1_1 = vaddq_s32(out_value_neon1_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(out_value_neon1_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(out_value_neon1_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_3);
        vst1q_s16(out_buf_3 + j, out_value_neon_16Bit);
    }
    else
#endif
    {
        for (; j < search_img_end; ++j)
        {
            int value_0 = 0;
            int value_1 = 0;
            int value_2 = 0;
            int value_3 = 0;
            FixedConvType *ptr = expand_input_data + j;
            FixedConvType *weight_0 = new_fixed_weight;

            for (m = 0; m < input_channel; ++m)
            {
                value_0 += (*ptr)*(*weight_0); weight_0++;
                value_1 += (*ptr)*(*weight_0); weight_0++;
                value_2 += (*ptr)*(*weight_0); weight_0++;
                value_3 += (*ptr)*(*weight_0); weight_0++;
                ptr += expand_size;
            }
            temp_output_data_0[j] = ((value_0 * adjust_div_value_0 + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[cur_channel];
            temp_output_data_1[j] = ((value_1 * adjust_div_value_1 + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[cur_channel + 1];
            temp_output_data_2[j] = ((value_2 * adjust_div_value_2 + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[cur_channel + 2];
            temp_output_data_3[j] = ((value_3 * adjust_div_value_3 + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[cur_channel + 3];
        }
    }
    EAGLEEYE_SAFEFREE(new_fixed_weight);
#ifdef EAGLEEYE_NEON_OPTIMIZATION11
    EAGLEEYE_SAFEFREE(weight_ptr_neon);
#endif
}

void GetOneOutputChannelResult1x1_32Bit(FixedConvType *weight_ptr, FixedConvType *expand_input_data, int input_channel, int expand_width,
                                        int expand_height, int output_height, FixedType *temp_output_data, int pad_size,
                                        FixedBiasType bias_value, int adjust_div_value, int norm_half_add_value)
{
    int i = 0, j = 0, m = 0;
    int search_img_start = 0;
    int search_img_end = output_height*expand_width - 2 * pad_size;
    int search_8_end = (search_img_end - search_img_start) / 8 * 8 + search_img_start;
    FixedType *p_out_buf = temp_output_data;
    int expand_size = expand_height*expand_width;
    const int data_norm_move_const = CNN_CONV_DATA_NORM_MOVE;

    j = search_img_start;

#ifdef EAGLEEYE_NEON_OPTIMIZATION
    int8x8_t *weight_ptr_neon;
    int loop_size = input_channel;

    weight_ptr_neon = (int8x8_t *)malloc(sizeof(int8x8_t)*loop_size);
    if (weight_ptr_neon == NULL)
        return;

    for (i = 0; i < loop_size; ++i)
        weight_ptr_neon[i] = vdup_n_s8(weight_ptr[i]);

    int8x8_t in_value_neon;
    int32x4_t out_value_neon_0, out_value_neon_1;
    int16x4_t out_value_neon_16Bit_0, out_value_neon_16Bit_1;
    int16x8_t mul_value, out_value_neon_16Bit, fixed_bias_value_neon;
    int32x4_t norm_half_add_value_neon;
    norm_half_add_value_neon = vdupq_n_s32(norm_half_add_value);
    fixed_bias_value_neon = vdupq_n_s16(bias_value);

    for (j = 0; j < search_8_end; j += 8)
    {
        FixedConvType *ptr_0 = expand_input_data + j;

        int8x8_t *weigth_neon_ptr = weight_ptr_neon;
        out_value_neon_0 = vdupq_n_s32(0);
        out_value_neon_1 = vdupq_n_s32(0);
        for (m = 0; m < input_channel; m++)
        {
            in_value_neon = vld1_s8(ptr_0);
            mul_value = vmull_s8(in_value_neon, *weigth_neon_ptr); weigth_neon_ptr++;
            out_value_neon_0 = vaddw_s16(out_value_neon_0, vget_low_s16(mul_value));
            out_value_neon_1 = vaddw_s16(out_value_neon_1, vget_high_s16(mul_value));

            ptr_0 += expand_size;
        }

        out_value_neon_0 = vmulq_n_s32(out_value_neon_0, adjust_div_value);
        out_value_neon_1 = vmulq_n_s32(out_value_neon_1, adjust_div_value);
        out_value_neon_0 = vaddq_s32(out_value_neon_0, norm_half_add_value_neon);
        out_value_neon_1 = vaddq_s32(out_value_neon_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(out_value_neon_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(out_value_neon_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon);
        vst1q_s16(p_out_buf + j, out_value_neon_16Bit);
    }

    if (j < search_img_end&&search_img_end - search_img_start >= 8)
    {
        j = search_img_end - 8;

        FixedConvType *ptr_0 = expand_input_data + j;
        int8x8_t *weigth_neon_ptr = weight_ptr_neon;

        out_value_neon_0 = vdupq_n_s32(0);
        out_value_neon_1 = vdupq_n_s32(0);
        for (m = 0; m < input_channel; ++m)
        {
            in_value_neon = vld1_s8(ptr_0);
            mul_value = vmull_s8(in_value_neon, *weigth_neon_ptr); weigth_neon_ptr++;
            out_value_neon_0 = vaddw_s16(out_value_neon_0, vget_low_s16(mul_value));
            out_value_neon_1 = vaddw_s16(out_value_neon_1, vget_high_s16(mul_value));

            ptr_0 += expand_size;
        }
        out_value_neon_0 = vmulq_n_s32(out_value_neon_0, adjust_div_value);
        out_value_neon_1 = vmulq_n_s32(out_value_neon_1, adjust_div_value);
        out_value_neon_0 = vaddq_s32(out_value_neon_0, norm_half_add_value_neon);
        out_value_neon_1 = vaddq_s32(out_value_neon_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(out_value_neon_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(out_value_neon_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon);
        vst1q_s16(p_out_buf + j, out_value_neon_16Bit);
    }
    else
#endif
    {
        for (; j < search_img_end; ++j)
        {
            int value_0 = 0;
            FixedConvType *ptr_0 = expand_input_data + j;
            FixedConvType *weight_0 = weight_ptr;

            for (m = 0; m < input_channel; ++m)
            {
                value_0 += (*ptr_0)*(*weight_0); weight_0++;
                ptr_0 += expand_size;
            }
            p_out_buf[j] = ((value_0 * adjust_div_value + norm_half_add_value) >> data_norm_move_const) + bias_value;
        }
    }
#ifdef EAGLEEYE_NEON_OPTIMIZATION
    if (weight_ptr_neon)
		free(weight_ptr_neon);
#endif
}


void GetMultiOutputChannelResult3x3_Stride2_2_32Bit(FixedConvType *fixed_weight, FixedConvType *expand_input_data, int input_channel, int expand_width,
                                                    int expand_height, int cur_channel, int output_width, int output_height, FixedType *output_data, int pad_size,
                                                    FixedBiasType *fixed_bias_value, int *adjust_div_value, int norm_half_add_value)
{
    int i = 0, j = 0, k = 0, m = 0, n = 0, l = 0;
    int expand_size = expand_height*expand_width;
    int new_kernel_size = input_channel * 2 * 9;
    const int data_norm_move_const = CNN_CONV_DATA_NORM_MOVE;
    FixedType *p_out_buf_0 = output_data + cur_channel*output_width*output_height;
    FixedType *p_out_buf_1 = output_data + (cur_channel + 1)*output_width*output_height;
    FixedConvType *new_fixed_weight = (FixedConvType *)malloc(sizeof(FixedConvType)*new_kernel_size);
    int adjust_div_value_0 = adjust_div_value[cur_channel];
    int adjust_div_value_1 = adjust_div_value[cur_channel + 1];


    if (new_fixed_weight == NULL)
        return;

    const FixedConvType* k0 = fixed_weight + cur_channel*input_channel * 9;
    const FixedConvType* k1 = fixed_weight + (cur_channel + 1)*input_channel * 9;
    m = 0;
    for (k = 0; k < input_channel; ++k)
    {
        for (j = 0; j < 9; ++j)
        {
            new_fixed_weight[m] = k0[j];
            m++;
            new_fixed_weight[m] = k1[j];
            m++;
        }
        k0 += 9;
        k1 += 9;
    }

#ifdef EAGLEEYE_NEON_OPTIMIZATION
    int8x8_t *weight_ptr_neon = NULL;
    int search_8_end = output_width / 8 * 8;
    int8x8x2_t in_value_neon;
    int16x8_t mul_value_0, mul_value_1, out_value_neon_16Bit;
    int16x4_t out_value_neon_16Bit_0, out_value_neon_16Bit_1;
    int32x4_t out_value_neon0_0, out_value_neon0_1, out_value_neon1_0, out_value_neon1_1;
    int16x8_t fixed_bias_value_neon_0, fixed_bias_value_neon_1;
    int32x4_t norm_half_add_value_neon;

    norm_half_add_value_neon = vdupq_n_s32(norm_half_add_value);
    fixed_bias_value_neon_0 = vdupq_n_s16(fixed_bias_value[cur_channel]);
    fixed_bias_value_neon_1 = vdupq_n_s16(fixed_bias_value[cur_channel + 1]);

    weight_ptr_neon = (int8x8_t *)malloc(sizeof(int8x8_t)*new_kernel_size);
    if (weight_ptr_neon == NULL)
        return;
    for (k = 0; k < new_kernel_size; ++k)
        weight_ptr_neon[k] = vdup_n_s8(new_fixed_weight[k]);

    for (j = 0; j < output_height; ++j)
    {
        n = j * 2 * expand_width;
        for (k = 0; k < search_8_end; k += 8)
        {
            int8x8_t *weigth_neon_ptr = weight_ptr_neon;
            FixedConvType *ptr_0 = expand_input_data + n;
            FixedConvType *ptr_1 = expand_input_data + n + expand_width;
            FixedConvType *ptr_2 = expand_input_data + n + expand_width * 2;
            n += 16;

            out_value_neon0_0 = vdupq_n_s32(0);
            out_value_neon0_1 = vdupq_n_s32(0);
            out_value_neon1_0 = vdupq_n_s32(0);
            out_value_neon1_1 = vdupq_n_s32(0);
            for (m = 0; m < input_channel; m++)
            {
                FixedConvType *ptr = ptr_0;
                in_value_neon = vld2_s8(ptr);
                mul_value_0 = vmull_s8(in_value_neon.val[0], *weigth_neon_ptr); weigth_neon_ptr++;
                mul_value_1 = vmull_s8(in_value_neon.val[0], *weigth_neon_ptr);   weigth_neon_ptr++;
                mul_value_0 = vmlal_s8(mul_value_0, in_value_neon.val[1], *weigth_neon_ptr);   weigth_neon_ptr++;
                mul_value_1 = vmlal_s8(mul_value_1, in_value_neon.val[1], *weigth_neon_ptr);   weigth_neon_ptr++;
                out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
                out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
                out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
                out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));
                ptr+=2;
                in_value_neon = vld2_s8(ptr);
                mul_value_0 = vmull_s8(in_value_neon.val[0], *weigth_neon_ptr); weigth_neon_ptr++;
                mul_value_1 = vmull_s8(in_value_neon.val[0], *weigth_neon_ptr);   weigth_neon_ptr++;

                ptr = ptr_1;
                in_value_neon = vld2_s8(ptr);
                mul_value_0 = vmlal_s8(mul_value_0, in_value_neon.val[0], *weigth_neon_ptr);   weigth_neon_ptr++;
                mul_value_1 = vmlal_s8(mul_value_1, in_value_neon.val[0], *weigth_neon_ptr);   weigth_neon_ptr++;
                out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
                out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
                out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
                out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));
                mul_value_0 = vmull_s8(in_value_neon.val[1], *weigth_neon_ptr); weigth_neon_ptr++;
                mul_value_1 = vmull_s8(in_value_neon.val[1], *weigth_neon_ptr);   weigth_neon_ptr++;
                ptr += 2;
                in_value_neon = vld2_s8(ptr);
                mul_value_0 = vmlal_s8(mul_value_0, in_value_neon.val[0], *weigth_neon_ptr);   weigth_neon_ptr++;
                mul_value_1 = vmlal_s8(mul_value_1, in_value_neon.val[0], *weigth_neon_ptr);   weigth_neon_ptr++;
                out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
                out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
                out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
                out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));

                ptr = ptr_2;
                in_value_neon = vld2_s8(ptr);
                mul_value_0 = vmull_s8(in_value_neon.val[0], *weigth_neon_ptr); weigth_neon_ptr++;
                mul_value_1 = vmull_s8(in_value_neon.val[0], *weigth_neon_ptr);   weigth_neon_ptr++;
                mul_value_0 = vmlal_s8(mul_value_0, in_value_neon.val[1], *weigth_neon_ptr);   weigth_neon_ptr++;
                mul_value_1 = vmlal_s8(mul_value_1, in_value_neon.val[1], *weigth_neon_ptr);   weigth_neon_ptr++;
                out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
                out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
                out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
                out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));
                ptr+=2;
                in_value_neon = vld2_s8(ptr);
                mul_value_0 = vmull_s8(in_value_neon.val[0], *weigth_neon_ptr); weigth_neon_ptr++;
                mul_value_1 = vmull_s8(in_value_neon.val[0], *weigth_neon_ptr);   weigth_neon_ptr++;
                out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
                out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
                out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
                out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));

                ptr_0 += expand_size;
                ptr_1 += expand_size;
                ptr_2 += expand_size;
            }

            out_value_neon0_0 = vmulq_n_s32(out_value_neon0_0, adjust_div_value_0);
            out_value_neon0_1 = vmulq_n_s32(out_value_neon0_1, adjust_div_value_0);
            out_value_neon0_0 = vaddq_s32(out_value_neon0_0, norm_half_add_value_neon);
            out_value_neon0_1 = vaddq_s32(out_value_neon0_1, norm_half_add_value_neon);
            out_value_neon_16Bit_0 = vshrn_n_s32(out_value_neon0_0, data_norm_move_const);
            out_value_neon_16Bit_1 = vshrn_n_s32(out_value_neon0_1, data_norm_move_const);
            out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
            out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_0);
            vst1q_s16(p_out_buf_0 + j*output_width + k, out_value_neon_16Bit);

            out_value_neon1_0 = vmulq_n_s32(out_value_neon1_0, adjust_div_value_1);
            out_value_neon1_1 = vmulq_n_s32(out_value_neon1_1, adjust_div_value_1);
            out_value_neon1_0 = vaddq_s32(out_value_neon1_0, norm_half_add_value_neon);
            out_value_neon1_1 = vaddq_s32(out_value_neon1_1, norm_half_add_value_neon);
            out_value_neon_16Bit_0 = vshrn_n_s32(out_value_neon1_0, data_norm_move_const);
            out_value_neon_16Bit_1 = vshrn_n_s32(out_value_neon1_1, data_norm_move_const);
            out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
            out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_1);
            vst1q_s16(p_out_buf_1 + j*output_width + k, out_value_neon_16Bit);
        }
        if (k < output_width&&output_width >= 8)
        {
            k = output_width - 8;
            n = j * 2 * expand_width + k*2;

            int8x8_t *weigth_neon_ptr = weight_ptr_neon;
            FixedConvType *ptr_0 = expand_input_data + n;
            FixedConvType *ptr_1 = expand_input_data + n + expand_width;
            FixedConvType *ptr_2 = expand_input_data + n + expand_width * 2;
            out_value_neon0_0 = vdupq_n_s32(0);
            out_value_neon0_1 = vdupq_n_s32(0);
            out_value_neon1_0 = vdupq_n_s32(0);
            out_value_neon1_1 = vdupq_n_s32(0);
            for (m = 0; m < input_channel; ++m)
            {
                FixedConvType *ptr = ptr_0;
                in_value_neon = vld2_s8(ptr);
                mul_value_0 = vmull_s8(in_value_neon.val[0], *weigth_neon_ptr);
                weigth_neon_ptr++;
                mul_value_1 = vmull_s8(in_value_neon.val[0], *weigth_neon_ptr);
                weigth_neon_ptr++;
                mul_value_0 = vmlal_s8(mul_value_0, in_value_neon.val[1], *weigth_neon_ptr);
                weigth_neon_ptr++;
                mul_value_1 = vmlal_s8(mul_value_1, in_value_neon.val[1], *weigth_neon_ptr);
                weigth_neon_ptr++;
                out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
                out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
                out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
                out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));
                ptr += 2;
                in_value_neon = vld2_s8(ptr);
                mul_value_0 = vmull_s8(in_value_neon.val[0], *weigth_neon_ptr);
                weigth_neon_ptr++;
                mul_value_1 = vmull_s8(in_value_neon.val[0], *weigth_neon_ptr);
                weigth_neon_ptr++;

                ptr = ptr_1;
                in_value_neon = vld2_s8(ptr);
                mul_value_0 = vmlal_s8(mul_value_0, in_value_neon.val[0], *weigth_neon_ptr);
                weigth_neon_ptr++;
                mul_value_1 = vmlal_s8(mul_value_1, in_value_neon.val[0], *weigth_neon_ptr);
                weigth_neon_ptr++;
                out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
                out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
                out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
                out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));
                mul_value_0 = vmull_s8(in_value_neon.val[1], *weigth_neon_ptr);
                weigth_neon_ptr++;
                mul_value_1 = vmull_s8(in_value_neon.val[1], *weigth_neon_ptr);
                weigth_neon_ptr++;
                ptr += 2;
                in_value_neon = vld2_s8(ptr);
                mul_value_0 = vmlal_s8(mul_value_0, in_value_neon.val[0], *weigth_neon_ptr);
                weigth_neon_ptr++;
                mul_value_1 = vmlal_s8(mul_value_1, in_value_neon.val[0], *weigth_neon_ptr);
                weigth_neon_ptr++;
                out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
                out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
                out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
                out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));

                ptr = ptr_2;
                in_value_neon = vld2_s8(ptr);
                mul_value_0 = vmull_s8(in_value_neon.val[0], *weigth_neon_ptr);
                weigth_neon_ptr++;
                mul_value_1 = vmull_s8(in_value_neon.val[0], *weigth_neon_ptr);
                weigth_neon_ptr++;
                mul_value_0 = vmlal_s8(mul_value_0, in_value_neon.val[1], *weigth_neon_ptr);
                weigth_neon_ptr++;
                mul_value_1 = vmlal_s8(mul_value_1, in_value_neon.val[1], *weigth_neon_ptr);
                weigth_neon_ptr++;
                out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
                out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
                out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
                out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));
                ptr += 2;
                in_value_neon = vld2_s8(ptr);
                mul_value_0 = vmull_s8(in_value_neon.val[0], *weigth_neon_ptr);
                weigth_neon_ptr++;
                mul_value_1 = vmull_s8(in_value_neon.val[0], *weigth_neon_ptr);
                weigth_neon_ptr++;
                out_value_neon0_0 = vaddw_s16(out_value_neon0_0, vget_low_s16(mul_value_0));
                out_value_neon0_1 = vaddw_s16(out_value_neon0_1, vget_high_s16(mul_value_0));
                out_value_neon1_0 = vaddw_s16(out_value_neon1_0, vget_low_s16(mul_value_1));
                out_value_neon1_1 = vaddw_s16(out_value_neon1_1, vget_high_s16(mul_value_1));

                ptr_0 += expand_size;
                ptr_1 += expand_size;
                ptr_2 += expand_size;
            }
            out_value_neon0_0 = vmulq_n_s32(out_value_neon0_0, adjust_div_value_0);
            out_value_neon0_1 = vmulq_n_s32(out_value_neon0_1, adjust_div_value_0);
            out_value_neon0_0 = vaddq_s32(out_value_neon0_0, norm_half_add_value_neon);
            out_value_neon0_1 = vaddq_s32(out_value_neon0_1, norm_half_add_value_neon);
            out_value_neon_16Bit_0 = vshrn_n_s32(out_value_neon0_0, data_norm_move_const);
            out_value_neon_16Bit_1 = vshrn_n_s32(out_value_neon0_1, data_norm_move_const);
            out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
            out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_0);
            vst1q_s16(p_out_buf_0 + j*output_width + k, out_value_neon_16Bit);

            out_value_neon1_0 = vmulq_n_s32(out_value_neon1_0, adjust_div_value_1);
            out_value_neon1_1 = vmulq_n_s32(out_value_neon1_1, adjust_div_value_1);
            out_value_neon1_0 = vaddq_s32(out_value_neon1_0, norm_half_add_value_neon);
            out_value_neon1_1 = vaddq_s32(out_value_neon1_1, norm_half_add_value_neon);
            out_value_neon_16Bit_0 = vshrn_n_s32(out_value_neon1_0, data_norm_move_const);
            out_value_neon_16Bit_1 = vshrn_n_s32(out_value_neon1_1, data_norm_move_const);
            out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
            out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_1);
            vst1q_s16(p_out_buf_1 + j*output_width + k, out_value_neon_16Bit);
        }
        else
        {
            n = j * 2 * expand_width + k*2;

            for (; k < output_width; ++k)
            {
                int value_0 = 0;
                int value_1 = 0;
                FixedConvType *ptr_0 = expand_input_data + n;
                FixedConvType *ptr_1 = expand_input_data + n + expand_width;
                FixedConvType *ptr_2 = expand_input_data + n + expand_width * 2;
                FixedConvType *weight_0 = new_fixed_weight;

                for (m = 0; m < input_channel; ++m)
                {
                    FixedConvType *ptr = ptr_0;
                    value_0 += (*ptr)*(*weight_0); weight_0++;
                    value_1 += (*ptr)*(*weight_0); weight_0++;
                    ptr++;
                    value_0 += (*ptr)*(*weight_0); weight_0++;
                    value_1 += (*ptr)*(*weight_0); weight_0++;
                    ptr++;
                    value_0 += (*ptr)*(*weight_0); weight_0++;
                    value_1 += (*ptr)*(*weight_0); weight_0++;

                    ptr = ptr_1;
                    value_0 += (*ptr)*(*weight_0); weight_0++;
                    value_1 += (*ptr)*(*weight_0); weight_0++;
                    ptr++;
                    value_0 += (*ptr)*(*weight_0); weight_0++;
                    value_1 += (*ptr)*(*weight_0); weight_0++;
                    ptr++;
                    value_0 += (*ptr)*(*weight_0); weight_0++;
                    value_1 += (*ptr)*(*weight_0); weight_0++;

                    ptr = ptr_2;
                    value_0 += (*ptr)*(*weight_0); weight_0++;
                    value_1 += (*ptr)*(*weight_0); weight_0++;
                    ptr++;
                    value_0 += (*ptr)*(*weight_0); weight_0++;
                    value_1 += (*ptr)*(*weight_0); weight_0++;
                    ptr++;
                    value_0 += (*ptr)*(*weight_0); weight_0++;
                    value_1 += (*ptr)*(*weight_0); weight_0++;

                    ptr_0 += expand_size;
                    ptr_1 += expand_size;
                    ptr_2 += expand_size;
                }

                n += 2;
                p_out_buf_0[j*output_width + k] = ((value_0 * adjust_div_value_0 + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[cur_channel];
                p_out_buf_1[j*output_width + k] = ((value_1 * adjust_div_value_1 + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[cur_channel + 1];
            }
        }
    }
    if (weight_ptr_neon)
        free(weight_ptr_neon);
#else
    for (j = 0; j < output_height; ++j)
    {
        n = j * 2 * expand_width;

        for (k = 0; k < output_width; ++k)
        {
            int value_0 = 0;
            int value_1 = 0;
            FixedConvType *ptr_0 = expand_input_data + n;
            FixedConvType *ptr_1 = expand_input_data + n + expand_width;
            FixedConvType *ptr_2 = expand_input_data + n + expand_width * 2;
            FixedConvType *weight_0 = new_fixed_weight;

            for (m = 0; m < input_channel; ++m)
            {
                FixedConvType *ptr = ptr_0;
                value_0 += (*ptr)*(*weight_0); weight_0++;
                value_1 += (*ptr)*(*weight_0); weight_0++;
                ptr++;
                value_0 += (*ptr)*(*weight_0); weight_0++;
                value_1 += (*ptr)*(*weight_0); weight_0++;
                ptr++;
                value_0 += (*ptr)*(*weight_0); weight_0++;
                value_1 += (*ptr)*(*weight_0); weight_0++;

                ptr = ptr_1;
                value_0 += (*ptr)*(*weight_0); weight_0++;
                value_1 += (*ptr)*(*weight_0); weight_0++;
                ptr++;
                value_0 += (*ptr)*(*weight_0); weight_0++;
                value_1 += (*ptr)*(*weight_0); weight_0++;
                ptr++;
                value_0 += (*ptr)*(*weight_0); weight_0++;
                value_1 += (*ptr)*(*weight_0); weight_0++;

                ptr = ptr_2;
                value_0 += (*ptr)*(*weight_0); weight_0++;
                value_1 += (*ptr)*(*weight_0); weight_0++;
                ptr++;
                value_0 += (*ptr)*(*weight_0); weight_0++;
                value_1 += (*ptr)*(*weight_0); weight_0++;
                ptr++;
                value_0 += (*ptr)*(*weight_0); weight_0++;
                value_1 += (*ptr)*(*weight_0); weight_0++;

                ptr_0 += expand_size;
                ptr_1 += expand_size;
                ptr_2 += expand_size;
            }

            n += 2;
            p_out_buf_0[j*output_width + k] = ((value_0 * adjust_div_value_0 + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[cur_channel];
            p_out_buf_1[j*output_width + k] = ((value_1 * adjust_div_value_1 + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[cur_channel + 1];
        }
    }
#endif
    EAGLEEYE_SAFEFREE(new_fixed_weight);
}

void GetOneOutputChannelResult3x3_Stride2_32Bit(FixedConvType *weight_ptr, FixedConvType *expand_input_data, int input_channel, int expand_width,
                                                int expand_height, int output_width, int output_height, FixedType *output_data, int pad_size,
                                                FixedBiasType bias_value, int adjust_div_value, int norm_half_add_value)
{
    int i = 0, j = 0, k = 0, l = 0, m = 0, n = 0;
    const int data_norm_move_const = CNN_CONV_DATA_NORM_MOVE;
    int expand_size = expand_width*expand_height;

#ifdef EAGLEEYE_NEON_OPTIMIZATION
    int8x8_t *weight_ptr_neon;
     int search_8_end = output_width / 16 * 16;

    weight_ptr_neon = (int8x8_t *)malloc(sizeof(int8x8_t)*input_channel * 9);
    if (weight_ptr_neon == NULL)
        return;

    for (i = 0; i < input_channel * 9; ++i)
        weight_ptr_neon[i] = vdup_n_s8(weight_ptr[i]);

    int8x16x2_t in_value_neon;
    int8x8x2_t in_value_neon_0;
    int32x4_t out_value_neon_0, out_value_neon_1;
    int32x4_t out_value_neon_2, out_value_neon_3;
    int16x4_t out_value_neon_16Bit_0, out_value_neon_16Bit_1;
    int16x8_t mul_value_0, mul_value_1, out_value_neon_16Bit, fixed_bias_value_neon;
    int32x4_t norm_half_add_value_neon;

    norm_half_add_value_neon = vdupq_n_s32(norm_half_add_value);
    fixed_bias_value_neon = vdupq_n_s16(bias_value);

    for (j = 0; j < output_height; ++j)
    {
        n = j * 2 * expand_width;
        FixedType *p_out_buf = output_data + j*output_width;
        for (k = 0; k < search_8_end; k += 16)
        {
            int8x8_t *weigth_neon_ptr = weight_ptr_neon;
            FixedConvType *ptr_0 = expand_input_data + n;
            FixedConvType *ptr_1 = expand_input_data + n + expand_width;
            FixedConvType *ptr_2 = expand_input_data + n + expand_width * 2;
            n += 32;

            out_value_neon_0 = vdupq_n_s32(0);
            out_value_neon_1 = vdupq_n_s32(0);
            out_value_neon_2 = vdupq_n_s32(0);
            out_value_neon_3 = vdupq_n_s32(0);
            for (m = 0; m < input_channel; m++)
            {
                FixedConvType *ptr = ptr_0;
                in_value_neon = vld2q_s8(ptr);
                mul_value_0 = vmull_s8(vget_low_s8(in_value_neon.val[0]), *weigth_neon_ptr);
                mul_value_1 = vmull_s8(vget_high_s8(in_value_neon.val[0]), *weigth_neon_ptr);
                weigth_neon_ptr++;
                mul_value_0 = vmlal_s8(mul_value_0, vget_low_s8(in_value_neon.val[1]), *weigth_neon_ptr);
                mul_value_1 = vmlal_s8(mul_value_1, vget_high_s8(in_value_neon.val[1]), *weigth_neon_ptr);
                out_value_neon_0 = vaddw_s16(out_value_neon_0, vget_low_s16(mul_value_0));
                out_value_neon_1 = vaddw_s16(out_value_neon_1, vget_high_s16(mul_value_0));
                out_value_neon_2 = vaddw_s16(out_value_neon_2, vget_low_s16(mul_value_1));
                out_value_neon_3 = vaddw_s16(out_value_neon_3, vget_high_s16(mul_value_1));
                weigth_neon_ptr++;
                ptr += 2;
                in_value_neon = vld2q_s8(ptr);
                mul_value_0 = vmull_s8(vget_low_s8(in_value_neon.val[0]), *weigth_neon_ptr);
                mul_value_1 = vmull_s8(vget_high_s8(in_value_neon.val[0]), *weigth_neon_ptr);
                weigth_neon_ptr++;

                ptr = ptr_1;
                in_value_neon = vld2q_s8(ptr);
                mul_value_0 = vmlal_s8(mul_value_0, vget_low_s8(in_value_neon.val[0]), *weigth_neon_ptr);
                mul_value_1 = vmlal_s8(mul_value_1, vget_high_s8(in_value_neon.val[0]), *weigth_neon_ptr);
                out_value_neon_0 = vaddw_s16(out_value_neon_0, vget_low_s16(mul_value_0));
                out_value_neon_1 = vaddw_s16(out_value_neon_1, vget_high_s16(mul_value_0));
                out_value_neon_2 = vaddw_s16(out_value_neon_2, vget_low_s16(mul_value_1));
                out_value_neon_3 = vaddw_s16(out_value_neon_3, vget_high_s16(mul_value_1));
                weigth_neon_ptr++;
                mul_value_0 = vmull_s8(vget_low_s8(in_value_neon.val[1]), *weigth_neon_ptr);
                mul_value_1 = vmull_s8(vget_high_s8(in_value_neon.val[1]), *weigth_neon_ptr);
                weigth_neon_ptr++;
                ptr += 2;
                in_value_neon = vld2q_s8(ptr);
                mul_value_0 = vmlal_s8(mul_value_0, vget_low_s8(in_value_neon.val[0]), *weigth_neon_ptr);
                mul_value_1 = vmlal_s8(mul_value_1, vget_high_s8(in_value_neon.val[0]), *weigth_neon_ptr);
                out_value_neon_0 = vaddw_s16(out_value_neon_0, vget_low_s16(mul_value_0));
                out_value_neon_1 = vaddw_s16(out_value_neon_1, vget_high_s16(mul_value_0));
                out_value_neon_2 = vaddw_s16(out_value_neon_2, vget_low_s16(mul_value_1));
                out_value_neon_3 = vaddw_s16(out_value_neon_3, vget_high_s16(mul_value_1));
                weigth_neon_ptr++;

                ptr = ptr_2;
                in_value_neon = vld2q_s8(ptr);
                mul_value_0 = vmull_s8(vget_low_s8(in_value_neon.val[0]), *weigth_neon_ptr);
                mul_value_1 = vmull_s8(vget_high_s8(in_value_neon.val[0]), *weigth_neon_ptr);
                weigth_neon_ptr++;
                mul_value_0 = vmlal_s8(mul_value_0, vget_low_s8(in_value_neon.val[1]), *weigth_neon_ptr);
                mul_value_1 = vmlal_s8(mul_value_1, vget_high_s8(in_value_neon.val[1]), *weigth_neon_ptr);
                out_value_neon_0 = vaddw_s16(out_value_neon_0, vget_low_s16(mul_value_0));
                out_value_neon_1 = vaddw_s16(out_value_neon_1, vget_high_s16(mul_value_0));
                out_value_neon_2 = vaddw_s16(out_value_neon_2, vget_low_s16(mul_value_1));
                out_value_neon_3 = vaddw_s16(out_value_neon_3, vget_high_s16(mul_value_1));
                weigth_neon_ptr++;
                ptr += 2;
                in_value_neon = vld2q_s8(ptr);
                mul_value_0 = vmull_s8(vget_low_s8(in_value_neon.val[0]), *weigth_neon_ptr);
                mul_value_1 = vmull_s8(vget_high_s8(in_value_neon.val[0]), *weigth_neon_ptr);
                out_value_neon_0 = vaddw_s16(out_value_neon_0, vget_low_s16(mul_value_0));
                out_value_neon_1 = vaddw_s16(out_value_neon_1, vget_high_s16(mul_value_0));
                out_value_neon_2 = vaddw_s16(out_value_neon_2, vget_low_s16(mul_value_1));
                out_value_neon_3 = vaddw_s16(out_value_neon_3, vget_high_s16(mul_value_1));
                weigth_neon_ptr++;

                ptr_0 += expand_size;
                ptr_1 += expand_size;
                ptr_2 += expand_size;
            }

            out_value_neon_0 = vmulq_n_s32(out_value_neon_0, adjust_div_value);
            out_value_neon_1 = vmulq_n_s32(out_value_neon_1, adjust_div_value);
            out_value_neon_0 = vaddq_s32(out_value_neon_0, norm_half_add_value_neon);
            out_value_neon_1 = vaddq_s32(out_value_neon_1, norm_half_add_value_neon);
            out_value_neon_16Bit_0 = vshrn_n_s32(out_value_neon_0, data_norm_move_const);
            out_value_neon_16Bit_1 = vshrn_n_s32(out_value_neon_1, data_norm_move_const);
            out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
            out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon);
            vst1q_s16(p_out_buf + k, out_value_neon_16Bit);

             out_value_neon_0 = vmulq_n_s32(out_value_neon_2, adjust_div_value);
             out_value_neon_1 = vmulq_n_s32(out_value_neon_3, adjust_div_value);
             out_value_neon_0 = vaddq_s32(out_value_neon_0, norm_half_add_value_neon);
             out_value_neon_1 = vaddq_s32(out_value_neon_1, norm_half_add_value_neon);
             out_value_neon_16Bit_0 = vshrn_n_s32(out_value_neon_0, data_norm_move_const);
             out_value_neon_16Bit_1 = vshrn_n_s32(out_value_neon_1, data_norm_move_const);
             out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
             out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon);
             vst1q_s16(p_out_buf + k + 8, out_value_neon_16Bit);
        }
        if (k < output_width&&output_width >= 8)
        {
            int start_n[2] = { 0 };
            int end_loop_n = 0;
            start_n[0] = output_width - 8;
            if (k + 8 >= output_width)
                end_loop_n = 1;
            else
            {
                end_loop_n = 2;
                start_n[1] = k;
            }

            for (l = 0; l < end_loop_n; ++l)
            {
                int nn = start_n[l];
                out_value_neon_0 = vdupq_n_s32(0);
                out_value_neon_1 = vdupq_n_s32(0);
                n = j * 2 * expand_width + nn*2;

                int8x8_t *weigth_neon_ptr = weight_ptr_neon;
                FixedConvType *ptr_0 = expand_input_data + n;
                FixedConvType *ptr_1 = expand_input_data + n + expand_width;
                FixedConvType *ptr_2 = expand_input_data + n + expand_width * 2;

                for (m = 0; m < input_channel; ++m)
                {
                    FixedConvType *ptr = ptr_0;
                    in_value_neon_0 = vld2_s8(ptr);
                    mul_value_0 = vmull_s8(in_value_neon_0.val[0], *weigth_neon_ptr);
                    weigth_neon_ptr++;
                    mul_value_0 = vmlal_s8(mul_value_0, in_value_neon_0.val[1], *weigth_neon_ptr);
                    out_value_neon_0 = vaddw_s16(out_value_neon_0, vget_low_s16(mul_value_0));
                    out_value_neon_1 = vaddw_s16(out_value_neon_1, vget_high_s16(mul_value_0));
                    weigth_neon_ptr++;
                    ptr+=2;
                    in_value_neon_0 = vld2_s8(ptr);
                    mul_value_0 = vmull_s8(in_value_neon_0.val[0], *weigth_neon_ptr);
                    weigth_neon_ptr++;

                    ptr = ptr_1;
                    in_value_neon_0 = vld2_s8(ptr);
                    mul_value_0 = vmlal_s8(mul_value_0, in_value_neon_0.val[0], *weigth_neon_ptr);
                    out_value_neon_0 = vaddw_s16(out_value_neon_0, vget_low_s16(mul_value_0));
                    out_value_neon_1 = vaddw_s16(out_value_neon_1, vget_high_s16(mul_value_0));
                    weigth_neon_ptr++;
                    mul_value_0 = vmull_s8(in_value_neon_0.val[1], *weigth_neon_ptr);
                    weigth_neon_ptr++;
                    ptr+=2;
                    in_value_neon_0 = vld2_s8(ptr);
                    mul_value_0 = vmlal_s8(mul_value_0, in_value_neon_0.val[0], *weigth_neon_ptr);
                    out_value_neon_0 = vaddw_s16(out_value_neon_0, vget_low_s16(mul_value_0));
                    out_value_neon_1 = vaddw_s16(out_value_neon_1, vget_high_s16(mul_value_0));
                    weigth_neon_ptr++;

                    ptr = ptr_2;
                    in_value_neon_0 = vld2_s8(ptr);
                    mul_value_0 = vmull_s8(in_value_neon_0.val[0], *weigth_neon_ptr);
                    weigth_neon_ptr++;
                    mul_value_0 = vmlal_s8(mul_value_0, in_value_neon_0.val[1], *weigth_neon_ptr);
                    out_value_neon_0 = vaddw_s16(out_value_neon_0, vget_low_s16(mul_value_0));
                    out_value_neon_1 = vaddw_s16(out_value_neon_1, vget_high_s16(mul_value_0));
                    weigth_neon_ptr++;
                    ptr+=2;
                    in_value_neon_0 = vld2_s8(ptr);
                    mul_value_0 = vmull_s8(in_value_neon_0.val[0], *weigth_neon_ptr);
                    out_value_neon_0 = vaddw_s16(out_value_neon_0, vget_low_s16(mul_value_0));
                    out_value_neon_1 = vaddw_s16(out_value_neon_1, vget_high_s16(mul_value_0));
                    weigth_neon_ptr++;

                    ptr_0 += expand_size;
                    ptr_1 += expand_size;
                    ptr_2 += expand_size;
                }
                out_value_neon_0 = vmulq_n_s32(out_value_neon_0, adjust_div_value);
                out_value_neon_1 = vmulq_n_s32(out_value_neon_1, adjust_div_value);
                out_value_neon_0 = vaddq_s32(out_value_neon_0, norm_half_add_value_neon);
                out_value_neon_1 = vaddq_s32(out_value_neon_1, norm_half_add_value_neon);
                out_value_neon_16Bit_0 = vshrn_n_s32(out_value_neon_0, data_norm_move_const);
                out_value_neon_16Bit_1 = vshrn_n_s32(out_value_neon_1, data_norm_move_const);
                out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
                out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon);

                vst1q_s16(p_out_buf + nn, out_value_neon_16Bit);
            }
        }
        else
        {
            for (; k < output_width; ++k)
            {
                n = j * 2 * expand_width + k*2;
                FixedCalType value = 0;
                FixedConvType *ptr_0 = expand_input_data + n;
                FixedConvType *ptr_1 = expand_input_data + n + expand_width;
                FixedConvType *ptr_2 = expand_input_data + n + expand_width * 2;
                FixedConvType *weight_0 = weight_ptr;

                for (m = 0; m < input_channel; ++m)
                {
                    FixedConvType *ptr = ptr_0;
                    value += (*ptr)*(*weight_0); weight_0++;
                    ptr++;
                    value += (*ptr)*(*weight_0); weight_0++;
                    ptr++;
                    value += (*ptr)*(*weight_0); weight_0++;

                    ptr = ptr_1;
                    value += (*ptr)*(*weight_0); weight_0++;
                    ptr++;
                    value += (*ptr)*(*weight_0); weight_0++;
                    ptr++;
                    value += (*ptr)*(*weight_0); weight_0++;

                    ptr = ptr_2;
                    value += (*ptr)*(*weight_0); weight_0++;
                    ptr++;
                    value += (*ptr)*(*weight_0); weight_0++;
                    ptr++;
                    value += (*ptr)*(*weight_0); weight_0++;

                    ptr_0 += expand_size;
                    ptr_1 += expand_size;
                    ptr_2 += expand_size;
                }
                n += 2;
                p_out_buf[k] = ((value * adjust_div_value + norm_half_add_value) >> data_norm_move_const) + bias_value;
            }
        }
    }
    if (weight_ptr_neon)
        free(weight_ptr_neon);
#else
    for (j = 0; j < output_height; ++j)
	{
		n = j * 2 * expand_width;
		FixedType *p_out_buf = output_data + j*output_width;

		for (k = 0; k < output_width; ++k)
		{
			FixedCalType value = 0;
			FixedConvType *ptr_0 = expand_input_data + n;
			FixedConvType *ptr_1 = expand_input_data + n + expand_width;
			FixedConvType *ptr_2 = expand_input_data + n + expand_width * 2;
			FixedConvType *weight_0 = weight_ptr;

			for (m = 0; m < input_channel; ++m)
			{
				FixedConvType *ptr = ptr_0;
				value += (*ptr)*(*weight_0); weight_0++;
				ptr++;
				value += (*ptr)*(*weight_0); weight_0++;
				ptr++;
				value += (*ptr)*(*weight_0); weight_0++;

				ptr = ptr_1;
				value += (*ptr)*(*weight_0); weight_0++;
				ptr++;
				value += (*ptr)*(*weight_0); weight_0++;
				ptr++;
				value += (*ptr)*(*weight_0); weight_0++;

				ptr = ptr_2;
				value += (*ptr)*(*weight_0); weight_0++;
				ptr++;
				value += (*ptr)*(*weight_0); weight_0++;
				ptr++;
				value += (*ptr)*(*weight_0); weight_0++;

				ptr_0 += expand_size;
				ptr_1 += expand_size;
				ptr_2 += expand_size;
			}

			n += 2;
			p_out_buf[k] = ((value * adjust_div_value + norm_half_add_value) >> data_norm_move_const) + bias_value;
		}
	}
#endif
}
bool GetOneOutputChannelResult_Stride2_32Bit(FixedConvType *weight_ptr, FixedConvType **input_ptr, int input_channel, int kernel_size_sq, int w_stride_, int h_stride_,
                                             int expand_width, int expand_height, int output_width, int output_height, FixedType *p_in_out_buf, int pad_size, FixedType bias_value,
                                             int adjust_div_value, int norm_half_add_value)
{
    int i = 0, j = 0, k = 0, m = 0;

#ifdef EAGLEEYE_NEON_OPTIMIZATION
    int8x8_t *weight_ptr_neon = NULL;
    int8x8x2_t in_value_neon_8;
    int8x16x2_t in_value_neon_0, in_value_neon_1;
    int32x4_t out_value_neon_0, out_value_neon_1;
    int32x4_t out_value_neon_2, out_value_neon_3;
    int16x4_t out_value_neon_16Bit_0, out_value_neon_16Bit_1;
    int16x8_t mul_value_0, mul_value_1, out_value_neon_16Bit, fixed_bias_value_neon;
    int32x4_t norm_half_add_value_neon;
    int search_8_end = output_width/ 16 * 16;
    int loop_size = input_channel*kernel_size_sq;
    int loop_size_2 = loop_size / 2 * 2;
    int n = 0;
    const int data_norm_move_const = CNN_CONV_DATA_NORM_MOVE;

    weight_ptr_neon = (int8x8_t *)malloc(sizeof(int8x8_t)*loop_size);
    if (weight_ptr_neon == NULL)
        return false;

    norm_half_add_value_neon = vdupq_n_s32(norm_half_add_value);

    for (j = 0; j < loop_size; ++j)
        weight_ptr_neon[j] = vdup_n_s8(weight_ptr[j]);

    fixed_bias_value_neon = vdupq_n_s16(bias_value);
    for (j = 0; j < output_height; ++j)
    {
        n = j * h_stride_ * expand_width;
        FixedType *p_out_buf = p_in_out_buf + j*output_width;
        for (k = 0; k < search_8_end; k += 16)
        {
            out_value_neon_0 = vdupq_n_s32(0);
            out_value_neon_1 = vdupq_n_s32(0);
            out_value_neon_2 = vdupq_n_s32(0);
            out_value_neon_3 = vdupq_n_s32(0);
            for (m = 0; m < loop_size_2; m += 2)
            {
                in_value_neon_0 = vld2q_s8(input_ptr[m] + n);
                in_value_neon_1 = vld2q_s8(input_ptr[m + 1] + n);
                mul_value_0 = vmull_s8(vget_low_s8(in_value_neon_0.val[0]), weight_ptr_neon[m]);
                mul_value_1 = vmull_s8(vget_high_s8(in_value_neon_0.val[0]), weight_ptr_neon[m]);
                mul_value_0 = vmlal_s8(mul_value_0 ,vget_low_s8(in_value_neon_1.val[0]), weight_ptr_neon[m + 1]);
                mul_value_1 = vmlal_s8(mul_value_1, vget_high_s8(in_value_neon_1.val[0]), weight_ptr_neon[m + 1]);
                out_value_neon_0 = vaddw_s16(out_value_neon_0, vget_low_s16(mul_value_0));
                out_value_neon_1 = vaddw_s16(out_value_neon_1, vget_high_s16(mul_value_0));
                out_value_neon_2 = vaddw_s16(out_value_neon_2, vget_low_s16(mul_value_1));
                out_value_neon_3 = vaddw_s16(out_value_neon_3, vget_high_s16(mul_value_1));
            }
            for (; m < loop_size; ++m)
            {
                in_value_neon_0 = vld2q_s8(input_ptr[m] + n);
                mul_value_0 = vmull_s8(vget_low_s8(in_value_neon_0.val[0]), weight_ptr_neon[m]);
                mul_value_1 = vmull_s8(vget_high_s8(in_value_neon_0.val[0]), weight_ptr_neon[m]);
                out_value_neon_0 = vaddw_s16(out_value_neon_0, vget_low_s16(mul_value_0));
                out_value_neon_1 = vaddw_s16(out_value_neon_1, vget_high_s16(mul_value_0));
                out_value_neon_2 = vaddw_s16(out_value_neon_2, vget_low_s16(mul_value_1));
                out_value_neon_3 = vaddw_s16(out_value_neon_3, vget_high_s16(mul_value_1));
            }
            n += 32;
            out_value_neon_0 = vmulq_n_s32(out_value_neon_0, adjust_div_value);
            out_value_neon_1 = vmulq_n_s32(out_value_neon_1, adjust_div_value);
            out_value_neon_0 = vaddq_s32(out_value_neon_0, norm_half_add_value_neon);
            out_value_neon_1 = vaddq_s32(out_value_neon_1, norm_half_add_value_neon);
            out_value_neon_16Bit_0 = vshrn_n_s32(out_value_neon_0, data_norm_move_const);
            out_value_neon_16Bit_1 = vshrn_n_s32(out_value_neon_1, data_norm_move_const);
            out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
            out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon);
            vst1q_s16(p_out_buf + k, out_value_neon_16Bit);

            out_value_neon_0 = vmulq_n_s32(out_value_neon_2, adjust_div_value);
            out_value_neon_1 = vmulq_n_s32(out_value_neon_3, adjust_div_value);
            out_value_neon_0 = vaddq_s32(out_value_neon_0, norm_half_add_value_neon);
            out_value_neon_1 = vaddq_s32(out_value_neon_1, norm_half_add_value_neon);
            out_value_neon_16Bit_0 = vshrn_n_s32(out_value_neon_0, data_norm_move_const);
            out_value_neon_16Bit_1 = vshrn_n_s32(out_value_neon_1, data_norm_move_const);
            out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
            out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon);
            vst1q_s16(p_out_buf + k + 8, out_value_neon_16Bit);
        }
//        if (k < output_width&&output_width >= 16)
//        {
//            int start_n[2] = { 0 };
//            int end_loop_n = 0;
//            start_n[0] = output_width - 8;
//            if (j + 8 >= output_width)
//                end_loop_n = 1;
//            else
//            {
//                end_loop_n = 2;
//                start_n[1] = j;
//            }
//
//            for (k = 0; k < end_loop_n; ++k)
//            {
//                int nn = start_n[k];
//                out_value_neon_0 = vdupq_n_s32(0);
//                out_value_neon_1 = vdupq_n_s32(0);
//                n = j * h_stride_ * expand_width + nn*2;
//                for (m = 0; m < loop_size; ++m)
//                {
//                    in_value_neon_8 = vld2_s8(input_ptr[m] + n);
//                    mul_value_0 = vmull_s8(in_value_neon_8.val[0], weight_ptr_neon[m]);
//                    out_value_neon_0 = vaddw_s16(out_value_neon_0, vget_low_s16(mul_value_0));
//                    out_value_neon_1 = vaddw_s16(out_value_neon_1, vget_high_s16(mul_value_0));
//                }
//                out_value_neon_0 = vmulq_n_s32(out_value_neon_0, adjust_div_value);
//                out_value_neon_1 = vmulq_n_s32(out_value_neon_1, adjust_div_value);
//                out_value_neon_0 = vaddq_s32(out_value_neon_0, norm_half_add_value_neon);
//                out_value_neon_1 = vaddq_s32(out_value_neon_1, norm_half_add_value_neon);
//                out_value_neon_16Bit_0 = vshrn_n_s32(out_value_neon_0, data_norm_move_const);
//                out_value_neon_16Bit_1 = vshrn_n_s32(out_value_neon_1, data_norm_move_const);
//                out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
//                out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon);
//
//                vst1q_s16(p_out_buf + nn, out_value_neon_16Bit);
//            }
//        }
//        else
        {
            for (; k < output_width; ++k)
            {
                FixedCalType value = 0;
                for (m = 0; m < loop_size; ++m)
                    value += input_ptr[m][n] * weight_ptr[m];
                n += w_stride_;
                p_out_buf[k] = ((value * adjust_div_value + norm_half_add_value) >> data_norm_move_const) + bias_value;
            }
        }
    }

    EAGLEEYE_SAFEFREE(weight_ptr_neon);
#else
    int loop_size = input_channel*kernel_size_sq;
    int n = 0;
    const int data_norm_move_const = CNN_CONV_DATA_NORM_MOVE;

    for (j = 0; j < output_height; ++j)
    {
        n = j * h_stride_ * expand_width;
        FixedType *p_out_buf = p_in_out_buf + j*output_width;
        for (k = 0; k < output_width; ++k)
        {
            FixedCalType value = 0;
            for (m = 0; m < loop_size; ++m)
                value += input_ptr[m][n] * weight_ptr[m];
            n += w_stride_;
            p_out_buf[k] = ((value * adjust_div_value + norm_half_add_value) >> data_norm_move_const) + bias_value;
        }
    }
#endif

    return true;
}
bool im2col_cpu_Fixed(FixedConvType** input_ptr, int channels, int width, int height, int out_width, int out_height, int kernel_w, int kernel_h,
                      int pad_w, int pad_h,
                      int stride_w, int stride_h,
                      int dilation_,
                      FixedConvType* data_col)
{
    int j = 0, k = 0;
    int m = 0, l = 0, n = 0;
    int kernel_size_sq = kernel_w*kernel_h;

    n = 0;
    FixedConvType *p_out_data = data_col;
    for (j = 0; j < out_height; ++j)
    {
        n = j*stride_h*width;
        for (k = 0; k < out_width; ++k)
        {
            for (m = 0; m < channels*kernel_size_sq; ++m)
            {
                (*p_out_data) = input_ptr[m][n];
                p_out_data++;
            }

            n += stride_w;
        }
    }

    return true;
}

void ConvMatrixMulti_32Bit(FixedConvType *fixed_weight, int row1, int col1, FixedConvType *data_col, int row2, int col2, FixedType *net_output_data,
                           int *adjust_div_value, int norm_half_add_value, FixedBiasType *fixed_bias_value)
{
#ifdef EAGLEEYE_NEON_OPTIMIZATION
    int i = 0, j = 0, k = 0;
    int8x8_t in_value_neon_m1_0, in_value_neon_m1_1, in_value_neon_m1_2, in_value_neon_m1_3, in_value_neon_m2_0, in_value_neon_m2_1;
    int16x8_t mul_value_neon;
    int32x4_t out_value_neon_0, out_value_neon_1, out_value_neon_2, out_value_neon_3, sum_value_neon;
    int64x2_t add_value;
    int search_8_end = col1 / 8 * 8;
    int search_16_end = col1 / 16 * 16;
    int search_row_end = row1 / 4 * 4;
    signed char *p1_0, *p2_0, *p1_1, *p1_2, *p1_3;
    const int data_norm_move_const = CNN_CONV_DATA_NORM_MOVE;

    for (i = 0; i < search_row_end; i += 4)
        for (j = 0; j < col2; ++j)
        {
            p1_0 = fixed_weight + i*col1;
            p1_1 = p1_0 + col1;
            p1_2 = p1_1 + col1;
            p1_3 = p1_2 + col1;
            p2_0 = data_col + j*row2;
            out_value_neon_0 = vdupq_n_s32(0);
            out_value_neon_1 = vdupq_n_s32(0);
            out_value_neon_2 = vdupq_n_s32(0);
            out_value_neon_3 = vdupq_n_s32(0);
            for (k = 0; k < search_16_end; k += 16)
            {
                in_value_neon_m2_0 = vld1_s8(p2_0);
                in_value_neon_m2_1 = vld1_s8(p2_0 + 8);

                in_value_neon_m1_0 = vld1_s8(p1_0);
                mul_value_neon = vmull_s8(in_value_neon_m1_0, in_value_neon_m2_0);
                in_value_neon_m1_0 = vld1_s8(p1_0 + 8);
                mul_value_neon = vmlal_s8(mul_value_neon, in_value_neon_m1_0, in_value_neon_m2_1);
                sum_value_neon = vpaddlq_s16(mul_value_neon);
                out_value_neon_0 = vaddq_s32(out_value_neon_0, sum_value_neon);
                in_value_neon_m1_1 = vld1_s8(p1_1);
                mul_value_neon = vmull_s8(in_value_neon_m1_1, in_value_neon_m2_0);
                in_value_neon_m1_1 = vld1_s8(p1_1 + 8);
                mul_value_neon = vmlal_s8(mul_value_neon, in_value_neon_m1_1, in_value_neon_m2_1);
                sum_value_neon = vpaddlq_s16(mul_value_neon);
                out_value_neon_1 = vaddq_s32(out_value_neon_1, sum_value_neon);
                in_value_neon_m1_2 = vld1_s8(p1_2);
                mul_value_neon = vmull_s8(in_value_neon_m1_2, in_value_neon_m2_0);
                in_value_neon_m1_2 = vld1_s8(p1_2 + 8);
                mul_value_neon = vmlal_s8(mul_value_neon, in_value_neon_m1_2, in_value_neon_m2_1);
                sum_value_neon = vpaddlq_s16(mul_value_neon);
                out_value_neon_2 = vaddq_s32(out_value_neon_2, sum_value_neon);
                in_value_neon_m1_3 = vld1_s8(p1_3);
                mul_value_neon = vmull_s8(in_value_neon_m1_3, in_value_neon_m2_0);
                in_value_neon_m1_3 = vld1_s8(p1_3 + 8);
                mul_value_neon = vmlal_s8(mul_value_neon, in_value_neon_m1_3, in_value_neon_m2_1);
                sum_value_neon = vpaddlq_s16(mul_value_neon);
                out_value_neon_3 = vaddq_s32(out_value_neon_3, sum_value_neon);
                p1_0 += 16;
                p1_1 += 16;
                p1_2 += 16;
                p1_3 += 16;
                p2_0 += 16;
            }
            for(;k < search_8_end; k+=8)
            {
                in_value_neon_m2_0 = vld1_s8(p2_0);
                in_value_neon_m1_0 = vld1_s8(p1_0);
                mul_value_neon = vmull_s8(in_value_neon_m1_0, in_value_neon_m2_0);
                sum_value_neon = vpaddlq_s16(mul_value_neon);
                out_value_neon_0 = vaddq_s32(out_value_neon_0, sum_value_neon);
                in_value_neon_m1_1 = vld1_s8(p1_1);
                mul_value_neon = vmull_s8(in_value_neon_m1_1, in_value_neon_m2_0);
                sum_value_neon = vpaddlq_s16(mul_value_neon);
                out_value_neon_1 = vaddq_s32(out_value_neon_1, sum_value_neon);
                in_value_neon_m1_2 = vld1_s8(p1_2);
                mul_value_neon = vmull_s8(in_value_neon_m1_2, in_value_neon_m2_0);
                sum_value_neon = vpaddlq_s16(mul_value_neon);
                out_value_neon_2 = vaddq_s32(out_value_neon_2, sum_value_neon);
                in_value_neon_m1_3 = vld1_s8(p1_3);
                mul_value_neon = vmull_s8(in_value_neon_m1_3, in_value_neon_m2_0);
                sum_value_neon = vpaddlq_s16(mul_value_neon);
                out_value_neon_3 = vaddq_s32(out_value_neon_3, sum_value_neon);
                p1_0 += 8;
                p1_1 += 8;
                p1_2 += 8;
                p1_3 += 8;
                p2_0 += 8;
            }
            int sum_0 = 0;
            int sum_1 = 0;
            int sum_2 = 0;
            int sum_3 = 0;

            add_value = vpaddlq_s32(out_value_neon_0);
            sum_0 = vgetq_lane_s64(add_value, 0) +vgetq_lane_s64(add_value, 1);
            add_value = vpaddlq_s32(out_value_neon_1);
            sum_1 = vgetq_lane_s64(add_value, 0) +vgetq_lane_s64(add_value, 1);
            add_value = vpaddlq_s32(out_value_neon_2);
            sum_2 = vgetq_lane_s64(add_value, 0) +vgetq_lane_s64(add_value, 1);
            add_value = vpaddlq_s32(out_value_neon_3);
            sum_3 = vgetq_lane_s64(add_value, 0) +vgetq_lane_s64(add_value, 1);

            for (; k < col1; ++k)
            {
                sum_0 += (*p1_0)*(*p2_0);
                sum_1 += (*p1_1)*(*p2_0);
                sum_2 += (*p1_2)*(*p2_0);
                sum_3 += (*p1_3)*(*p2_0);
                p1_0++;
                p1_1++;
                p1_2++;
                p1_3++;
                p2_0++;
            }
            net_output_data[i*col2 + j] = ((sum_0 * adjust_div_value[i] + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[i];
            net_output_data[(i + 1)*col2 + j] = ((sum_1 * adjust_div_value[i + 1] + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[i + 1];
            net_output_data[(i + 2)*col2 + j] = ((sum_2 * adjust_div_value[i + 2] + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[i + 2];
            net_output_data[(i + 3)*col2 + j] = ((sum_3 * adjust_div_value[i + 3] + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[i + 3];
        }
    for (; i < row1; i++)
        for (j = 0; j < col2; j++)
        {
            p1_0 = fixed_weight + i*col1;
            p2_0 = data_col + j*row2;
            out_value_neon_0 = vdupq_n_s32(0);
            for (k = 0; k < search_8_end; k += 8)
            {
                in_value_neon_m1_0 = vld1_s8(p1_0);
                in_value_neon_m2_0 = vld1_s8(p2_0);
                mul_value_neon = vmull_s8(in_value_neon_m1_0, in_value_neon_m2_0);
                sum_value_neon = vpaddlq_s16(mul_value_neon);
                out_value_neon_0 = vaddq_s32(out_value_neon_0, sum_value_neon);
                p1_0 += 8;
                p2_0 += 8;
            }
            int sum_0 = 0;
            add_value = vpaddlq_s32(out_value_neon_0);
            sum_0 = vgetq_lane_s64(add_value, 0) +vgetq_lane_s64(add_value, 1);
            for (; k < col1; ++k)
            {
                sum_0 += (*p1_0)*(*p2_0);
                p1_0++;
                p2_0++;
            }
            net_output_data[i*col2 + j] = ((sum_0 * adjust_div_value[i] + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[i];
        }
#endif
}
bool RunConvForward_KernelLoop(FixedConvType *expand_input_data, int input_channel, int expand_width, int expand_height, FixedConvType *fixed_weight,
                               FixedType *net_output_data, int output_channel, int output_width, int output_height, int kernel_size, int pad_size, int w_stride_, int h_stride_,
                               FixedBiasType *fixed_bias_value, int *adjust_div_value, int norm_half_add_value, int bool_16Bit_32Bit_Add)
{
    int i = 0, j = 0, m = 0, l = 0, k = 0, n = 0;
    bool bRet = false;
    FixedConvType *data_col = NULL;
    int row1 = output_channel;
    int col1 = kernel_size*kernel_size*input_channel;
    int row2 = col1;
    int col2 = output_width*output_height;
    FixedConvType **input_ptr = NULL;
    const int  data_norm_move_const = CNN_CONV_DATA_NORM_MOVE;

    input_ptr = (FixedConvType **)malloc(sizeof(FixedConvType *)*kernel_size*kernel_size*input_channel);
    data_col = (FixedConvType *)malloc(sizeof(FixedConvType)*row2*col2);
    if (data_col == NULL || input_ptr == NULL)
    {
        EAGLEEYE_SAFEFREE(data_col);
        EAGLEEYE_SAFEFREE(input_ptr);
        return false;
    }

    n = 0;
    for (j = 0; j < input_channel; ++j)
    {
        for (m = 0; m < kernel_size; ++m)
            for (l = 0; l < kernel_size; ++l)
            {
                input_ptr[n] = expand_input_data + j*expand_width*expand_height + m*expand_width + l;
                n++;
            }
    }

    bRet = im2col_cpu_Fixed(input_ptr, input_channel, expand_width, expand_height, output_width, output_height, kernel_size, kernel_size,
                            pad_size, pad_size, w_stride_, h_stride_, 1, data_col);


#ifdef EAGLEEYE_NEON_OPTIMIZATION
    if(bool_16Bit_32Bit_Add){
        ConvMatrixMulti_16Bit(fixed_weight, row1, col1, data_col, row2, col2, net_output_data, adjust_div_value, norm_half_add_value, fixed_bias_value);
    }
    else{
        ConvMatrixMulti_32Bit(fixed_weight, row1, col1, data_col, row2, col2, net_output_data, adjust_div_value, norm_half_add_value, fixed_bias_value);
    }
#else
    #ifdef CNN_RECOGNITION_SSE_PROCESSING
	//MatrixTranspose(data_col, row2, col2, NULL);
	__m128i in_value_sse_m1_0, in_value_sse_m1_1, in_value_sse_m1_2, in_value_sse_m1_3, in_value_sse_m2, mul_value;
	__m128i out_value_sse_0, out_value_sse_1, out_value_sse_2, out_value_sse_3;
	int search_8_end = col1 / 8 * 8;
	int search_row_end = row1 / 4 * 4;
	signed char *p1_0, *p2_0, *p1_1, *p1_2, *p1_3;
	int out_value[4];

	for (i = 0; i < search_row_end; i += 4)
	for (j = 0; j < col2; j++)
	{
		out_value_sse_0 = _mm_set1_epi32(0);
		out_value_sse_1 = _mm_set1_epi32(0);
		out_value_sse_2 = _mm_set1_epi32(0);
		out_value_sse_3 = _mm_set1_epi32(0);
		p1_0 = fixed_weight + i*col1;
		p1_1 = p1_0 + col1;
		p1_2 = p1_1 + col1;
		p1_3 = p1_2 + col1;
		p2_0 = data_col + j*row2;
		for (k = 0; k < search_8_end; k += 8)
		{
			in_value_sse_m1_0 = _mm_loadu_si128((__m128i *)p1_0);
			in_value_sse_m1_0 = _mm_cvtepi8_epi16(in_value_sse_m1_0);
			in_value_sse_m1_1 = _mm_loadu_si128((__m128i *)p1_1);
			in_value_sse_m1_1 = _mm_cvtepi8_epi16(in_value_sse_m1_1);
			in_value_sse_m1_2 = _mm_loadu_si128((__m128i *)p1_2);
			in_value_sse_m1_2 = _mm_cvtepi8_epi16(in_value_sse_m1_2);
			in_value_sse_m1_3 = _mm_loadu_si128((__m128i *)p1_3);
			in_value_sse_m1_3 = _mm_cvtepi8_epi16(in_value_sse_m1_3);
			in_value_sse_m2 = _mm_loadu_si128((__m128i *)p2_0);
			in_value_sse_m2 = _mm_cvtepi8_epi16(in_value_sse_m2);
			mul_value = _mm_madd_epi16(in_value_sse_m1_0, in_value_sse_m2);
			out_value_sse_0 = _mm_add_epi32(out_value_sse_0, mul_value);
			mul_value = _mm_madd_epi16(in_value_sse_m1_1, in_value_sse_m2);
			out_value_sse_1 = _mm_add_epi32(out_value_sse_1, mul_value);
			mul_value = _mm_madd_epi16(in_value_sse_m1_2, in_value_sse_m2);
			out_value_sse_2 = _mm_add_epi32(out_value_sse_2, mul_value);
			mul_value = _mm_madd_epi16(in_value_sse_m1_3, in_value_sse_m2);
			out_value_sse_3 = _mm_add_epi32(out_value_sse_3, mul_value);

			p1_0 += 8;
			p1_1 += 8;
			p1_2 += 8;
			p1_3 += 8;
			p2_0 += 8;
		}
		int sum_0 = 0;
		int sum_1 = 0;
		int sum_2 = 0;
		int sum_3 = 0;
		_mm_storeu_si128((__m128i *)out_value, out_value_sse_0);
		sum_0 = out_value[0] + out_value[1] + out_value[2] + out_value[3];
		_mm_storeu_si128((__m128i *)out_value, out_value_sse_1);
		sum_1 = out_value[0] + out_value[1] + out_value[2] + out_value[3];
		_mm_storeu_si128((__m128i *)out_value, out_value_sse_2);
		sum_2 = out_value[0] + out_value[1] + out_value[2] + out_value[3];
		_mm_storeu_si128((__m128i *)out_value, out_value_sse_3);
		sum_3 = out_value[0] + out_value[1] + out_value[2] + out_value[3];
		for (; k < col1; ++k)
		{
			sum_0 += (*p1_0)*(*p2_0);
			sum_1 += (*p1_1)*(*p2_0);
			sum_2 += (*p1_2)*(*p2_0);
			sum_3 += (*p1_3)*(*p2_0);
			p1_0++;
			p1_1++;
			p1_2++;
			p1_3++;
			p2_0++;
		}
		net_output_data[i*col2 + j] = ((sum_0 * adjust_div_value[i] + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[i];
		net_output_data[(i + 1)*col2 + j] = ((sum_1 * adjust_div_value[i + 1] + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[i + 1];
		net_output_data[(i + 2)*col2 + j] = ((sum_2 * adjust_div_value[i + 2] + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[i + 2];
		net_output_data[(i + 3)*col2 + j] = ((sum_3 * adjust_div_value[i + 3] + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[i + 3];
	}
	for (; i < row1; i++)
	for (j = 0; j < col2; j++)
	{
		out_value_sse_0 = _mm_set1_epi32(0);
		p1_0 = fixed_weight + i*col1;
		p2_0 = data_col + j*row2;
		for (k = 0; k < search_8_end; k += 8)
		{
			in_value_sse_m1_0 = _mm_loadu_si128((__m128i *)p1_0);
			in_value_sse_m1_0 = _mm_cvtepi8_epi16(in_value_sse_m1_0);
			in_value_sse_m2 = _mm_loadu_si128((__m128i *)p2_0);
			in_value_sse_m2 = _mm_cvtepi8_epi16(in_value_sse_m2);
			mul_value = _mm_madd_epi16(in_value_sse_m1_0, in_value_sse_m2);
			out_value_sse_0 = _mm_add_epi32(out_value_sse_0, mul_value);
			p1_0 += 8;
			p2_0 += 8;
		}
		int sum_0 = 0;
		_mm_storeu_si128((__m128i *)out_value, out_value_sse_0);
		sum_0 += out_value[0] + out_value[1] + out_value[2] + out_value[3];
		for (; k < col1; ++k)
		{
			sum_0 += (*p1_0)*(*p2_0);
			p1_0++;
			p2_0++;
		}

		net_output_data[i*col2 + j] = ((sum_0 * adjust_div_value[i] + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[i];
	}

#else
	if (bool_16Bit_32Bit_Add)
	{
		for (i = 0; i < row1; ++i)
		for (j = 0; j < col2; ++j)
		{
			short sum = 0;
			for (k = 0; k < col1; ++k)
				sum += fixed_weight[i*col1 + k] * data_col[j*row2 + k];
			net_output_data[i*col2 + j] = ((sum * adjust_div_value[i] + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[i];
		}
	}
	else
	{
		for (i = 0; i < row1; ++i)
		for (j = 0; j < col2; ++j)
		{
			int sum = 0;
			for (k = 0; k < col1; ++k)
				sum += fixed_weight[i*col1 + k] * data_col[j*row2 + k];
			net_output_data[i*col2 + j] = ((sum * adjust_div_value[i] + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[i];
		}
	}
	
#endif
#endif
    EAGLEEYE_SAFEFREE(data_col);
    EAGLEEYE_SAFEFREE(input_ptr);
    return true;
}

#ifdef CNN_RECOGNITION_SSE_PROCESSING
inline void DoOneTimeProcess(signed char **input_ptr, int n, int loop_size, short *p_out_buf_0, short *p_out_buf_1, __m128i *weight_sse_0, __m128i *weight_sse_1,
	__m128i fixed_bias_sse_0, __m128i fixed_bias_sse_1, __m128i adjust_div_value_sse_0, __m128i adjust_div_value_sse_1, __m128i norm_half_add_value_sse, int data_norm_move)
{
	__m128i in_value_sse, in_value_sse_0, in_value_sse_1;
	__m128i combine_value_sse_0, combine_value_sse_1;
	__m128i multi_value_sse_0, multi_value_sse_1;
	__m128i out_value_sse0_0, out_value_sse0_1;
	__m128i out_value_sse1_0, out_value_sse1_1;
	__m128i zero_value_sse = _mm_set1_epi32(0);
	int m = 0;
	int loop_size_2 = loop_size / 2 * 2;

	out_value_sse0_0 = _mm_set1_epi32(0);
	out_value_sse0_1 = _mm_set1_epi32(0);
	out_value_sse1_0 = _mm_set1_epi32(0);
	out_value_sse1_1 = _mm_set1_epi32(0);

	for (m = 0; m < loop_size_2; m += 2)//一次处理两行
	{
		in_value_sse_0 = _mm_loadu_si128((__m128i *)(input_ptr[m] + n));
		in_value_sse_0 = _mm_cvtepi8_epi16(in_value_sse_0);
		in_value_sse_1 = _mm_loadu_si128((__m128i *)(input_ptr[m + 1] + n));
		in_value_sse_1 = _mm_cvtepi8_epi16(in_value_sse_1);
		combine_value_sse_0 = _mm_unpacklo_epi16(in_value_sse_0, in_value_sse_1);
		combine_value_sse_1 = _mm_unpackhi_epi16(in_value_sse_0, in_value_sse_1);

		multi_value_sse_0 = _mm_madd_epi16(combine_value_sse_0, weight_sse_0[m]);
		multi_value_sse_1 = _mm_madd_epi16(combine_value_sse_1, weight_sse_0[m]);
		out_value_sse0_0 = _mm_add_epi32(out_value_sse0_0, multi_value_sse_0);
		out_value_sse0_1 = _mm_add_epi32(out_value_sse0_1, multi_value_sse_1);

		multi_value_sse_0 = _mm_madd_epi16(combine_value_sse_0, weight_sse_1[m]);
		multi_value_sse_1 = _mm_madd_epi16(combine_value_sse_1, weight_sse_1[m]);
		out_value_sse1_0 = _mm_add_epi32(out_value_sse1_0, multi_value_sse_0);
		out_value_sse1_1 = _mm_add_epi32(out_value_sse1_1, multi_value_sse_1);
	}
	for (; m < loop_size; ++m)
	{
		in_value_sse = _mm_loadu_si128((__m128i *)(input_ptr[m] + n));
		in_value_sse = _mm_cvtepi8_epi16(in_value_sse);

		multi_value_sse_0 = _mm_mullo_epi16(in_value_sse, weight_sse_0[m]);
		in_value_sse_0 = _mm_cvtepi16_epi32(multi_value_sse_0);
		in_value_sse_1 = _mm_unpackhi_epi64(multi_value_sse_0, zero_value_sse);
		in_value_sse_1 = _mm_cvtepi16_epi32(in_value_sse_1);
		out_value_sse0_0 = _mm_add_epi32(out_value_sse0_0, in_value_sse_0);
		out_value_sse0_1 = _mm_add_epi32(out_value_sse0_1, in_value_sse_1);

		multi_value_sse_1 = _mm_mullo_epi16(in_value_sse, weight_sse_1[m]);
		in_value_sse_0 = _mm_cvtepi16_epi32(multi_value_sse_1);
		in_value_sse_1 = _mm_unpackhi_epi64(multi_value_sse_1, zero_value_sse);
		in_value_sse_1 = _mm_cvtepi16_epi32(in_value_sse_1);
		out_value_sse1_0 = _mm_add_epi32(out_value_sse1_0, in_value_sse_0);
		out_value_sse1_1 = _mm_add_epi32(out_value_sse1_1, in_value_sse_1);
	}
	/*
	_mm_storeu_si128((__m128i *)p_out_buf_0, out_value_sse0_0);
	_mm_storeu_si128((__m128i *)(p_out_buf_0 + 4), out_value_sse0_1);
	_mm_storeu_si128((__m128i *)p_out_buf_1, out_value_sse1_0);
	_mm_storeu_si128((__m128i *)(p_out_buf_1 + 4), out_value_sse1_1);*/

	out_value_sse0_0 = _mm_mullo_epi32(out_value_sse0_0, adjust_div_value_sse_0);
	out_value_sse0_0 = _mm_add_epi32(out_value_sse0_0, norm_half_add_value_sse);
	out_value_sse0_0 = _mm_srai_epi32(out_value_sse0_0, data_norm_move);
	out_value_sse0_0 = _mm_add_epi32(out_value_sse0_0, fixed_bias_sse_0);
	short *out_ptr = p_out_buf_0;
	*out_ptr = _mm_extract_epi16(out_value_sse0_0, 0); out_ptr++;
	*out_ptr = _mm_extract_epi16(out_value_sse0_0, 2); out_ptr++;
	*out_ptr = _mm_extract_epi16(out_value_sse0_0, 4); out_ptr++;
	*out_ptr = _mm_extract_epi16(out_value_sse0_0, 6); out_ptr++;
	out_value_sse0_1 = _mm_mullo_epi32(out_value_sse0_1, adjust_div_value_sse_0);
	out_value_sse0_1 = _mm_add_epi32(out_value_sse0_1, norm_half_add_value_sse);
	out_value_sse0_1 = _mm_srai_epi32(out_value_sse0_1, data_norm_move);
	out_value_sse0_1 = _mm_add_epi32(out_value_sse0_1, fixed_bias_sse_0);
	*out_ptr = _mm_extract_epi16(out_value_sse0_1, 0); out_ptr++;
	*out_ptr = _mm_extract_epi16(out_value_sse0_1, 2); out_ptr++;
	*out_ptr = _mm_extract_epi16(out_value_sse0_1, 4); out_ptr++;
	*out_ptr = _mm_extract_epi16(out_value_sse0_1, 6); out_ptr++;

	out_value_sse1_0 = _mm_mullo_epi32(out_value_sse1_0, adjust_div_value_sse_1);
	out_value_sse1_0 = _mm_add_epi32(out_value_sse1_0, norm_half_add_value_sse);
	out_value_sse1_0 = _mm_srai_epi32(out_value_sse1_0, data_norm_move);
	out_value_sse1_0 = _mm_add_epi32(out_value_sse1_0, fixed_bias_sse_1);
	out_ptr = p_out_buf_1;
	*out_ptr = _mm_extract_epi16(out_value_sse1_0, 0); out_ptr++;
	*out_ptr = _mm_extract_epi16(out_value_sse1_0, 2); out_ptr++;
	*out_ptr = _mm_extract_epi16(out_value_sse1_0, 4); out_ptr++;
	*out_ptr = _mm_extract_epi16(out_value_sse1_0, 6); out_ptr++;
	out_value_sse1_1 = _mm_mullo_epi32(out_value_sse1_1, adjust_div_value_sse_1);
	out_value_sse1_1 = _mm_add_epi32(out_value_sse1_1, norm_half_add_value_sse);
	out_value_sse1_1 = _mm_srai_epi32(out_value_sse1_1, data_norm_move);
	out_value_sse1_1 = _mm_add_epi32(out_value_sse1_1, fixed_bias_sse_1);
	*out_ptr = _mm_extract_epi16(out_value_sse1_1, 0); out_ptr++;
	*out_ptr = _mm_extract_epi16(out_value_sse1_1, 2); out_ptr++;
	*out_ptr = _mm_extract_epi16(out_value_sse1_1, 4); out_ptr++;
	*out_ptr = _mm_extract_epi16(out_value_sse1_1, 6); out_ptr++;
}

void ProcessTwoLine(signed char **input_ptr, signed char *fixed_weight, int row, int loop_size, int expand_width, int output_width, int output_height, int pad_size,
	short *net_output_data, short *temp_output_data_0, short *temp_output_data_1, __m128i *weight_sse_0, __m128i *weight_sse_1,
	short fixed_bias_0, int fixed_bias_1, int adjust_div_value_0, int adjust_div_value_1, int norm_half_add_value, int data_norm_move)
{
	int j = 0, m = 0;
	int search_img_start = 0;
	int search_img_end = output_height*expand_width - 2 * pad_size;
	int search_8_end = (search_img_end - search_img_start) / 8 * 8 + search_img_start;
	int loop_size_2 = loop_size / 2 * 2;
	__m128i fixed_bias_sse_0, fixed_bias_sse_1;
	__m128i adjust_div_value_sse_0, adjust_div_value_sse_1, norm_half_add_value_sse;

	adjust_div_value_sse_0 = _mm_set1_epi32(adjust_div_value_0);
	adjust_div_value_sse_1 = _mm_set1_epi32(adjust_div_value_1);
	norm_half_add_value_sse = _mm_set1_epi32(norm_half_add_value);
	fixed_bias_sse_0 = _mm_set1_epi32(fixed_bias_0);
	fixed_bias_sse_1 = _mm_set1_epi32(fixed_bias_1);

	signed char *weight_ptr_0 = fixed_weight + row*loop_size;
	signed char *weight_ptr_1 = fixed_weight + (row + 1)*loop_size;
	for (j = 0; j < loop_size_2; j += 2)
	{
		signed char v1 = weight_ptr_0[j];
		signed char v2 = weight_ptr_0[j + 1];
		weight_sse_0[j] = _mm_setr_epi16(v1, v2, v1, v2, v1, v2, v1, v2);
		v1 = weight_ptr_1[j];
		v2 = weight_ptr_1[j + 1];
		weight_sse_1[j] = _mm_setr_epi16(v1, v2, v1, v2, v1, v2, v1, v2);
	}

	for (; j < loop_size; ++j)
	{
		weight_sse_0[j] = _mm_set1_epi16(weight_ptr_0[j]);
		weight_sse_1[j] = _mm_set1_epi16(weight_ptr_1[j]);
	}

	short *p_out_buf_0 = temp_output_data_0;
	short *p_out_buf_1 = temp_output_data_1;

	for (j = search_img_start; j < search_8_end; j += 8)
	{
		DoOneTimeProcess(input_ptr, j, loop_size, p_out_buf_0 + j, p_out_buf_1 + j, weight_sse_0, weight_sse_1,
			fixed_bias_sse_0, fixed_bias_sse_1, adjust_div_value_sse_0, adjust_div_value_sse_1, norm_half_add_value_sse, data_norm_move);

	}

	if (j < search_img_end&&search_img_end - search_img_start >= 8)
	{
		j = search_img_end - 8;
		DoOneTimeProcess(input_ptr, j, loop_size, p_out_buf_0 + j, p_out_buf_1 + j, weight_sse_0, weight_sse_1,
			fixed_bias_sse_0, fixed_bias_sse_1, adjust_div_value_sse_0, adjust_div_value_sse_1, norm_half_add_value_sse, data_norm_move);
	}
	else
	{
		for (; j < search_img_end; ++j)
		{
			int value = 0;
			for (m = 0; m < loop_size; ++m)
				value += input_ptr[m][j] * weight_ptr_0[m];
			temp_output_data_0[j] = ((value * adjust_div_value_0 + norm_half_add_value) >> data_norm_move) + fixed_bias_0;
			value = 0;
			for (m = 0; m < loop_size; ++m)
				value += input_ptr[m][j] * weight_ptr_1[m];
			temp_output_data_1[j] = ((value * adjust_div_value_1 + norm_half_add_value) >> data_norm_move) + fixed_bias_1;
		}
	}

	short *p_ori_outbuf = net_output_data + row*output_height*output_width;

	for (j = 0; j < output_height; ++j)
		memcpy(p_ori_outbuf + j*output_width, temp_output_data_0 + j*expand_width, sizeof(short)*output_width);
	p_ori_outbuf = net_output_data + (row + 1)*output_height*output_width;
	for (j = 0; j < output_height; ++j)
		memcpy(p_ori_outbuf + j*output_width, temp_output_data_1 + j*expand_width, sizeof(short)*output_width);
}

void DoOneTimeProcess3x3(signed char *expand_input_data, int n, int expand_width, int expand_size, int input_channel, short *p_out_buf_0, short *p_out_buf_1, __m128i *weight_sse_0, __m128i *weight_sse_1,
	__m128i fixed_bias_sse_0, __m128i fixed_bias_sse_1, __m128i adjust_div_value_sse_0, __m128i adjust_div_value_sse_1, __m128i norm_half_add_value_sse, int data_norm_move)
{
	__m128i in_value_sse, in_value_sse_0, in_value_sse_1;
	__m128i combine_value_sse_0, combine_value_sse_1;
	__m128i multi_value_sse_0, multi_value_sse_1;
	__m128i out_value_sse0_0, out_value_sse0_1;
	__m128i out_value_sse1_0, out_value_sse1_1;
	__m128i zero_value_sse = _mm_set1_epi32(0);

	int m = 0;
	int weight_n = 0;

	out_value_sse0_0 = _mm_set1_epi32(0);
	out_value_sse0_1 = _mm_set1_epi32(0);
	out_value_sse1_0 = _mm_set1_epi32(0);
	out_value_sse1_1 = _mm_set1_epi32(0);

	signed char *ptr_0 = expand_input_data + n;
	signed char *ptr_1 = expand_input_data + n + expand_width;
	signed char *ptr_2 = expand_input_data + n + 2 * expand_width;

	for (m = 0; m < input_channel; ++m)
	{
		in_value_sse_0 = _mm_loadu_si128((__m128i *)ptr_0); ptr_0++;
		in_value_sse_0 = _mm_cvtepi8_epi16(in_value_sse_0);
		in_value_sse_1 = _mm_loadu_si128((__m128i *)ptr_0); ptr_0++;
		in_value_sse_1 = _mm_cvtepi8_epi16(in_value_sse_1);
		combine_value_sse_0 = _mm_unpacklo_epi16(in_value_sse_0, in_value_sse_1);
		combine_value_sse_1 = _mm_unpackhi_epi16(in_value_sse_0, in_value_sse_1);
		multi_value_sse_0 = _mm_madd_epi16(combine_value_sse_0, weight_sse_0[weight_n]);
		multi_value_sse_1 = _mm_madd_epi16(combine_value_sse_1, weight_sse_0[weight_n]);
		out_value_sse0_0 = _mm_add_epi32(out_value_sse0_0, multi_value_sse_0);
		out_value_sse0_1 = _mm_add_epi32(out_value_sse0_1, multi_value_sse_1);
		multi_value_sse_0 = _mm_madd_epi16(combine_value_sse_0, weight_sse_1[weight_n]);
		multi_value_sse_1 = _mm_madd_epi16(combine_value_sse_1, weight_sse_1[weight_n]);
		out_value_sse1_0 = _mm_add_epi32(out_value_sse1_0, multi_value_sse_0);
		out_value_sse1_1 = _mm_add_epi32(out_value_sse1_1, multi_value_sse_1);
		weight_n += 2;

		in_value_sse_0 = _mm_loadu_si128((__m128i *)ptr_0); ptr_0 += expand_size;
		in_value_sse_0 = _mm_cvtepi8_epi16(in_value_sse_0);
		in_value_sse_1 = _mm_loadu_si128((__m128i *)ptr_1); ptr_1++;
		in_value_sse_1 = _mm_cvtepi8_epi16(in_value_sse_1);
		combine_value_sse_0 = _mm_unpacklo_epi16(in_value_sse_0, in_value_sse_1);
		combine_value_sse_1 = _mm_unpackhi_epi16(in_value_sse_0, in_value_sse_1);
		multi_value_sse_0 = _mm_madd_epi16(combine_value_sse_0, weight_sse_0[weight_n]);
		multi_value_sse_1 = _mm_madd_epi16(combine_value_sse_1, weight_sse_0[weight_n]);
		out_value_sse0_0 = _mm_add_epi32(out_value_sse0_0, multi_value_sse_0);
		out_value_sse0_1 = _mm_add_epi32(out_value_sse0_1, multi_value_sse_1);
		multi_value_sse_0 = _mm_madd_epi16(combine_value_sse_0, weight_sse_1[weight_n]);
		multi_value_sse_1 = _mm_madd_epi16(combine_value_sse_1, weight_sse_1[weight_n]);
		out_value_sse1_0 = _mm_add_epi32(out_value_sse1_0, multi_value_sse_0);
		out_value_sse1_1 = _mm_add_epi32(out_value_sse1_1, multi_value_sse_1);
		weight_n += 2;

		in_value_sse_0 = _mm_loadu_si128((__m128i *)ptr_1); ptr_1++;
		in_value_sse_0 = _mm_cvtepi8_epi16(in_value_sse_0);
		in_value_sse_1 = _mm_loadu_si128((__m128i *)ptr_1);  ptr_1 += expand_size;
		in_value_sse_1 = _mm_cvtepi8_epi16(in_value_sse_1);
		combine_value_sse_0 = _mm_unpacklo_epi16(in_value_sse_0, in_value_sse_1);
		combine_value_sse_1 = _mm_unpackhi_epi16(in_value_sse_0, in_value_sse_1);
		multi_value_sse_0 = _mm_madd_epi16(combine_value_sse_0, weight_sse_0[weight_n]);
		multi_value_sse_1 = _mm_madd_epi16(combine_value_sse_1, weight_sse_0[weight_n]);
		out_value_sse0_0 = _mm_add_epi32(out_value_sse0_0, multi_value_sse_0);
		out_value_sse0_1 = _mm_add_epi32(out_value_sse0_1, multi_value_sse_1);
		multi_value_sse_0 = _mm_madd_epi16(combine_value_sse_0, weight_sse_1[weight_n]);
		multi_value_sse_1 = _mm_madd_epi16(combine_value_sse_1, weight_sse_1[weight_n]);
		out_value_sse1_0 = _mm_add_epi32(out_value_sse1_0, multi_value_sse_0);
		out_value_sse1_1 = _mm_add_epi32(out_value_sse1_1, multi_value_sse_1);
		weight_n += 2;

		in_value_sse_0 = _mm_loadu_si128((__m128i *)ptr_2); ptr_2++;
		in_value_sse_0 = _mm_cvtepi8_epi16(in_value_sse_0);
		in_value_sse_1 = _mm_loadu_si128((__m128i *)ptr_2);  ptr_2++;
		in_value_sse_1 = _mm_cvtepi8_epi16(in_value_sse_1);
		combine_value_sse_0 = _mm_unpacklo_epi16(in_value_sse_0, in_value_sse_1);
		combine_value_sse_1 = _mm_unpackhi_epi16(in_value_sse_0, in_value_sse_1);
		multi_value_sse_0 = _mm_madd_epi16(combine_value_sse_0, weight_sse_0[weight_n]);
		multi_value_sse_1 = _mm_madd_epi16(combine_value_sse_1, weight_sse_0[weight_n]);
		out_value_sse0_0 = _mm_add_epi32(out_value_sse0_0, multi_value_sse_0);
		out_value_sse0_1 = _mm_add_epi32(out_value_sse0_1, multi_value_sse_1);
		multi_value_sse_0 = _mm_madd_epi16(combine_value_sse_0, weight_sse_1[weight_n]);
		multi_value_sse_1 = _mm_madd_epi16(combine_value_sse_1, weight_sse_1[weight_n]);
		out_value_sse1_0 = _mm_add_epi32(out_value_sse1_0, multi_value_sse_0);
		out_value_sse1_1 = _mm_add_epi32(out_value_sse1_1, multi_value_sse_1);
		weight_n += 2;

		in_value_sse = _mm_loadu_si128((__m128i *)ptr_2); ptr_2 += expand_size;
		in_value_sse = _mm_cvtepi8_epi16(in_value_sse);
		multi_value_sse_0 = _mm_mullo_epi16(in_value_sse, weight_sse_0[weight_n]);
		in_value_sse_0 = _mm_cvtepi16_epi32(multi_value_sse_0);
		in_value_sse_1 = _mm_unpackhi_epi64(multi_value_sse_0, zero_value_sse);
		in_value_sse_1 = _mm_cvtepi16_epi32(in_value_sse_1);
		out_value_sse0_0 = _mm_add_epi32(out_value_sse0_0, in_value_sse_0);
		out_value_sse0_1 = _mm_add_epi32(out_value_sse0_1, in_value_sse_1);
		multi_value_sse_1 = _mm_mullo_epi16(in_value_sse, weight_sse_1[weight_n]);
		in_value_sse_0 = _mm_cvtepi16_epi32(multi_value_sse_1);
		in_value_sse_1 = _mm_unpackhi_epi64(multi_value_sse_1, zero_value_sse);
		in_value_sse_1 = _mm_cvtepi16_epi32(in_value_sse_1);
		out_value_sse1_0 = _mm_add_epi32(out_value_sse1_0, in_value_sse_0);
		out_value_sse1_1 = _mm_add_epi32(out_value_sse1_1, in_value_sse_1);
		weight_n++;
	}


	/*_mm_storeu_si128((__m128i *)p_out_buf_0, out_value_sse0_0);
	_mm_storeu_si128((__m128i *)(p_out_buf_0 + 4), out_value_sse0_1);
	_mm_storeu_si128((__m128i *)p_out_buf_1, out_value_sse1_0);
	_mm_storeu_si128((__m128i *)(p_out_buf_1 + 4), out_value_sse1_1);*/

	out_value_sse0_0 = _mm_mullo_epi32(out_value_sse0_0, adjust_div_value_sse_0);
	out_value_sse0_0 = _mm_add_epi32(out_value_sse0_0, norm_half_add_value_sse);
	out_value_sse0_0 = _mm_srai_epi32(out_value_sse0_0, data_norm_move);
	out_value_sse0_0 = _mm_add_epi32(out_value_sse0_0, fixed_bias_sse_0);
	short *out_ptr = p_out_buf_0;
	*out_ptr = _mm_extract_epi16(out_value_sse0_0, 0); out_ptr++;
	*out_ptr = _mm_extract_epi16(out_value_sse0_0, 2); out_ptr++;
	*out_ptr = _mm_extract_epi16(out_value_sse0_0, 4); out_ptr++;
	*out_ptr = _mm_extract_epi16(out_value_sse0_0, 6); out_ptr++;
	out_value_sse0_1 = _mm_mullo_epi32(out_value_sse0_1, adjust_div_value_sse_0);
	out_value_sse0_1 = _mm_add_epi32(out_value_sse0_1, norm_half_add_value_sse);
	out_value_sse0_1 = _mm_srai_epi32(out_value_sse0_1, data_norm_move);
	out_value_sse0_1 = _mm_add_epi32(out_value_sse0_1, fixed_bias_sse_0);
	*out_ptr = _mm_extract_epi16(out_value_sse0_1, 0); out_ptr++;
	*out_ptr = _mm_extract_epi16(out_value_sse0_1, 2); out_ptr++;
	*out_ptr = _mm_extract_epi16(out_value_sse0_1, 4); out_ptr++;
	*out_ptr = _mm_extract_epi16(out_value_sse0_1, 6); out_ptr++;

	out_value_sse1_0 = _mm_mullo_epi32(out_value_sse1_0, adjust_div_value_sse_1);
	out_value_sse1_0 = _mm_add_epi32(out_value_sse1_0, norm_half_add_value_sse);
	out_value_sse1_0 = _mm_srai_epi32(out_value_sse1_0, data_norm_move);
	out_value_sse1_0 = _mm_add_epi32(out_value_sse1_0, fixed_bias_sse_1);
	out_ptr = p_out_buf_1;
	*out_ptr = _mm_extract_epi16(out_value_sse1_0, 0); out_ptr++;
	*out_ptr = _mm_extract_epi16(out_value_sse1_0, 2); out_ptr++;
	*out_ptr = _mm_extract_epi16(out_value_sse1_0, 4); out_ptr++;
	*out_ptr = _mm_extract_epi16(out_value_sse1_0, 6); out_ptr++;
	out_value_sse1_1 = _mm_mullo_epi32(out_value_sse1_1, adjust_div_value_sse_1);
	out_value_sse1_1 = _mm_add_epi32(out_value_sse1_1, norm_half_add_value_sse);
	out_value_sse1_1 = _mm_srai_epi32(out_value_sse1_1, data_norm_move);
	out_value_sse1_1 = _mm_add_epi32(out_value_sse1_1, fixed_bias_sse_1);
	*out_ptr = _mm_extract_epi16(out_value_sse1_1, 0); out_ptr++;
	*out_ptr = _mm_extract_epi16(out_value_sse1_1, 2); out_ptr++;
	*out_ptr = _mm_extract_epi16(out_value_sse1_1, 4); out_ptr++;
	*out_ptr = _mm_extract_epi16(out_value_sse1_1, 6); out_ptr++;

}

void ProcessTwoLine3x3(signed char *expand_input_data, signed char **input_ptr, signed char *fixed_weight, int row, int loop_size, int input_channel, int expand_width, int expand_height, int output_width, int output_height, int pad_size,
	short *net_output_data, short *temp_output_data_0, short *temp_output_data_1, __m128i *weight_sse_0, __m128i *weight_sse_1,
	int fixed_bias_0, int fixed_bias_1, int adjust_div_value_0, int adjust_div_value_1, int norm_half_add_value, int data_norm_move)
{
	int j = 0, k = 0, m = 0, n = 0;
	int search_img_start = 0;
	int search_img_end = output_height*expand_width - 2 * pad_size;
	int search_8_end = (search_img_end - search_img_start) / 8 * 8 + search_img_start;
	int expand_size = expand_width*expand_height - 2;
	__m128i fixed_bias_sse_0, fixed_bias_sse_1;
	__m128i adjust_div_value_sse_0, adjust_div_value_sse_1, norm_half_add_value_sse;

	adjust_div_value_sse_0 = _mm_set1_epi32(adjust_div_value_0);
	adjust_div_value_sse_1 = _mm_set1_epi32(adjust_div_value_1);
	norm_half_add_value_sse = _mm_set1_epi32(norm_half_add_value);
	fixed_bias_sse_0 = _mm_set1_epi32(fixed_bias_0);
	fixed_bias_sse_1 = _mm_set1_epi32(fixed_bias_1);

	signed char *weight_ptr_0 = fixed_weight + row*loop_size;
	signed char *weight_ptr_1 = fixed_weight + (row + 1)*loop_size;
	for (j = 0; j < input_channel; j++)
	{
		for (k = 0; k < 4; ++k)
		{
			signed char v1 = weight_ptr_0[n];
			signed char v2 = weight_ptr_0[n + 1];
			weight_sse_0[n] = _mm_setr_epi16(v1, v2, v1, v2, v1, v2, v1, v2);
			v1 = weight_ptr_1[n];
			v2 = weight_ptr_1[n + 1];
			weight_sse_1[n] = _mm_setr_epi16(v1, v2, v1, v2, v1, v2, v1, v2);
			n += 2;
		}
		weight_sse_0[n] = _mm_set1_epi16(weight_ptr_0[n]);
		weight_sse_1[n] = _mm_set1_epi16(weight_ptr_1[n]);
		n++;
	}

	short *p_out_buf_0 = temp_output_data_0;
	short *p_out_buf_1 = temp_output_data_1;

	for (j = search_img_start; j < search_8_end; j += 8)
	{
		DoOneTimeProcess3x3(expand_input_data, j, expand_width, expand_size, input_channel, p_out_buf_0 + j, p_out_buf_1 + j, weight_sse_0, weight_sse_1,
			fixed_bias_sse_0, fixed_bias_sse_1, adjust_div_value_sse_0, adjust_div_value_sse_1, norm_half_add_value_sse, data_norm_move);
	}

	if (j < search_img_end&&search_img_end - search_img_start >= 8)
	{
		j = search_img_end - 8;
		DoOneTimeProcess3x3(expand_input_data, j, expand_width, expand_size, input_channel, p_out_buf_0 + j, p_out_buf_1 + j, weight_sse_0, weight_sse_1,
			fixed_bias_sse_0, fixed_bias_sse_1, adjust_div_value_sse_0, adjust_div_value_sse_1, norm_half_add_value_sse, data_norm_move);
	}
	else
	{
		for (; j < search_img_end; ++j)
		{
			int value = 0;
			for (m = 0; m < loop_size; ++m)
				value += input_ptr[m][j] * weight_ptr_0[m];
			temp_output_data_0[j] = ((value * adjust_div_value_0 + norm_half_add_value) >> data_norm_move) + fixed_bias_0;
			value = 0;
			for (m = 0; m < loop_size; ++m)
				value += input_ptr[m][j] * weight_ptr_1[m];
			temp_output_data_1[j] = ((value * adjust_div_value_1 + norm_half_add_value) >> data_norm_move) + fixed_bias_1;
		}
	}

	short *p_ori_outbuf = net_output_data + row*output_height*output_width;
	for (j = 0; j < output_height; ++j)
		memcpy(p_ori_outbuf + j*output_width, temp_output_data_0 + j*expand_width, sizeof(short)*output_width);
	p_ori_outbuf = net_output_data + (row + 1)*output_height*output_width;
	for (j = 0; j < output_height; ++j)
		memcpy(p_ori_outbuf + j*output_width, temp_output_data_1 + j*expand_width, sizeof(short)*output_width);

}


#endif
bool RunConvForward_ImageLoop(FixedConvType *expand_input_data, int input_channel, int expand_width, int expand_height, FixedConvType *fixed_weight,
                              FixedType *net_output_data, int output_channel, int output_width, int output_height, int kernel_size, int pad_size, int w_stride_, int h_stride_,
                              FixedBiasType *fixed_bias_value, int *adjust_div_value, int norm_half_add_value, int bool_16Bit_32Bit_Add)
{
    int i = 0, j = 0, k = 0, m = 0, l = 0, n = 0;
    int kernel_size_sq = kernel_size*kernel_size;
    FixedType *temp_output_data_0 = NULL;
    FixedType *temp_output_data_1 = NULL;
    FixedConvType **input_ptr = NULL;
    signed char *weight_ptr = NULL;
    const int  data_norm_move_const = CNN_CONV_DATA_NORM_MOVE;

    temp_output_data_0 = (FixedType *)malloc(sizeof(FixedType)*expand_width*output_height);
    temp_output_data_1 = (FixedType *)malloc(sizeof(FixedType)*expand_width*output_height);
    input_ptr = (FixedConvType **)malloc(sizeof(FixedConvType *)*kernel_size_sq*input_channel);
    if (temp_output_data_0 == NULL || temp_output_data_1 == NULL || input_ptr == NULL)
    {
        EAGLEEYE_SAFEFREE(temp_output_data_0);
        EAGLEEYE_SAFEFREE(temp_output_data_1);
        EAGLEEYE_SAFEFREE(input_ptr);
        return false;
    }
    n = 0;
    for (j = 0; j < input_channel; ++j)
    {
        for (m = 0; m < kernel_size; ++m)
            for (l = 0; l < kernel_size; ++l)
            {
                input_ptr[n] = expand_input_data + j*expand_height*expand_width + m*expand_width + l;
                n++;
            }
    }

#ifdef EAGLEEYE_NEON_OPTIMIZATION
    if (w_stride_ == 1 && h_stride_ == 1 && kernel_size == 1)
    {
        int loop_size = input_channel*kernel_size_sq;
        int output_channel_4 = output_channel / 4 * 4;
        FixedType *temp_output_data_2 = NULL;
        FixedType *temp_output_data_3 = NULL;

        temp_output_data_2 = (FixedType *)malloc(sizeof(FixedType)*expand_width*output_height);
        temp_output_data_3 = (FixedType *)malloc(sizeof(FixedType)*expand_width*output_height);
        if (temp_output_data_2 == NULL || temp_output_data_3 == NULL)
        {
            EAGLEEYE_SAFEFREE(temp_output_data_0);
            EAGLEEYE_SAFEFREE(temp_output_data_1);
            EAGLEEYE_SAFEFREE(temp_output_data_2);
            EAGLEEYE_SAFEFREE(temp_output_data_3);
            EAGLEEYE_SAFEFREE(input_ptr);
            return false;
        }
        for (i = 0; i < output_channel_4; i += 4)
        {
            if(bool_16Bit_32Bit_Add)
                GetMultiOutputChannelResult1x1_4_16Bit(fixed_weight, expand_input_data, input_channel, expand_width, expand_height, i, output_height,
                                                       temp_output_data_0, temp_output_data_1, temp_output_data_2, temp_output_data_3,
                                                       pad_size, fixed_bias_value,
                                                       adjust_div_value, norm_half_add_value);
            else
                GetMultiOutputChannelResult1x1_4_32Bit(fixed_weight, expand_input_data, input_channel, expand_width, expand_height, i, output_height,
                                                       temp_output_data_0, temp_output_data_1, temp_output_data_2, temp_output_data_3, pad_size, fixed_bias_value,
                                                       adjust_div_value, norm_half_add_value);
            FixedType *p_ori_outbuf = net_output_data + i*output_height*output_width;
            for (j = 0; j < output_height; ++j)
                memcpy(p_ori_outbuf + j*output_width, temp_output_data_0 + j*expand_width, sizeof(FixedType)*output_width);
            p_ori_outbuf = net_output_data + (i + 1)*output_height*output_width;
            for (j = 0; j < output_height; ++j)
                memcpy(p_ori_outbuf + j*output_width, temp_output_data_1 + j*expand_width, sizeof(FixedType)*output_width);
            p_ori_outbuf = net_output_data + (i + 2)*output_height*output_width;
            for (j = 0; j < output_height; ++j)
                memcpy(p_ori_outbuf + j*output_width, temp_output_data_2 + j*expand_width, sizeof(FixedType)*output_width);
            p_ori_outbuf = net_output_data + (i + 3)*output_height*output_width;
            for (j = 0; j < output_height; ++j)
                memcpy(p_ori_outbuf + j*output_width, temp_output_data_3 + j*expand_width, sizeof(FixedType)*output_width);
        }
        free(temp_output_data_2);
        free(temp_output_data_3);

        for (; i < output_channel; ++i)
        {
            weight_ptr = fixed_weight + i*loop_size;
            if(bool_16Bit_32Bit_Add)
                GetOneOutputChannelResult1x1_16Bit(weight_ptr, expand_input_data, input_channel, expand_width, expand_height, output_height,
                                                   temp_output_data_0, pad_size, fixed_bias_value[i],
                                                   adjust_div_value[i], norm_half_add_value);
            else
                GetOneOutputChannelResult1x1_32Bit(weight_ptr, expand_input_data, input_channel, expand_width, expand_height, output_height,
                                                   temp_output_data_0, pad_size, fixed_bias_value[i],
                                                   adjust_div_value[i], norm_half_add_value);
            FixedType *p_ori_outbuf = net_output_data + i*output_height*output_width;
            for (j = 0; j < output_height; ++j)
                memcpy(p_ori_outbuf + j*output_width, temp_output_data_0 + j*expand_width, sizeof(FixedType)*output_width);
        }
    }
    else if (w_stride_ == 1 && h_stride_ == 1 && kernel_size == 3)
    {
        int loop_size = input_channel*kernel_size_sq;
        int output_channel_2 = output_channel / 2 * 2;

        for (i = 0; i < output_channel_2; i += 2)
        {
            if(bool_16Bit_32Bit_Add)
                GetMultiOutputChannelResult3x3_2_16Bit(fixed_weight, expand_input_data, input_channel, expand_width, expand_height, i, output_height,
                                                       temp_output_data_0, temp_output_data_1, pad_size, fixed_bias_value,
                                                       adjust_div_value, norm_half_add_value);
            else
                GetMultiOutputChannelResult3x3_2_32Bit(fixed_weight, expand_input_data, input_channel, expand_width, expand_height, i, output_height,
                                                       temp_output_data_0, temp_output_data_1, pad_size, fixed_bias_value,
                                                       adjust_div_value, norm_half_add_value);
            FixedType *p_ori_outbuf = net_output_data + i*output_height*output_width;
            for (j = 0; j < output_height; ++j)
                memcpy(p_ori_outbuf + j*output_width, temp_output_data_0 + j*expand_width, sizeof(FixedType)*output_width);
           p_ori_outbuf = net_output_data + (i + 1)*output_height*output_width;
            for (j = 0; j < output_height; ++j)
                memcpy(p_ori_outbuf + j*output_width, temp_output_data_1 + j*expand_width, sizeof(FixedType)*output_width);
        }

        for (; i < output_channel; ++i)
       {
            weight_ptr = fixed_weight + i*loop_size;
            if(bool_16Bit_32Bit_Add)
                GetOneOutputChannelResult3x3_16Bit(weight_ptr, expand_input_data, input_channel, expand_width, expand_height, output_height,
                                                  temp_output_data_0, pad_size, fixed_bias_value[i],
                                                   adjust_div_value[i], norm_half_add_value);
            else
                GetOneOutputChannelResult3x3_32Bit(weight_ptr, expand_input_data, input_channel, expand_width, expand_height, output_height,
                                                  temp_output_data_0, pad_size, fixed_bias_value[i],
                                                   adjust_div_value[i], norm_half_add_value);
            FixedType *p_ori_outbuf = net_output_data + i*output_height*output_width;
            for (j = 0; j < output_height; ++j)
                memcpy(p_ori_outbuf + j*output_width, temp_output_data_0 + j*expand_width, sizeof(FixedType)*output_width);
        }
    }
    else if (w_stride_ == 1 && h_stride_ == 1)
    {
        int8x8_t *weight_ptr_neon = NULL;
        int8x8_t in_value_neon;
        int32x4_t out_value_neon_0, out_value_neon_1;
        int16x4_t out_value_neon_16Bit_0, out_value_neon_16Bit_1;
        int16x8_t mul_value, out_value_neon_16Bit, fixed_bias_value_neon;
        int32x4_t norm_half_add_value_neon;
        int search_img_start = 0;
        int search_img_end = output_height*expand_width - 2 * pad_size;
        int search_8_end = (search_img_end - search_img_start) / 8 * 8 + search_img_start;
        int loop_size = input_channel*kernel_size_sq;
        int loop_size_2 = loop_size / 2 * 2;

        weight_ptr_neon = (int8x8_t *)malloc(sizeof(int8x8_t)*loop_size);
        if (weight_ptr_neon == NULL)
            return false;

        norm_half_add_value_neon = vdupq_n_s32(norm_half_add_value);

        for (i = 0; i < output_channel; i++)
        {
            weight_ptr = fixed_weight + i*loop_size;
            for (j = 0; j < loop_size; ++j)
                weight_ptr_neon[j] = vdup_n_s8(weight_ptr[j]);

            fixed_bias_value_neon = vdupq_n_s16(fixed_bias_value[i]);

            for (j = search_img_start; j < search_8_end; j += 8)
            {
                out_value_neon_0 = vdupq_n_s32(0);
                out_value_neon_1 = vdupq_n_s32(0);
                for (m = 0; m < loop_size_2; m += 2)
                {
                    in_value_neon = vld1_s8(input_ptr[m] + j);
                    mul_value = vmull_s8(in_value_neon, weight_ptr_neon[m]);
                    in_value_neon = vld1_s8(input_ptr[m + 1] + j);
                    mul_value = vmlal_s8(mul_value, in_value_neon, weight_ptr_neon[m + 1]);
                    out_value_neon_0 = vaddw_s16(out_value_neon_0, vget_low_s16(mul_value));
                    out_value_neon_1 = vaddw_s16(out_value_neon_1, vget_high_s16(mul_value));
                }
                for (; m < loop_size; ++m)
                {
                    in_value_neon = vld1_s8(input_ptr[m] + j);
                    mul_value = vmull_s8(in_value_neon, weight_ptr_neon[m]);
                    out_value_neon_0 = vaddw_s16(out_value_neon_0, vget_low_s16(mul_value));
                    out_value_neon_1 = vaddw_s16(out_value_neon_1, vget_high_s16(mul_value));
                }
                out_value_neon_0 = vmulq_n_s32(out_value_neon_0, adjust_div_value[i]);
                out_value_neon_1 = vmulq_n_s32(out_value_neon_1, adjust_div_value[i]);
                out_value_neon_0 = vaddq_s32(out_value_neon_0, norm_half_add_value_neon);
                out_value_neon_1 = vaddq_s32(out_value_neon_1, norm_half_add_value_neon);
                out_value_neon_16Bit_0 = vshrn_n_s32(out_value_neon_0, data_norm_move_const);
                out_value_neon_16Bit_1 = vshrn_n_s32(out_value_neon_1, data_norm_move_const);
                out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
                out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon);

                vst1q_s16(temp_output_data_0 + j, out_value_neon_16Bit);
            }
            if (j < search_img_end&&search_img_end - search_img_start >= 8)
            {
                j = search_img_end - 8;
                out_value_neon_0 = vdupq_n_s32(0);
                out_value_neon_1 = vdupq_n_s32(0);
                for (m = 0; m < loop_size; ++m)
                {
                    in_value_neon = vld1_s8(input_ptr[m] + j);
                    mul_value = vmull_s8(in_value_neon, weight_ptr_neon[m]);
                    out_value_neon_0 = vaddw_s16(out_value_neon_0, vget_low_s16(mul_value));
                    out_value_neon_1 = vaddw_s16(out_value_neon_1, vget_high_s16(mul_value));
                }

                out_value_neon_0 = vmulq_n_s32(out_value_neon_0, adjust_div_value[i]);
                out_value_neon_1 = vmulq_n_s32(out_value_neon_1, adjust_div_value[i]);
                out_value_neon_0 = vaddq_s32(out_value_neon_0, norm_half_add_value_neon);
                out_value_neon_1 = vaddq_s32(out_value_neon_1, norm_half_add_value_neon);
                out_value_neon_16Bit_0 = vshrn_n_s32(out_value_neon_0, data_norm_move_const);
                out_value_neon_16Bit_1 = vshrn_n_s32(out_value_neon_1, data_norm_move_const);
                out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
                out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon);

                vst1q_s16(temp_output_data_0 + j, out_value_neon_16Bit);
            }
            else
            {
                for (; j < search_img_end; ++j)
                {
                    int value = 0;
                    for (m = 0; m < loop_size; ++m)
                        value += input_ptr[m][j] * weight_ptr[m];
                    temp_output_data_0[j] = ((value * adjust_div_value[i] + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[i];
                }
            }
            FixedType *p_ori_outbuf = net_output_data + i*output_height*output_width;
            for (j = 0; j < output_height; ++j)
                memcpy(p_ori_outbuf + j*output_width, temp_output_data_0 + j*expand_width, sizeof(FixedType)*output_width);

        }
        EAGLEEYE_SAFEFREE(weight_ptr_neon);
    }
    else if(w_stride_ == 2&&kernel_size == 3)
    {
        int output_channel_2 = output_channel / 2 * 2;
        i = 0;
        for (i = 0; i < output_channel_2; i += 2)
        {
            if(bool_16Bit_32Bit_Add)
                GetMultiOutputChannelResult3x3_Stride2_2_16Bit(fixed_weight, expand_input_data, input_channel, expand_width, expand_height, i, output_width, output_height,
                                                                   net_output_data, pad_size, fixed_bias_value, adjust_div_value, norm_half_add_value);
            else
                GetMultiOutputChannelResult3x3_Stride2_2_32Bit(fixed_weight, expand_input_data, input_channel, expand_width, expand_height, i, output_width, output_height,
                                                               net_output_data, pad_size, fixed_bias_value, adjust_div_value, norm_half_add_value);
        }
        for (; i < output_channel; ++i)
        {
            weight_ptr = fixed_weight + i*input_channel*9;
            if(bool_16Bit_32Bit_Add)
                GetOneOutputChannelResult3x3_Stride2_16Bit(weight_ptr, expand_input_data, input_channel, expand_width, expand_height, output_width, output_height,
                                                               net_output_data + i*output_height*output_width, pad_size, fixed_bias_value[i], adjust_div_value[i], norm_half_add_value);
            else
                GetOneOutputChannelResult3x3_Stride2_32Bit(weight_ptr, expand_input_data, input_channel, expand_width, expand_height, output_width, output_height,
                                                           net_output_data + i*output_height*output_width, pad_size, fixed_bias_value[i], adjust_div_value[i], norm_half_add_value);
        }
    }
    else if(w_stride_ == 2)
    {
        int loop_size = kernel_size_sq*input_channel;
        for (i = 0; i < output_channel; ++i)
        {
            FixedType *p_ori_outbuf = net_output_data + i*output_height*output_width;
            weight_ptr = fixed_weight + i*loop_size;
            if(bool_16Bit_32Bit_Add)
                GetOneOutputChannelResult_Stride2_16Bit(weight_ptr, input_ptr, input_channel, kernel_size_sq, w_stride_, h_stride_, expand_width, expand_height, output_width, output_height,
                                                        p_ori_outbuf, pad_size, fixed_bias_value[i], adjust_div_value[i], norm_half_add_value);
            else
                GetOneOutputChannelResult_Stride2_32Bit(weight_ptr, input_ptr, input_channel, kernel_size_sq, w_stride_, h_stride_, expand_width, expand_height, output_width, output_height,
                                                        p_ori_outbuf, pad_size, fixed_bias_value[i], adjust_div_value[i], norm_half_add_value);
        }
    }
    else
    {
        for (i = 0; i < output_channel; ++i)
        {
            weight_ptr = fixed_weight + i*input_channel*kernel_size_sq;

            FixedType *p_out_buf = net_output_data + i*output_height*output_width;
            for (j = 0; j < output_height; ++j)
            {
                n = j*h_stride_*expand_width;
                for (k = 0; k < output_width; ++k)
                {
                    FixedCalType value = 0;
                    for (m = 0; m < input_channel*kernel_size_sq; ++m)
                        value += input_ptr[m][n] * weight_ptr[m];
                    n += w_stride_;
                    *p_out_buf = ((value * adjust_div_value[i] + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[i];
                    p_out_buf++;
                }
            }
        }
    }
#else
    #ifdef CNN_RECOGNITION_SSE_PROCESSING
	__m128i *weight_sse_0 = NULL;
	__m128i *weight_sse_1 = NULL;
	int loop_size = kernel_size_sq*input_channel;

	weight_sse_0 = (__m128i *)_mm_malloc(sizeof(__m128i)*loop_size, 16);
	weight_sse_1 = (__m128i *)_mm_malloc(sizeof(__m128i)*loop_size, 16);
	if (weight_sse_0 == NULL || weight_sse_1 == NULL)
	{
		EAGLEEYE_SAFEFREE(temp_output_data_0);
		EAGLEEYE_SAFEFREE(temp_output_data_1);
		EAGLEEYE_SAFEFREE(input_ptr);
		_mm_free(weight_sse_0);
		_mm_free(weight_sse_1);
		return false;
	}

	if (w_stride_ == 1 && h_stride_ == 1 && kernel_size == 3)
	{
		int output_channel_2 = output_channel / 2 * 2;

		for (i = 0; i < output_channel_2; i += 2)
		{
			ProcessTwoLine3x3(expand_input_data, input_ptr, fixed_weight, i, loop_size, input_channel, expand_width, expand_height, output_width, output_height, pad_size, net_output_data,
				temp_output_data_0, temp_output_data_1, weight_sse_0, weight_sse_1, fixed_bias_value[i], fixed_bias_value[i + 1], adjust_div_value[i], adjust_div_value[i + 1], norm_half_add_value, data_norm_move_const);
		}

		if (i < output_channel&&output_channel >= 2)
		{
			i = output_channel - 2;
			ProcessTwoLine3x3(expand_input_data, input_ptr, fixed_weight, i, loop_size, input_channel, expand_width, expand_height, output_width, output_height, pad_size, net_output_data,
				temp_output_data_0, temp_output_data_1, weight_sse_0, weight_sse_1, fixed_bias_value[i], fixed_bias_value[i + 1], adjust_div_value[i], adjust_div_value[i + 1], norm_half_add_value, data_norm_move_const);
		}

		for (; i < output_channel; ++i)
		{
			signed char *weight_ptr = fixed_weight + i*input_channel*kernel_size_sq;
			int search_img_end = output_height*expand_width - 2 * pad_size;
			for (j = 0; j < search_img_end; ++j)
			{
				int value = 0;
				for (m = 0; m < loop_size; ++m)
					value += input_ptr[m][j] * weight_ptr[m];
				temp_output_data_0[j] = ((value * adjust_div_value[i] + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[i];
				n++;
			}
			short *p_ori_outbuf = net_output_data + i*output_height*output_width;

			for (j = 0; j < output_height; ++j)
				memcpy(p_ori_outbuf + j*output_width, temp_output_data_0 + j*expand_width, sizeof(short)*output_width);
		}
	}
	else if (w_stride_ == 1 && h_stride_ == 1)
	{
		int output_channel_2 = output_channel / 2 * 2;
		for (i = 0; i < output_channel_2; i += 2)
		{
			ProcessTwoLine(input_ptr, fixed_weight, i, loop_size, expand_width, output_width, output_height, pad_size, net_output_data, temp_output_data_0, temp_output_data_1,
				weight_sse_0, weight_sse_1, fixed_bias_value[i], fixed_bias_value[i + 1], adjust_div_value[i], adjust_div_value[i + 1], norm_half_add_value, data_norm_move_const);
		}

		if (i < output_channel&&output_channel >= 2)
		{
			i = output_channel - 2;
			ProcessTwoLine(input_ptr, fixed_weight, i, loop_size, expand_width, output_width, output_height, pad_size, net_output_data, temp_output_data_0, temp_output_data_1,
				weight_sse_0, weight_sse_1, fixed_bias_value[i], fixed_bias_value[i + 1], adjust_div_value[i], adjust_div_value[i + 1], norm_half_add_value, data_norm_move_const);
		}

		for (; i < output_channel; ++i)
		{
			signed char *weight_ptr = fixed_weight + i*input_channel*kernel_size_sq;
			int search_img_end = output_height*expand_width - 2 * pad_size;
			for (j = 0; j < search_img_end; ++j)
			{
				int value = 0;
				for (m = 0; m < loop_size; ++m)
					value += input_ptr[m][j] * weight_ptr[m];
				temp_output_data_0[j] = ((value * adjust_div_value[i] + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[i];
				n++;
			}
			short *p_ori_outbuf = net_output_data + i*output_height*output_width;

			for (j = 0; j < output_height; ++j)
				memcpy(p_ori_outbuf + j*output_width, temp_output_data_0 + j*expand_width, sizeof(short)*output_width);
		}
	}
	else
	{
		for (i = 0; i < output_channel; ++i)
		{
			signed char *weight_ptr = fixed_weight + i*input_channel*kernel_size_sq;

			short *p_out_buf = net_output_data + i*output_height*output_width;
			for (j = 0; j < output_height; ++j)
			{
				n = j*h_stride_*expand_width;
				for (k = 0; k < output_width; ++k)
				{
					int value = 0;
					for (m = 0; m < input_channel*kernel_size_sq; ++m)
						value += input_ptr[m][n] * weight_ptr[m];
					n += w_stride_;
					*p_out_buf = ((value * adjust_div_value[i] + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[i];
					p_out_buf++;
				}
			}
		}
	}
	_mm_free(weight_sse_0);
	_mm_free(weight_sse_1);

#else
    if (w_stride_ == 1 && h_stride_ == 1)
	{
		FixedType *temp_output_data = (FixedType *)malloc(sizeof(FixedType)*expand_width*output_height);
		if (temp_output_data == NULL)
			return false;
		for (i = 0; i < output_channel; ++i)
		{
			FixedConvType *weight_ptr = fixed_weight + i*input_channel*kernel_size_sq;

			FixedType *p_out_buf = temp_output_data;
			FixedType *p_ori_out_buf = net_output_data + i*output_height*output_width;

			for (j = 0; j < output_height*expand_width - 2 * pad_size; ++j)
			{
			    if(bool_16Bit_32Bit_Add)
				{
				    short value = 0;
					for (m = 0; m < input_channel*kernel_size_sq; ++m)
					{
						value += input_ptr[m][j] * weight_ptr[m];
					}
				    *p_out_buf = ((value * adjust_div_value[i] + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[i];
				}
				else
				{
				    int value = 0;
				    for (m = 0; m < input_channel*kernel_size_sq; ++m)
					    value += input_ptr[m][j] * weight_ptr[m];
		
				    *p_out_buf = ((value * adjust_div_value[i] + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[i];
				}
				p_out_buf++;
			}
			for (j = 0; j < output_height; ++j)
				memcpy(p_ori_out_buf + j*output_width, temp_output_data + j*expand_width, sizeof(FixedType)*output_width);

		}
		EAGLEEYE_SAFEFREE(temp_output_data);
	}
	else
	{
		for (i = 0; i < output_channel; ++i)
		{
			FixedConvType *weight_ptr = fixed_weight + i*input_channel*kernel_size_sq;

			FixedType *p_out_buf = net_output_data + i*output_height*output_width;
			for (j = 0; j < output_height; ++j)
			{
				n = j*h_stride_*expand_width;
				for (k = 0; k < output_width; ++k)
				{
                    if(bool_16Bit_32Bit_Add)
                    {
                        short value = 0;
						for (m = 0; m < input_channel*kernel_size_sq; ++m)
						{
							value += input_ptr[m][n] * weight_ptr[m];
						}
						n += w_stride_;
                        *p_out_buf = ((value * adjust_div_value[i] + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[i];
                    } else
                    {
                        int value = 0;
                        for (m = 0; m < input_channel*kernel_size_sq; ++m)
                            value += input_ptr[m][n] * weight_ptr[m];
                        n += w_stride_;
                        *p_out_buf = ((value * adjust_div_value[i] + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[i];
                    }
					p_out_buf++;
				}
			}
		}
	}
#endif
#endif
    EAGLEEYE_SAFEFREE(temp_output_data_0);
    EAGLEEYE_SAFEFREE(temp_output_data_1);
    EAGLEEYE_SAFEFREE(input_ptr);
    return true;
}

bool RunConvForward_ImageLoop_Temp(FixedConvType *expand_input_data, int input_channel, int expand_width, int expand_height, FixedConvType *fixed_weight,
	FixedType *net_output_data, int output_channel, int output_width, int output_height, int kernel_size, int pad_size, int w_stride_, int h_stride_,
	FixedType *fixed_bias_value, float multi_value)
{
	int i = 0, j = 0, k = 0, m = 0, l = 0, n = 0;
	int kernel_size_sq = kernel_size*kernel_size;
	FixedType *temp_output_data_0 = NULL;
	FixedType *temp_output_data_1 = NULL;
	FixedConvType **input_ptr = NULL;
	signed char *weight_ptr = NULL;
	const int  data_norm_move_const = CNN_CONV_DATA_NORM_MOVE;

	temp_output_data_0 = (FixedType *)malloc(sizeof(FixedType)*expand_width*output_height);
	temp_output_data_1 = (FixedType *)malloc(sizeof(FixedType)*expand_width*output_height);
	input_ptr = (FixedConvType **)malloc(sizeof(FixedConvType *)*kernel_size_sq*input_channel);
	if (temp_output_data_0 == NULL || temp_output_data_1 == NULL || input_ptr == NULL)
	{
		EAGLEEYE_SAFEFREE(temp_output_data_0);
		EAGLEEYE_SAFEFREE(temp_output_data_1);
		EAGLEEYE_SAFEFREE(input_ptr);
		return false;
	}
	n = 0;
	for (j = 0; j < input_channel; ++j)
	{
		for (m = 0; m < kernel_size; ++m)
		for (l = 0; l < kernel_size; ++l)
		{
			input_ptr[n] = expand_input_data + j*expand_height*expand_width + m*expand_width + l;
			n++;
		}
	}
	for (i = 0; i < output_channel; ++i)
	{
		FixedConvType *weight_ptr = fixed_weight + i*input_channel*kernel_size_sq;

		FixedType *p_out_buf = net_output_data + i*output_height*output_width;
		for (j = 0; j < output_height; ++j)
		{
			n = j*h_stride_*expand_width;
			for (k = 0; k < output_width; ++k)
			{								
				FixedCalType value = 0;
				for (m = 0; m < input_channel*kernel_size_sq; ++m)
					value += input_ptr[m][n] * weight_ptr[m];
				n += w_stride_;
				*p_out_buf = int(value * multi_value + fixed_bias_value[i]);			
				p_out_buf++;
			}
		}
	}
	

	EAGLEEYE_SAFEFREE(temp_output_data_0);
	EAGLEEYE_SAFEFREE(temp_output_data_1);
	EAGLEEYE_SAFEFREE(input_ptr);
	return true;
}



bool RunConvForward_KernelLoop_Temp(FixedConvType *expand_input_data, int input_channel, int expand_width, int expand_height, FixedConvType *fixed_weight,
	FixedType *net_output_data, int output_channel, int output_width, int output_height, int kernel_size, int pad_size, int w_stride_, int h_stride_,
	FixedBiasType *fixed_bias_value, float *multi_value)
{
	int i = 0, j = 0, m = 0, l = 0, k = 0, n = 0;
	bool bRet = false;
	FixedConvType *data_col = NULL;
	int row1 = output_channel;
	int col1 = kernel_size*kernel_size*input_channel;
	int row2 = col1;
	int col2 = output_width*output_height;
	FixedConvType **input_ptr = NULL;
	const int  data_norm_move_const = CNN_CONV_DATA_NORM_MOVE;

	input_ptr = (FixedConvType **)malloc(sizeof(FixedConvType *)*kernel_size*kernel_size*input_channel);
	data_col = (FixedConvType *)malloc(sizeof(FixedConvType)*row2*col2);
	if (data_col == NULL || input_ptr == NULL)
	{
		EAGLEEYE_SAFEFREE(data_col);
		EAGLEEYE_SAFEFREE(input_ptr);
		return false;
	}

	n = 0;
	for (j = 0; j < input_channel; ++j)
	{
		for (m = 0; m < kernel_size; ++m)
		for (l = 0; l < kernel_size; ++l)
		{
			input_ptr[n] = expand_input_data + j*expand_width*expand_height + m*expand_width + l;
			n++;
		}
	}

	bRet = im2col_cpu_Fixed(input_ptr, input_channel, expand_width, expand_height, output_width, output_height, kernel_size, kernel_size,
		pad_size, pad_size, w_stride_, h_stride_, 1, data_col);


	for (i = 0; i < row1; ++i)
	for (j = 0; j < col2; ++j)
	{
		float sum = 0;
		for (k = 0; k < col1; ++k)
		{
			sum += fixed_weight[i*col1 + k] * data_col[j*row2 + k];
		}
		float xx = sum * multi_value[i] + fixed_bias_value[i];
		if (xx > 32767)
		{
			printf("----------------------------------------------------------------------%f\n", xx);
			xx = 32767;
		}
		if (xx < -32768)
		{
			printf("--------------------------------------------------------------------%f\n", xx);
			xx = -32768;
		}
		if (xx < 0)
			net_output_data[i*col2 + j] = int(xx - 0.5);
		else
			net_output_data[i*col2 + j] = int(xx + 0.5);
	}

	EAGLEEYE_SAFEFREE(data_col);
	EAGLEEYE_SAFEFREE(input_ptr);
	return true;
}    
} // namespace eagleeye