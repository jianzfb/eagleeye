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
                                      FixedBiasType *fixed_bias_value, int *adjust_div_value, int norm_half_add_value)
{
    int j = 0, k = 0, m = 0;
    int search_img_start = 0;
    int search_img_end = output_height*expand_width - 2 * pad_size;
    int expand_size = expand_height*expand_width;
    int search_8_end = (search_img_end - search_img_start) / 16 * 16 + search_img_start;
    int new_kernel_size = input_channel * 2 * 9;
    const int data_norm_move_const = CNN_CONV_DATA_NORM_MOVE;
    int adjust_div_value_0 = adjust_div_value[cur_channel];
    int adjust_div_value_1 = adjust_div_value[cur_channel + 1];

    FixedConvType *new_fixed_weight = (FixedConvType *)malloc(sizeof(FixedConvType)*new_kernel_size);
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

    j = search_img_start;

#ifdef CNN_ARM_NEON_PROCESSING
    int8x8_t *weight_ptr_neon;
	int8x16_t in_value_neon_q;
	int8x8_t in_value_neon_0, in_value_neon_1;
	int16x8_t out_value_neon0_0, out_value_neon0_1, out_value_neon1_0, out_value_neon1_1;
    int16x8_t fixed_bias_value_neon_0, fixed_bias_value_neon_1;
    int32x4_t norm_half_add_value_neon, mul_value_0, mul_value_1;
    int16x4_t out_value_neon_16Bit_0, out_value_neon_16Bit_1;
    int16x8_t out_value_neon_16Bit;

    norm_half_add_value_neon = vdupq_n_s32(norm_half_add_value);
    fixed_bias_value_neon_0 = vdupq_n_s16(fixed_bias_value[cur_channel]);
    fixed_bias_value_neon_1 = vdupq_n_s16(fixed_bias_value[cur_channel + 1]);

	weight_ptr_neon = (int8x8_t *)malloc(sizeof(int8x8_t)*new_kernel_size);
	if (weight_ptr_neon == NULL)
		return;
	for (k = 0; k < new_kernel_size; ++k)
		weight_ptr_neon[k] = vdup_n_s8(new_fixed_weight[k]);

    for (j = 0; j < search_8_end; j += 16)
	{
        FixedConvType *ptr_0 = expand_input_data + j;
        FixedConvType *ptr_1 = expand_input_data + j + expand_width;
        FixedConvType *ptr_2 = expand_input_data + j + 2 * expand_width;

		out_value_neon0_0 = vdupq_n_s16(0);
		out_value_neon0_1 = vdupq_n_s16(0);
		out_value_neon1_0 = vdupq_n_s16(0);
		out_value_neon1_1 = vdupq_n_s16(0);
		int8x8_t *weigth_neon_ptr_0 = weight_ptr_neon;
		for (m = 0; m < input_channel; ++m)
		{
            FixedConvType *ptr = ptr_0;
			in_value_neon_q = vld1q_s8(ptr);
			in_value_neon_0 = vget_low_s8(in_value_neon_q);
			in_value_neon_1 = vget_high_s8(in_value_neon_q);
            out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_0, *weigth_neon_ptr_0);
            out_value_neon0_1 = vmlal_s8(out_value_neon0_1, in_value_neon_1, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
			out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon_0, *weigth_neon_ptr_0);
			out_value_neon1_1 = vmlal_s8(out_value_neon1_1, in_value_neon_1, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
			ptr++;
			in_value_neon_q = vld1q_s8(ptr);
			in_value_neon_0 = vget_low_s8(in_value_neon_q);
			in_value_neon_1 = vget_high_s8(in_value_neon_q);
            out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_0, *weigth_neon_ptr_0);
            out_value_neon0_1 = vmlal_s8(out_value_neon0_1, in_value_neon_1, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon_0, *weigth_neon_ptr_0);
            out_value_neon1_1 = vmlal_s8(out_value_neon1_1, in_value_neon_1, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
			ptr++;
			in_value_neon_q = vld1q_s8(ptr);
			in_value_neon_0 = vget_low_s8(in_value_neon_q);
			in_value_neon_1 = vget_high_s8(in_value_neon_q);
            out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_0, *weigth_neon_ptr_0);
            out_value_neon0_1 = vmlal_s8(out_value_neon0_1, in_value_neon_1, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon_0, *weigth_neon_ptr_0);
            out_value_neon1_1 = vmlal_s8(out_value_neon1_1, in_value_neon_1, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;

			ptr = ptr_1;
			in_value_neon_q = vld1q_s8(ptr);
			in_value_neon_0 = vget_low_s8(in_value_neon_q);
			in_value_neon_1 = vget_high_s8(in_value_neon_q);
            out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_0, *weigth_neon_ptr_0);
            out_value_neon0_1 = vmlal_s8(out_value_neon0_1, in_value_neon_1, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon_0, *weigth_neon_ptr_0);
            out_value_neon1_1 = vmlal_s8(out_value_neon1_1, in_value_neon_1, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
			ptr++;
			in_value_neon_q = vld1q_s8(ptr);
			in_value_neon_0 = vget_low_s8(in_value_neon_q);
			in_value_neon_1 = vget_high_s8(in_value_neon_q);
            out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_0, *weigth_neon_ptr_0);
            out_value_neon0_1 = vmlal_s8(out_value_neon0_1, in_value_neon_1, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon_0, *weigth_neon_ptr_0);
            out_value_neon1_1 = vmlal_s8(out_value_neon1_1, in_value_neon_1, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
			ptr++;
			in_value_neon_q = vld1q_s8(ptr);
			in_value_neon_0 = vget_low_s8(in_value_neon_q);
			in_value_neon_1 = vget_high_s8(in_value_neon_q);
            out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_0, *weigth_neon_ptr_0);
            out_value_neon0_1 = vmlal_s8(out_value_neon0_1, in_value_neon_1, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon_0, *weigth_neon_ptr_0);
            out_value_neon1_1 = vmlal_s8(out_value_neon1_1, in_value_neon_1, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;

			ptr = ptr_2;
			in_value_neon_q = vld1q_s8(ptr);
			in_value_neon_0 = vget_low_s8(in_value_neon_q);
			in_value_neon_1 = vget_high_s8(in_value_neon_q);
            out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_0, *weigth_neon_ptr_0);
            out_value_neon0_1 = vmlal_s8(out_value_neon0_1, in_value_neon_1, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon_0, *weigth_neon_ptr_0);
            out_value_neon1_1 = vmlal_s8(out_value_neon1_1, in_value_neon_1, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
			ptr++;
			in_value_neon_q = vld1q_s8(ptr);
			in_value_neon_0 = vget_low_s8(in_value_neon_q);
			in_value_neon_1 = vget_high_s8(in_value_neon_q);
            out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_0, *weigth_neon_ptr_0);
            out_value_neon0_1 = vmlal_s8(out_value_neon0_1, in_value_neon_1, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon_0, *weigth_neon_ptr_0);
            out_value_neon1_1 = vmlal_s8(out_value_neon1_1, in_value_neon_1, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
			ptr++;
			in_value_neon_q = vld1q_s8(ptr);
			in_value_neon_0 = vget_low_s8(in_value_neon_q);
			in_value_neon_1 = vget_high_s8(in_value_neon_q);
            out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_0, *weigth_neon_ptr_0);
            out_value_neon0_1 = vmlal_s8(out_value_neon0_1, in_value_neon_1, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon_0, *weigth_neon_ptr_0);
            out_value_neon1_1 = vmlal_s8(out_value_neon1_1, in_value_neon_1, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;

			ptr_0 += expand_size;
			ptr_1 += expand_size;
			ptr_2 += expand_size;
		}

        mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon0_0));
        mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon0_0));
        mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value_0);
        mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value_0);
        mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
        mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_0);
        vst1q_s16(temp_output_data_0 + j, out_value_neon_16Bit);

        mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon0_1));
        mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon0_1));
        mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value_0);
        mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value_0);
        mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
        mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_0);
        vst1q_s16(temp_output_data_0 + j + 8, out_value_neon_16Bit);

        mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon1_0));
        mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon1_0));
        mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value_1);
        mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value_1);
        mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
        mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_1);
        vst1q_s16(temp_output_data_1 + j, out_value_neon_16Bit);

        mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon1_1));
        mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon1_1));
        mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value_1);
        mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value_1);
        mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
        mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_1);
        vst1q_s16(temp_output_data_1 + j + 8, out_value_neon_16Bit);
	}

	if (j < search_img_end&&search_img_end - search_img_start >= 8)
	{
        int start_n[2] = { 0 };
		int end_loop_n = 0;
		start_n[0] = search_img_end - 8;
		if (j + 8 >= search_img_end)
			end_loop_n = 1;
		else
		{
			end_loop_n = 2;
			start_n[1] = j;
		}

        for (k = 0; k < end_loop_n; ++k)
		{
			int n = start_n[k];
			int8x8_t *weigth_neon_ptr_0 = weight_ptr_neon;
			int8x8_t in_value_neon;
            FixedConvType *ptr_0 = expand_input_data + n;
            FixedConvType *ptr_1 = expand_input_data + n + expand_width;
            FixedConvType *ptr_2 = expand_input_data + n + 2 * expand_width;

            out_value_neon0_0 = vdupq_n_s16(0);
            out_value_neon1_0 = vdupq_n_s16(0);
			for (m = 0; m < input_channel; ++m) {
                FixedConvType *ptr = ptr_0;
				in_value_neon = vld1_s8(ptr);
				out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
                out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
				ptr++;
				in_value_neon = vld1_s8(ptr);
                out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
                out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
				ptr++;
				in_value_neon = vld1_s8(ptr);
                out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
                out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;

				ptr = ptr_1;
				in_value_neon = vld1_s8(ptr);
                out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
                out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
				ptr++;
				in_value_neon = vld1_s8(ptr);
                out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
                out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
				ptr++;
				in_value_neon = vld1_s8(ptr);
                out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
                out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;

				ptr = ptr_2;
				in_value_neon = vld1_s8(ptr);
                out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
                out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
				ptr++;
				in_value_neon = vld1_s8(ptr);
                out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
                out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
				ptr++;
				in_value_neon = vld1_s8(ptr);
                out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
                out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;

				ptr_0 += expand_size;
				ptr_1 += expand_size;
				ptr_2 += expand_size;
			}
            mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon0_0));
            mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon0_0));
            mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value_0);
            mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value_0);
            mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
            mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
            out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
            out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
            out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
            out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_0);
            vst1q_s16(temp_output_data_0 + n, out_value_neon_16Bit);

            mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon1_0));
            mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon1_0));
            mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value_1);
            mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value_1);
            mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
            mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
            out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
            out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
            out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
            out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_1);
            vst1q_s16(temp_output_data_1 + n, out_value_neon_16Bit);
		}
	}
	else
#endif
    {
        for (; j < search_img_end; ++j)
        {
            short value_0 = 0;
            short value_1 = 0;
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

            temp_output_data_0[j] = ((value_0 * adjust_div_value_0 + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[cur_channel];
            temp_output_data_1[j] = ((value_1 * adjust_div_value_1 + norm_half_add_value) >> data_norm_move_const) + fixed_bias_value[cur_channel + 1];
        }
    }
    EAGLEEYE_SAFEFREE(new_fixed_weight);
#ifdef CNN_ARM_NEON_PROCESSING
    EAGLEEYE_SAFEFREE(weight_ptr_neon);
#endif
}


void GetOneOutputChannelResult3x3_16Bit(FixedConvType *weight_ptr, FixedConvType *expand_input_data, int input_channel, int expand_width,
                                  int expand_height, int output_height, FixedType *temp_output_data, int pad_size,
                                  FixedBiasType bias_value, int adjust_div_value, int norm_half_add_value)
{
    int j = 0, k = 0, m = 0;
    int search_img_start = 0;
    int search_img_end = output_height*expand_width - 2 * pad_size;
    int expand_size = expand_height*expand_width;
    int search_8_end = (search_img_end - search_img_start) / 16 * 16 + search_img_start;
    const int data_norm_move_const = CNN_CONV_DATA_NORM_MOVE;
    int new_kernel_size = input_channel*9;

    j = search_img_start;

#ifdef CNN_ARM_NEON_PROCESSING
    int8x8_t *weight_ptr_neon;
    int8x16_t in_value_neon_q;
    int8x8_t in_value_neon_0, in_value_neon_1;
    int16x8_t out_value_neon0_0, out_value_neon0_1;
    int16x8_t fixed_bias_value_neon;
    int32x4_t norm_half_add_value_neon, mul_value_0, mul_value_1;
    int16x4_t out_value_neon_16Bit_0, out_value_neon_16Bit_1;
    int16x8_t out_value_neon_16Bit;

    norm_half_add_value_neon = vdupq_n_s32(norm_half_add_value);
    fixed_bias_value_neon = vdupq_n_s16(bias_value);

    weight_ptr_neon = (int8x8_t *)malloc(sizeof(int8x8_t)*new_kernel_size);
    if (weight_ptr_neon == NULL)
        return;
    for (k = 0; k < new_kernel_size; ++k)
        weight_ptr_neon[k] = vdup_n_s8(weight_ptr[k]);

    for (j = 0; j < search_8_end; j += 16)
    {
        FixedConvType *ptr_0 = expand_input_data + j;
        FixedConvType *ptr_1 = expand_input_data + j + expand_width;
        FixedConvType *ptr_2 = expand_input_data + j + 2 * expand_width;

        out_value_neon0_0 = vdupq_n_s16(0);
        out_value_neon0_1 = vdupq_n_s16(0);
        int8x8_t *weigth_neon_ptr_0 = weight_ptr_neon;
        for (m = 0; m < input_channel; ++m)
        {
            FixedConvType *ptr = ptr_0;
            in_value_neon_q = vld1q_s8(ptr);
            in_value_neon_0 = vget_low_s8(in_value_neon_q);
            in_value_neon_1 = vget_high_s8(in_value_neon_q);
            out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_0, *weigth_neon_ptr_0);
            out_value_neon0_1 = vmlal_s8(out_value_neon0_1, in_value_neon_1, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            ptr++;
            in_value_neon_q = vld1q_s8(ptr);
            in_value_neon_0 = vget_low_s8(in_value_neon_q);
            in_value_neon_1 = vget_high_s8(in_value_neon_q);
            out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_0, *weigth_neon_ptr_0);
            out_value_neon0_1 = vmlal_s8(out_value_neon0_1, in_value_neon_1, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            ptr++;
            in_value_neon_q = vld1q_s8(ptr);
            in_value_neon_0 = vget_low_s8(in_value_neon_q);
            in_value_neon_1 = vget_high_s8(in_value_neon_q);
            out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_0, *weigth_neon_ptr_0);
            out_value_neon0_1 = vmlal_s8(out_value_neon0_1, in_value_neon_1, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;

            ptr = ptr_1;
            in_value_neon_q = vld1q_s8(ptr);
            in_value_neon_0 = vget_low_s8(in_value_neon_q);
            in_value_neon_1 = vget_high_s8(in_value_neon_q);
            out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_0, *weigth_neon_ptr_0);
            out_value_neon0_1 = vmlal_s8(out_value_neon0_1, in_value_neon_1, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            ptr++;
            in_value_neon_q = vld1q_s8(ptr);
            in_value_neon_0 = vget_low_s8(in_value_neon_q);
            in_value_neon_1 = vget_high_s8(in_value_neon_q);
            out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_0, *weigth_neon_ptr_0);
            out_value_neon0_1 = vmlal_s8(out_value_neon0_1, in_value_neon_1, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            ptr++;
            in_value_neon_q = vld1q_s8(ptr);
            in_value_neon_0 = vget_low_s8(in_value_neon_q);
            in_value_neon_1 = vget_high_s8(in_value_neon_q);
            out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_0, *weigth_neon_ptr_0);
            out_value_neon0_1 = vmlal_s8(out_value_neon0_1, in_value_neon_1, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;

            ptr = ptr_2;
            in_value_neon_q = vld1q_s8(ptr);
            in_value_neon_0 = vget_low_s8(in_value_neon_q);
            in_value_neon_1 = vget_high_s8(in_value_neon_q);
            out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_0, *weigth_neon_ptr_0);
            out_value_neon0_1 = vmlal_s8(out_value_neon0_1, in_value_neon_1, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            ptr++;
            in_value_neon_q = vld1q_s8(ptr);
            in_value_neon_0 = vget_low_s8(in_value_neon_q);
            in_value_neon_1 = vget_high_s8(in_value_neon_q);
            out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_0, *weigth_neon_ptr_0);
            out_value_neon0_1 = vmlal_s8(out_value_neon0_1, in_value_neon_1, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            ptr++;
            in_value_neon_q = vld1q_s8(ptr);
            in_value_neon_0 = vget_low_s8(in_value_neon_q);
            in_value_neon_1 = vget_high_s8(in_value_neon_q);
            out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_0, *weigth_neon_ptr_0);
            out_value_neon0_1 = vmlal_s8(out_value_neon0_1, in_value_neon_1, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;

            ptr_0 += expand_size;
            ptr_1 += expand_size;
            ptr_2 += expand_size;
        }

        mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon0_0));
        mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon0_0));
        mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value);
        mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value);
        mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
        mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon);
        vst1q_s16(temp_output_data + j, out_value_neon_16Bit);

        mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon0_1));
        mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon0_1));
        mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value);
        mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value);
        mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
        mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon);
        vst1q_s16(temp_output_data + j + 8, out_value_neon_16Bit);

    }

    if (j < search_img_end&&search_img_end - search_img_start >= 8)
    {
        int start_n[2] = { 0 };
        int end_loop_n = 0;
        start_n[0] = search_img_end - 8;
        if (j + 8 >= search_img_end)
            end_loop_n = 1;
        else
        {
            end_loop_n = 2;
            start_n[1] = j;
        }

        for (k = 0; k < end_loop_n; ++k)
        {
            int n = start_n[k];
            int8x8_t *weigth_neon_ptr_0 = weight_ptr_neon;
            int8x8_t in_value_neon;
            FixedConvType *ptr_0 = expand_input_data + n;
            FixedConvType *ptr_1 = expand_input_data + n + expand_width;
            FixedConvType *ptr_2 = expand_input_data + n + 2 * expand_width;

            out_value_neon0_0 = vdupq_n_s16(0);
             for (m = 0; m < input_channel; ++m) {
                FixedConvType *ptr = ptr_0;
                in_value_neon = vld1_s8(ptr);
                out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
                ptr++;
                in_value_neon = vld1_s8(ptr);
                out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
                ptr++;
                in_value_neon = vld1_s8(ptr);
                out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;

                ptr = ptr_1;
                in_value_neon = vld1_s8(ptr);
                out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
                 ptr++;
                in_value_neon = vld1_s8(ptr);
                out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
                ptr++;
                in_value_neon = vld1_s8(ptr);
                out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;

                ptr = ptr_2;
                in_value_neon = vld1_s8(ptr);
                out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
                ptr++;
                in_value_neon = vld1_s8(ptr);
                out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
                 ptr++;
                in_value_neon = vld1_s8(ptr);
                out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;

                ptr_0 += expand_size;
                ptr_1 += expand_size;
                ptr_2 += expand_size;
            }
            mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon0_0));
            mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon0_0));
            mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value);
            mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value);
            mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
            mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
            out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
            out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
            out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
            out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon);
            vst1q_s16(temp_output_data + n, out_value_neon_16Bit);
        }
    }
    else
#endif
    {
        for (; j < search_img_end; ++j)
        {
            short value_0 = 0;
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

            temp_output_data[j] = ((value_0 * adjust_div_value + norm_half_add_value) >> data_norm_move_const) + bias_value;
        }
    }

#ifdef CNN_ARM_NEON_PROCESSING
    EAGLEEYE_SAFEFREE(weight_ptr_neon);
#endif
}

void GetMultiOutputChannelResult1x1_4_16Bit(FixedConvType *fixed_weight, FixedConvType *expand_input_data, int input_channel, int expand_width, int expand_height, int cur_channel,
                                      int output_height, FixedType *temp_output_data_0, FixedType *temp_output_data_1, FixedType *temp_output_data_2, FixedType *temp_output_data_3,
                                      int pad_size,  FixedBiasType *fixed_bias_value, int *adjust_div_value, int norm_half_add_value)
{
    int j = 0, k = 0, m = 0;
    int search_img_start = 0;
    int search_img_end = output_height*expand_width - 2 * pad_size;
    int expand_size = expand_height*expand_width;
    int search_8_end = (search_img_end - search_img_start) / 16 * 16 + search_img_start;
    int new_kernel_size = input_channel * 4;
    const int data_norm_move_const = CNN_CONV_DATA_NORM_MOVE;
    int adjust_div_value_0 = adjust_div_value[cur_channel];
    int adjust_div_value_1 = adjust_div_value[cur_channel + 1];
    int adjust_div_value_2 = adjust_div_value[cur_channel + 2];
    int adjust_div_value_3 = adjust_div_value[cur_channel + 3];


    FixedConvType *new_fixed_weight = (FixedConvType *)malloc(sizeof(FixedConvType)*new_kernel_size);
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

    j = search_img_start;

#ifdef CNN_ARM_NEON_PROCESSING
    int8x8_t *weight_ptr_neon;
    int8x16_t in_value_neon_q;
    int8x8_t in_value_neon_0, in_value_neon_1;
    int16x8_t out_value_neon0_0, out_value_neon0_1, out_value_neon1_0, out_value_neon1_1;
    int16x8_t out_value_neon2_0, out_value_neon2_1, out_value_neon3_0, out_value_neon3_1;
    int16x8_t fixed_bias_value_neon_0, fixed_bias_value_neon_1, fixed_bias_value_neon_2, fixed_bias_value_neon_3;
    int32x4_t norm_half_add_value_neon, mul_value_0, mul_value_1;
    int16x4_t out_value_neon_16Bit_0, out_value_neon_16Bit_1;
    int16x8_t out_value_neon_16Bit;

    norm_half_add_value_neon = vdupq_n_s32(norm_half_add_value);
    fixed_bias_value_neon_0 = vdupq_n_s16(fixed_bias_value[cur_channel]);
    fixed_bias_value_neon_1 = vdupq_n_s16(fixed_bias_value[cur_channel + 1]);
    fixed_bias_value_neon_2 = vdupq_n_s16(fixed_bias_value[cur_channel + 2]);
    fixed_bias_value_neon_3 = vdupq_n_s16(fixed_bias_value[cur_channel + 3]);


    weight_ptr_neon = (int8x8_t *)malloc(sizeof(int8x8_t)*new_kernel_size);
    if (weight_ptr_neon == NULL)
        return;
    for (k = 0; k < new_kernel_size; ++k)
        weight_ptr_neon[k] = vdup_n_s8(new_fixed_weight[k]);

    for (j = 0; j < search_8_end; j += 16)
    {
        FixedConvType *ptr_0 = expand_input_data + j;

        out_value_neon0_0 = vdupq_n_s16(0);
        out_value_neon0_1 = vdupq_n_s16(0);
        out_value_neon1_0 = vdupq_n_s16(0);
        out_value_neon1_1 = vdupq_n_s16(0);
        out_value_neon2_0 = vdupq_n_s16(0);
        out_value_neon2_1 = vdupq_n_s16(0);
        out_value_neon3_0 = vdupq_n_s16(0);
        out_value_neon3_1 = vdupq_n_s16(0);

        int8x8_t *weigth_neon_ptr_0 = weight_ptr_neon;
        for (m = 0; m < input_channel; m++)
        {
            in_value_neon_q = vld1q_s8(ptr_0);
            in_value_neon_0 = vget_low_s8(in_value_neon_q);
            in_value_neon_1 = vget_high_s8(in_value_neon_q);
            out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_0, *weigth_neon_ptr_0);
            out_value_neon0_1 = vmlal_s8(out_value_neon0_1, in_value_neon_1, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon_0, *weigth_neon_ptr_0);
            out_value_neon1_1 = vmlal_s8(out_value_neon1_1, in_value_neon_1, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            out_value_neon2_0 = vmlal_s8(out_value_neon2_0, in_value_neon_0, *weigth_neon_ptr_0);
            out_value_neon2_1 = vmlal_s8(out_value_neon2_1, in_value_neon_1, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            out_value_neon3_0 = vmlal_s8(out_value_neon3_0, in_value_neon_0, *weigth_neon_ptr_0);
            out_value_neon3_1 = vmlal_s8(out_value_neon3_1, in_value_neon_1, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
            ptr_0 += expand_size;
        }
        mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon0_0));
        mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon0_0));
        mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value_0);
        mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value_0);
        mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
        mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_0);
        vst1q_s16(temp_output_data_0 + j, out_value_neon_16Bit);

        mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon0_1));
        mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon0_1));
        mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value_0);
        mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value_0);
        mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
        mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_0);
        vst1q_s16(temp_output_data_0 + j + 8, out_value_neon_16Bit);

        mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon1_0));
        mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon1_0));
        mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value_1);
        mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value_1);
        mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
        mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_1);
        vst1q_s16(temp_output_data_1 + j, out_value_neon_16Bit);

        mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon1_1));
        mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon1_1));
        mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value_1);
        mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value_1);
        mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
        mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_1);
        vst1q_s16(temp_output_data_1 + j + 8, out_value_neon_16Bit);

        mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon2_0));
        mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon2_0));
        mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value_2);
        mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value_2);
        mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
        mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_2);
        vst1q_s16(temp_output_data_2 + j, out_value_neon_16Bit);

        mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon2_1));
        mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon2_1));
        mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value_2);
        mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value_2);
        mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
        mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_2);
        vst1q_s16(temp_output_data_2 + j + 8, out_value_neon_16Bit);

        mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon3_0));
        mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon3_0));
        mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value_3);
        mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value_3);
        mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
        mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_3);
        vst1q_s16(temp_output_data_3 + j, out_value_neon_16Bit);

        mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon3_1));
        mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon3_1));
        mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value_3);
        mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value_3);
        mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
        mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_3);
        vst1q_s16(temp_output_data_3 + j + 8, out_value_neon_16Bit);
    }

    if (j < search_img_end&&search_img_end - search_img_start >= 8)
    {
        int start_n[2] = { 0 };
        int end_loop_n = 0;
        start_n[0] = search_img_end - 8;
        if (j + 8 >= search_img_end)
            end_loop_n = 1;
        else
        {
            end_loop_n = 2;
            start_n[1] = j;
        }

        for (k = 0; k < end_loop_n; ++k)
        {
            int n = start_n[k];
            int8x8_t *weigth_neon_ptr_0 = weight_ptr_neon;
            int8x8_t in_value_neon;
            FixedConvType *ptr_0 = expand_input_data + n;

            out_value_neon0_0 = vdupq_n_s16(0);
            out_value_neon1_0 = vdupq_n_s16(0);
            out_value_neon2_0 = vdupq_n_s16(0);
            out_value_neon3_0 = vdupq_n_s16(0);
            for (m = 0; m < input_channel; ++m) {
                in_value_neon = vld1_s8(ptr_0);
                out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
                out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
                out_value_neon2_0 = vmlal_s8(out_value_neon2_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
                out_value_neon3_0 = vmlal_s8(out_value_neon3_0, in_value_neon, *weigth_neon_ptr_0);   weigth_neon_ptr_0++;
                ptr_0 += expand_size;
            }
            mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon0_0));
            mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon0_0));
            mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value_0);
            mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value_0);
            mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
            mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
            out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
            out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
            out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
            out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_0);
            vst1q_s16(temp_output_data_0 + n, out_value_neon_16Bit);

            mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon1_0));
            mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon1_0));
            mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value_1);
            mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value_1);
            mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
            mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
            out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
            out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
            out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
            out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_1);
            vst1q_s16(temp_output_data_1 + n, out_value_neon_16Bit);

            mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon2_0));
            mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon2_0));
            mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value_2);
            mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value_2);
            mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
            mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
            out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
            out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
            out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
            out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_2);
            vst1q_s16(temp_output_data_2 + n, out_value_neon_16Bit);

            mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon3_0));
            mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon3_0));
            mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value_3);
            mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value_3);
            mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
            mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
            out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
            out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
            out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
            out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_3);
            vst1q_s16(temp_output_data_3 + n, out_value_neon_16Bit);
        }
    }
    else
#endif
    {
        for (; j < search_img_end; ++j)
        {
            short value_0 = 0;
            short value_1 = 0;
            short value_2 = 0;
            short value_3 = 0;
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
#ifdef CNN_ARM_NEON_PROCESSING
    EAGLEEYE_SAFEFREE(weight_ptr_neon);
#endif
}

void GetOneOutputChannelResult1x1_16Bit(FixedConvType *weight_ptr, FixedConvType *expand_input_data, int input_channel, int expand_width,
                                  int expand_height, int output_height, FixedType *temp_output_data, int pad_size,
                                  FixedBiasType bias_value, int adjust_div_value, int norm_half_add_value)
{
    int j = 0, k = 0, m = 0;
    int search_img_start = 0;
    int search_img_end = output_height*expand_width - 2 * pad_size;
    int expand_size = expand_height*expand_width;
    int search_8_end = (search_img_end - search_img_start) / 8 * 8 + search_img_start;
    const int data_norm_move_const = CNN_CONV_DATA_NORM_MOVE;

    j = search_img_start;

#ifdef CNN_ARM_NEON_PROCESSING
    int8x8_t *weight_ptr_neon;
    int8x8_t in_value_neon;
    int16x8_t out_value_neon;
    int16x8_t fixed_bias_value_neon;
    int32x4_t norm_half_add_value_neon, mul_value_0, mul_value_1;
    int16x4_t out_value_neon_16Bit_0, out_value_neon_16Bit_1;
    int16x8_t out_value_neon_16Bit;
    int loop_size = input_channel;
    norm_half_add_value_neon = vdupq_n_s32(norm_half_add_value);
    fixed_bias_value_neon = vdupq_n_s16(bias_value);

    weight_ptr_neon = (int8x8_t *)malloc(sizeof(int8x8_t)*loop_size);
    if (weight_ptr_neon == NULL)
        return;

    for (k = 0; k < loop_size; ++k)
        weight_ptr_neon[k] = vdup_n_s8(weight_ptr[k]);

    for (j = 0; j < search_8_end; j += 8)
    {
        FixedConvType *ptr_0 = expand_input_data + j;

        out_value_neon = vdupq_n_s16(0);
        int8x8_t *weigth_neon_ptr_0 = weight_ptr_neon;
        for (m = 0; m < input_channel; ++m)
        {
            in_value_neon = vld1_s8(ptr_0);
            out_value_neon = vmlal_s8(out_value_neon, in_value_neon, *weigth_neon_ptr_0);  weigth_neon_ptr_0++;

            ptr_0 += expand_size;
        }
        mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon));
        mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon));
        mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value);
        mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value);
        mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
        mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon);
        vst1q_s16(temp_output_data + j, out_value_neon_16Bit);
    }

    if (j < search_img_end&&search_img_end - search_img_start >= 8)
    {
        j = search_img_end - 8;

        FixedConvType *ptr_0 = expand_input_data + j;
        int8x8_t *weigth_neon_ptr_0 = weight_ptr_neon;
        out_value_neon = vdupq_n_s16(0);
        for (m = 0; m < input_channel; ++m)
        {
            in_value_neon = vld1_s8(ptr_0);
            out_value_neon = vmlal_s8(out_value_neon, in_value_neon, *weigth_neon_ptr_0);  weigth_neon_ptr_0++;

            ptr_0 += expand_size;
        }
        mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon));
        mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon));
        mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value);
        mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value);
        mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
        mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
        out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
        out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
        out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
        out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon);
        vst1q_s16(temp_output_data + j, out_value_neon_16Bit);
    }
    else
#endif
    {
        for (; j < search_img_end; ++j)
        {
            short value_0 = 0;
            FixedConvType *ptr_0 = expand_input_data + j;
            FixedConvType *weight_0 = weight_ptr;

            for (m = 0; m < input_channel; ++m)
            {
                value_0 += (*ptr_0)*(*weight_0); weight_0++;
                ptr_0 += expand_size;
            }
            temp_output_data[j] = ((value_0 * adjust_div_value + norm_half_add_value) >> data_norm_move_const) + bias_value;
        }
    }
#ifdef CNN_ARM_NEON_PROCESSING
    if (weight_ptr_neon)
        free(weight_ptr_neon);
#endif
}

void ConvMatrixMulti_16Bit(FixedConvType *fixed_weight, int row1, int col1, FixedConvType *data_col, int row2, int col2, FixedType *net_output_data,
                           int *adjust_div_value, int norm_half_add_value, FixedBiasType *fixed_bias_value)
{
#ifdef CNN_ARM_NEON_PROCESSING
    int i = 0, j = 0, k = 0;
    int8x8_t in_value_neon_m1_0, in_value_neon_m1_1, in_value_neon_m1_2, in_value_neon_m1_3, in_value_neon_m2;
    int16x8_t mul_value_neon;
    int16x8_t out_value_neon_0, out_value_neon_1, out_value_neon_2, out_value_neon_3, sum_value_neon;
    int32x4_t add_value_0;
    int64x2_t add_value_1;
    int search_8_end = col1 / 8 * 8;
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
            out_value_neon_0 = vdupq_n_s16(0);
            out_value_neon_1 = vdupq_n_s16(0);
            out_value_neon_2 = vdupq_n_s16(0);
            out_value_neon_3 = vdupq_n_s16(0);
            for (k = 0; k < search_8_end; k += 8)
            {
                in_value_neon_m1_0 = vld1_s8(p1_0);
                in_value_neon_m1_1 = vld1_s8(p1_1);
                in_value_neon_m1_2 = vld1_s8(p1_2);
                in_value_neon_m1_3 = vld1_s8(p1_3);
                in_value_neon_m2 = vld1_s8(p2_0);
                out_value_neon_0 = vmlal_s8(out_value_neon_0, in_value_neon_m1_0, in_value_neon_m2);
                out_value_neon_1 = vmlal_s8(out_value_neon_1, in_value_neon_m1_1, in_value_neon_m2);
                out_value_neon_2 = vmlal_s8(out_value_neon_2, in_value_neon_m1_2, in_value_neon_m2);
                out_value_neon_3 = vmlal_s8(out_value_neon_3, in_value_neon_m1_3, in_value_neon_m2);

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
            add_value_0 = vpaddlq_s16(out_value_neon_0);
            add_value_1 = vpaddlq_s32(add_value_0);
            sum_0 = vgetq_lane_s64(add_value_1, 0) +vgetq_lane_s64(add_value_1, 1);
            add_value_0 = vpaddlq_s16(out_value_neon_1);
            add_value_1 = vpaddlq_s32(add_value_0);
            sum_1 = vgetq_lane_s64(add_value_1, 0) +vgetq_lane_s64(add_value_1, 1);
            add_value_0 = vpaddlq_s16(out_value_neon_2);
            add_value_1 = vpaddlq_s32(add_value_0);
            sum_2 = vgetq_lane_s64(add_value_1, 0) +vgetq_lane_s64(add_value_1, 1);
            add_value_0 = vpaddlq_s16(out_value_neon_3);
            add_value_1 = vpaddlq_s32(add_value_0);
            sum_3 = vgetq_lane_s64(add_value_1, 0) +vgetq_lane_s64(add_value_1, 1);

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
            out_value_neon_0 = vdupq_n_s16(0);
            for (k = 0; k < search_8_end; k += 8)
            {
                in_value_neon_m1_0 = vld1_s8(p1_0);
                in_value_neon_m2 = vld1_s8(p2_0);
                out_value_neon_0 = vmlal_s8(out_value_neon_0, in_value_neon_m1_0, in_value_neon_m2);
                p1_0 += 8;
                p2_0 += 8;
            }
            int sum_0 = 0;
            add_value_0 = vpaddlq_s16(out_value_neon_0);
            add_value_1 = vpaddlq_s32(add_value_0);
            sum_0 = vgetq_lane_s64(add_value_1, 0) +vgetq_lane_s64(add_value_1, 1);
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

void GetMultiOutputChannelResult3x3_Stride2_2_16Bit(FixedConvType *fixed_weight, FixedConvType *expand_input_data, int input_channel, int expand_width,
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

#ifdef CNN_ARM_NEON_PROCESSING
    int8x8_t *weight_ptr_neon = NULL;
    int search_16_end = output_width / 16 * 16;
    int8x16x2_t in_value_neon;
    int8x8_t in_value_neon_0, in_value_neon_1;
    int16x8_t out_value_neon0_0, out_value_neon0_1;
    int16x8_t out_value_neon1_0, out_value_neon1_1;
    int16x4_t out_value_neon_16Bit_0, out_value_neon_16Bit_1;
    int16x8_t mul_value_0, mul_value_1, out_value_neon_16Bit, fixed_bias_value_neon_0, fixed_bias_value_neon_1;
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
        for (k = 0; k < search_16_end; k += 16)
        {
            int8x8_t *weigth_neon_ptr = weight_ptr_neon;
            FixedConvType *ptr_0 = expand_input_data + n;
            FixedConvType *ptr_1 = expand_input_data + n + expand_width;
            FixedConvType *ptr_2 = expand_input_data + n + expand_width * 2;
            n += 32;

            out_value_neon0_0 = vdupq_n_s16(0);
            out_value_neon0_1 = vdupq_n_s16(0);
            out_value_neon1_0 = vdupq_n_s16(0);
            out_value_neon1_1 = vdupq_n_s16(0);
            for (m = 0; m < input_channel; m++)
            {
                FixedConvType *ptr = ptr_0;
                in_value_neon = vld2q_s8(ptr);
                in_value_neon_0 = vget_low_s8(in_value_neon.val[0]);
                in_value_neon_1 = vget_high_s8(in_value_neon.val[0]);
                out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_0, *weigth_neon_ptr);
                out_value_neon0_1 = vmlal_s8(out_value_neon0_1, in_value_neon_1, *weigth_neon_ptr);   weigth_neon_ptr++;
                out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon_0, *weigth_neon_ptr);
                out_value_neon1_1 = vmlal_s8(out_value_neon1_1, in_value_neon_1, *weigth_neon_ptr);   weigth_neon_ptr++;
                in_value_neon_0 = vget_low_s8(in_value_neon.val[1]);
                in_value_neon_1 = vget_high_s8(in_value_neon.val[1]);
                out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_0, *weigth_neon_ptr);
                out_value_neon0_1 = vmlal_s8(out_value_neon0_1, in_value_neon_1, *weigth_neon_ptr);   weigth_neon_ptr++;
                out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon_0, *weigth_neon_ptr);
                out_value_neon1_1 = vmlal_s8(out_value_neon1_1, in_value_neon_1, *weigth_neon_ptr);   weigth_neon_ptr++;
                ptr += 2;
                in_value_neon = vld2q_s8(ptr);
                in_value_neon_0 = vget_low_s8(in_value_neon.val[0]);
                in_value_neon_1 = vget_high_s8(in_value_neon.val[0]);
                out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_0, *weigth_neon_ptr);
                out_value_neon0_1 = vmlal_s8(out_value_neon0_1, in_value_neon_1, *weigth_neon_ptr);   weigth_neon_ptr++;
                out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon_0, *weigth_neon_ptr);
                out_value_neon1_1 = vmlal_s8(out_value_neon1_1, in_value_neon_1, *weigth_neon_ptr);   weigth_neon_ptr++;

                ptr = ptr_1;
                in_value_neon = vld2q_s8(ptr);
                in_value_neon_0 = vget_low_s8(in_value_neon.val[0]);
                in_value_neon_1 = vget_high_s8(in_value_neon.val[0]);
                out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_0, *weigth_neon_ptr);
                out_value_neon0_1 = vmlal_s8(out_value_neon0_1, in_value_neon_1, *weigth_neon_ptr);   weigth_neon_ptr++;
                out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon_0, *weigth_neon_ptr);
                out_value_neon1_1 = vmlal_s8(out_value_neon1_1, in_value_neon_1, *weigth_neon_ptr);   weigth_neon_ptr++;
                in_value_neon_0 = vget_low_s8(in_value_neon.val[1]);
                in_value_neon_1 = vget_high_s8(in_value_neon.val[1]);
                out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_0, *weigth_neon_ptr);
                out_value_neon0_1 = vmlal_s8(out_value_neon0_1, in_value_neon_1, *weigth_neon_ptr);   weigth_neon_ptr++;
                out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon_0, *weigth_neon_ptr);
                out_value_neon1_1 = vmlal_s8(out_value_neon1_1, in_value_neon_1, *weigth_neon_ptr);   weigth_neon_ptr++;
                ptr += 2;
                in_value_neon = vld2q_s8(ptr);
                in_value_neon_0 = vget_low_s8(in_value_neon.val[0]);
                in_value_neon_1 = vget_high_s8(in_value_neon.val[0]);
                out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_0, *weigth_neon_ptr);
                out_value_neon0_1 = vmlal_s8(out_value_neon0_1, in_value_neon_1, *weigth_neon_ptr);   weigth_neon_ptr++;
                out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon_0, *weigth_neon_ptr);
                out_value_neon1_1 = vmlal_s8(out_value_neon1_1, in_value_neon_1, *weigth_neon_ptr);   weigth_neon_ptr++;

                ptr = ptr_2;
                in_value_neon = vld2q_s8(ptr);
                in_value_neon_0 = vget_low_s8(in_value_neon.val[0]);
                in_value_neon_1 = vget_high_s8(in_value_neon.val[0]);
                out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_0, *weigth_neon_ptr);
                out_value_neon0_1 = vmlal_s8(out_value_neon0_1, in_value_neon_1, *weigth_neon_ptr);   weigth_neon_ptr++;
                out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon_0, *weigth_neon_ptr);
                out_value_neon1_1 = vmlal_s8(out_value_neon1_1, in_value_neon_1, *weigth_neon_ptr);   weigth_neon_ptr++;
                in_value_neon_0 = vget_low_s8(in_value_neon.val[1]);
                in_value_neon_1 = vget_high_s8(in_value_neon.val[1]);
                out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_0, *weigth_neon_ptr);
                out_value_neon0_1 = vmlal_s8(out_value_neon0_1, in_value_neon_1, *weigth_neon_ptr);   weigth_neon_ptr++;
                out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon_0, *weigth_neon_ptr);
                out_value_neon1_1 = vmlal_s8(out_value_neon1_1, in_value_neon_1, *weigth_neon_ptr);   weigth_neon_ptr++;
                ptr += 2;
                in_value_neon = vld2q_s8(ptr);
                in_value_neon_0 = vget_low_s8(in_value_neon.val[0]);
                in_value_neon_1 = vget_high_s8(in_value_neon.val[0]);
                out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_0, *weigth_neon_ptr);
                out_value_neon0_1 = vmlal_s8(out_value_neon0_1, in_value_neon_1, *weigth_neon_ptr);   weigth_neon_ptr++;
                out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon_0, *weigth_neon_ptr);
                out_value_neon1_1 = vmlal_s8(out_value_neon1_1, in_value_neon_1, *weigth_neon_ptr);   weigth_neon_ptr++;

                ptr_0 += expand_size;
                ptr_1 += expand_size;
                ptr_2 += expand_size;
            }

            mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon0_0));
            mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon0_0));
            mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value_0);
            mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value_0);
            mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
            mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
            out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
            out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
            out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
            out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_0);
            vst1q_s16(p_out_buf_0 + j*output_width + k, out_value_neon_16Bit);

            mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon0_1));
            mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon0_1));
            mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value_0);
            mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value_0);
            mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
            mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
            out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
            out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
            out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
            out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_0);
            vst1q_s16(p_out_buf_0 + j*output_width + k + 8, out_value_neon_16Bit);

            mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon1_0));
            mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon1_0));
            mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value_1);
            mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value_1);
            mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
            mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
            out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
            out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
            out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
            out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_1);
            vst1q_s16(p_out_buf_1 + j*output_width + k, out_value_neon_16Bit);

            mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon1_1));
            mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon1_1));
            mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value_1);
            mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value_1);
            mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
            mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
            out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
            out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
            out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
            out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_1);
            vst1q_s16(p_out_buf_1 + j*output_width + k + 8, out_value_neon_16Bit);
        }
        if (k < output_width&&output_width >= 8)
        {
            int8x8x2_t in_value_neon_8bit;
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
                out_value_neon0_0 = vdupq_n_s32(0);
                out_value_neon1_0 = vdupq_n_s32(0);
                n = j * 2 * expand_width + nn*2;

                int8x8_t *weigth_neon_ptr = weight_ptr_neon;
                FixedConvType *ptr_0 = expand_input_data + n;
                FixedConvType *ptr_1 = expand_input_data + n + expand_width;
                FixedConvType *ptr_2 = expand_input_data + n + expand_width * 2;

                for (m = 0; m < input_channel; ++m)
                {
                    FixedConvType *ptr = ptr_0;
                    in_value_neon_8bit = vld2_s8(ptr);
                    out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_8bit.val[0], *weigth_neon_ptr); weigth_neon_ptr++;
                    out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon_8bit.val[0], *weigth_neon_ptr); weigth_neon_ptr++;
                    out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_8bit.val[1], *weigth_neon_ptr); weigth_neon_ptr++;
                    out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon_8bit.val[1], *weigth_neon_ptr); weigth_neon_ptr++;
                    ptr+=2;
                    in_value_neon_8bit = vld2_s8(ptr);
                    out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_8bit.val[0], *weigth_neon_ptr); weigth_neon_ptr++;
                    out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon_8bit.val[0], *weigth_neon_ptr); weigth_neon_ptr++;

                    ptr = ptr_1;
                    in_value_neon_8bit = vld2_s8(ptr);
                    out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_8bit.val[0], *weigth_neon_ptr); weigth_neon_ptr++;
                    out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon_8bit.val[0], *weigth_neon_ptr); weigth_neon_ptr++;
                    out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_8bit.val[1], *weigth_neon_ptr); weigth_neon_ptr++;
                    out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon_8bit.val[1], *weigth_neon_ptr); weigth_neon_ptr++;
                    ptr+=2;
                    in_value_neon_8bit = vld2_s8(ptr);
                    out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_8bit.val[0], *weigth_neon_ptr); weigth_neon_ptr++;
                    out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon_8bit.val[0], *weigth_neon_ptr); weigth_neon_ptr++;

                    ptr = ptr_2;
                    in_value_neon_8bit = vld2_s8(ptr);
                    out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_8bit.val[0], *weigth_neon_ptr); weigth_neon_ptr++;
                    out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon_8bit.val[0], *weigth_neon_ptr); weigth_neon_ptr++;
                    out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_8bit.val[1], *weigth_neon_ptr); weigth_neon_ptr++;
                    out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon_8bit.val[1], *weigth_neon_ptr); weigth_neon_ptr++;
                    ptr+=2;
                    in_value_neon_8bit = vld2_s8(ptr);
                    out_value_neon0_0 = vmlal_s8(out_value_neon0_0, in_value_neon_8bit.val[0], *weigth_neon_ptr); weigth_neon_ptr++;
                    out_value_neon1_0 = vmlal_s8(out_value_neon1_0, in_value_neon_8bit.val[0], *weigth_neon_ptr); weigth_neon_ptr++;

                    ptr_0 += expand_size;
                    ptr_1 += expand_size;
                    ptr_2 += expand_size;
                }
                mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon0_0));
                mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon0_0));
                mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value_0);
                mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value_0);
                mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
                mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
                out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
                out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
                out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
                out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_0);
                vst1q_s16(p_out_buf_0 + j*output_width + nn, out_value_neon_16Bit);

                mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon1_0));
                mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon1_0));
                mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value_1);
                mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value_1);
                mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
                mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
                out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
                out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
                out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
                out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon_1);
                vst1q_s16(p_out_buf_1 + j*output_width + nn, out_value_neon_16Bit);
            }
        }
        else
        {
            n = j * 2 * expand_width + k*2;

            for (; k < output_width; ++k)
            {
                short value_0 = 0;
                short value_1 = 0;
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
            short value_0 = 0;
            short value_1 = 0;
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
void GetOneOutputChannelResult3x3_Stride2_16Bit(FixedConvType *weight_ptr, FixedConvType *expand_input_data, int input_channel, int expand_width,
                                                int expand_height, int output_width, int output_height, FixedType *output_data, int pad_size,
                                                FixedBiasType bias_value, int adjust_div_value, int norm_half_add_value)
{
    int i = 0, j = 0, k = 0, l = 0, m = 0, n = 0;
    const int data_norm_move_const = CNN_CONV_DATA_NORM_MOVE;
    int expand_size = expand_width*expand_height;

#ifdef CNN_ARM_NEON_PROCESSING
    int8x8_t *weight_ptr_neon;
    int search_8_end = output_width / 16 * 16;

    weight_ptr_neon = (int8x8_t *)malloc(sizeof(int8x8_t)*input_channel * 9);
    if (weight_ptr_neon == NULL)
        return;

    for (i = 0; i < input_channel * 9; ++i)
        weight_ptr_neon[i] = vdup_n_s8(weight_ptr[i]);

    int8x8x2_t in_value_neon_0;
    int8x16x2_t in_value_neon;
    int16x8_t out_value_neon_0, out_value_neon_1;
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

            out_value_neon_0 = vdupq_n_s16(0);
            out_value_neon_1 = vdupq_n_s16(0);
            for (m = 0; m < input_channel; m++)
            {
                FixedConvType *ptr = ptr_0;
                in_value_neon = vld2q_s8(ptr);
                out_value_neon_0 = vmlal_s8(out_value_neon_0, vget_low_s8(in_value_neon.val[0]), *weigth_neon_ptr);
                out_value_neon_1 = vmlal_s8(out_value_neon_1, vget_high_s8(in_value_neon.val[0]), *weigth_neon_ptr);
                weigth_neon_ptr++;
                out_value_neon_0 = vmlal_s8(out_value_neon_0, vget_low_s8(in_value_neon.val[1]), *weigth_neon_ptr);
                out_value_neon_1 = vmlal_s8(out_value_neon_1, vget_high_s8(in_value_neon.val[1]), *weigth_neon_ptr);
                weigth_neon_ptr++;
                ptr += 2;
                in_value_neon = vld2q_s8(ptr);
                out_value_neon_0 = vmlal_s8(out_value_neon_0, vget_low_s8(in_value_neon.val[0]), *weigth_neon_ptr);
                out_value_neon_1 = vmlal_s8(out_value_neon_1, vget_high_s8(in_value_neon.val[0]), *weigth_neon_ptr);
                weigth_neon_ptr++;

                ptr = ptr_1;
                in_value_neon = vld2q_s8(ptr);
                out_value_neon_0 = vmlal_s8(out_value_neon_0, vget_low_s8(in_value_neon.val[0]), *weigth_neon_ptr);
                out_value_neon_1 = vmlal_s8(out_value_neon_1, vget_high_s8(in_value_neon.val[0]), *weigth_neon_ptr);
                weigth_neon_ptr++;
                out_value_neon_0 = vmlal_s8(out_value_neon_0, vget_low_s8(in_value_neon.val[1]), *weigth_neon_ptr);
                out_value_neon_1 = vmlal_s8(out_value_neon_1, vget_high_s8(in_value_neon.val[1]), *weigth_neon_ptr);
                weigth_neon_ptr++;
                ptr += 2;
                in_value_neon = vld2q_s8(ptr);
                out_value_neon_0 = vmlal_s8(out_value_neon_0, vget_low_s8(in_value_neon.val[0]), *weigth_neon_ptr);
                out_value_neon_1 = vmlal_s8(out_value_neon_1, vget_high_s8(in_value_neon.val[0]), *weigth_neon_ptr);
                weigth_neon_ptr++;

                ptr = ptr_2;
                in_value_neon = vld2q_s8(ptr);
                out_value_neon_0 = vmlal_s8(out_value_neon_0, vget_low_s8(in_value_neon.val[0]), *weigth_neon_ptr);
                out_value_neon_1 = vmlal_s8(out_value_neon_1, vget_high_s8(in_value_neon.val[0]), *weigth_neon_ptr);
                weigth_neon_ptr++;
                out_value_neon_0 = vmlal_s8(out_value_neon_0, vget_low_s8(in_value_neon.val[1]), *weigth_neon_ptr);
                out_value_neon_1 = vmlal_s8(out_value_neon_1, vget_high_s8(in_value_neon.val[1]), *weigth_neon_ptr);
                weigth_neon_ptr++;
                ptr += 2;
                in_value_neon = vld2q_s8(ptr);
                out_value_neon_0 = vmlal_s8(out_value_neon_0, vget_low_s8(in_value_neon.val[0]), *weigth_neon_ptr);
                out_value_neon_1 = vmlal_s8(out_value_neon_1, vget_high_s8(in_value_neon.val[0]), *weigth_neon_ptr);
                weigth_neon_ptr++;

                ptr_0 += expand_size;
                ptr_1 += expand_size;
                ptr_2 += expand_size;
            }

            mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon_0));
            mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon_0));
            mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value);
            mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value);
            mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
            mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
            out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
            out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
            out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
            out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon);
            vst1q_s16(p_out_buf + k, out_value_neon_16Bit);

            mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon_1));
            mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon_1));
            mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value);
            mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value);
            mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
            mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
            out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
            out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
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
                n = j * 2 * expand_width + nn*2;

                int8x8_t *weigth_neon_ptr = weight_ptr_neon;
                FixedConvType *ptr_0 = expand_input_data + n;
                FixedConvType *ptr_1 = expand_input_data + n + expand_width;
                FixedConvType *ptr_2 = expand_input_data + n + expand_width * 2;

                for (m = 0; m < input_channel; ++m)
                {
                    FixedConvType *ptr = ptr_0;
                    in_value_neon_0 = vld2_s8(ptr);
                    out_value_neon_0 = vmlal_s8(out_value_neon_0, in_value_neon_0.val[0], *weigth_neon_ptr);
                    weigth_neon_ptr++;
                    out_value_neon_0 = vmlal_s8(out_value_neon_0, in_value_neon_0.val[1], *weigth_neon_ptr);
                    weigth_neon_ptr++;
                    ptr+=2;
                    in_value_neon_0 = vld2_s8(ptr);
                    out_value_neon_0 = vmlal_s8(out_value_neon_0, in_value_neon_0.val[0], *weigth_neon_ptr);
                    weigth_neon_ptr++;

                    ptr = ptr_1;
                    in_value_neon_0 = vld2_s8(ptr);
                    out_value_neon_0 = vmlal_s8(out_value_neon_0, in_value_neon_0.val[0], *weigth_neon_ptr);
                    weigth_neon_ptr++;
                    out_value_neon_0 = vmlal_s8(out_value_neon_0, in_value_neon_0.val[1], *weigth_neon_ptr);
                    weigth_neon_ptr++;
                    ptr+=2;
                    in_value_neon_0 = vld2_s8(ptr);
                    out_value_neon_0 = vmlal_s8(out_value_neon_0, in_value_neon_0.val[0], *weigth_neon_ptr);
                    weigth_neon_ptr++;

                    ptr = ptr_2;
                    in_value_neon_0 = vld2_s8(ptr);
                    out_value_neon_0 = vmlal_s8(out_value_neon_0, in_value_neon_0.val[0], *weigth_neon_ptr);
                    weigth_neon_ptr++;
                    out_value_neon_0 = vmlal_s8(out_value_neon_0, in_value_neon_0.val[1], *weigth_neon_ptr);
                    weigth_neon_ptr++;
                    ptr+=2;
                    in_value_neon_0 = vld2_s8(ptr);
                    out_value_neon_0 = vmlal_s8(out_value_neon_0, in_value_neon_0.val[0], *weigth_neon_ptr);
                    weigth_neon_ptr++;

                    ptr_0 += expand_size;
                    ptr_1 += expand_size;
                    ptr_2 += expand_size;
                }
                mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon_0));
                mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon_0));
                mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value);
                mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value);
                mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
                mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
                out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
                out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
                out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
                out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon);
                vst1q_s16(p_out_buf + k, out_value_neon_16Bit);

                vst1q_s16(p_out_buf + nn, out_value_neon_16Bit);
            }
        }
        else
        {
            for (; k < output_width; ++k)
            {
                n = j * 2 * expand_width + k*2;
                short value = 0;
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
			short value = 0;
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

bool GetOneOutputChannelResult_Stride2_16Bit(FixedConvType *weight_ptr, FixedConvType **input_ptr, int input_channel, int kernel_size_sq, int w_stride_, int h_stride_,
                                             int expand_width, int expand_height, int output_width, int output_height, FixedType *p_in_out_buf, int pad_size, FixedBiasType bias_value,
                                             int adjust_div_value, int norm_half_add_value)
{
    int i = 0, j = 0, k = 0, m = 0;

#ifdef CNN_ARM_NEON_PROCESSING
    int8x8_t *weight_ptr_neon = NULL;
    int8x16x2_t in_value_neon_0;
    int16x8_t out_value_neon_0, out_value_neon_1;
    int16x4_t out_value_neon_16Bit_0, out_value_neon_16Bit_1;
    int16x8_t mul_value_0, mul_value_1, out_value_neon_16Bit, fixed_bias_value_neon;
    int32x4_t norm_half_add_value_neon;
    int search_8_end = output_width/ 16 * 16;
    int loop_size = input_channel*kernel_size_sq;
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
            out_value_neon_0 = vdupq_n_s16(0);
            out_value_neon_1 = vdupq_n_s16(0);

            for (m = 0; m < loop_size; ++m)
            {
                in_value_neon_0 = vld2q_s8(input_ptr[m] + n);
                out_value_neon_0 = vmlal_s8(out_value_neon_0, vget_low_s8(in_value_neon_0.val[0]), weight_ptr_neon[m]);
                out_value_neon_1 = vmlal_s8(out_value_neon_1, vget_high_s8(in_value_neon_0.val[0]), weight_ptr_neon[m]);
            }
            n += 32;
            mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon_0));
            mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon_0));
            mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value);
            mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value);
            mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
            mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
            out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
            out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
            out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
            out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon);
            vst1q_s16(p_out_buf + k, out_value_neon_16Bit);

            mul_value_0 = vmovl_s16(vget_low_s16(out_value_neon_1));
            mul_value_1 = vmovl_s16(vget_high_s16(out_value_neon_1));
            mul_value_0 = vmulq_n_s32(mul_value_0, adjust_div_value);
            mul_value_1 = vmulq_n_s32(mul_value_1, adjust_div_value);
            mul_value_0 = vaddq_s32(mul_value_0, norm_half_add_value_neon);
            mul_value_1 = vaddq_s32(mul_value_1, norm_half_add_value_neon);
            out_value_neon_16Bit_0 = vshrn_n_s32(mul_value_0, data_norm_move_const);
            out_value_neon_16Bit_1 = vshrn_n_s32(mul_value_1, data_norm_move_const);
            out_value_neon_16Bit = vcombine_s16(out_value_neon_16Bit_0, out_value_neon_16Bit_1);
            out_value_neon_16Bit = vaddq_s16(out_value_neon_16Bit, fixed_bias_value_neon);
            vst1q_s16(p_out_buf + k + 8, out_value_neon_16Bit);
        }

        {
            for( ; k < output_width; ++k)
            {
                short value = 0;
                for (m = 0; m < loop_size; ++m)
                    value += input_ptr[m][n] * weight_ptr[m];
                n += w_stride_;
                p_out_buf[k] = ((value * adjust_div_value + norm_half_add_value) >> data_norm_move_const) + bias_value;
            }
        }
    }

    MM_SafeFree(weight_ptr_neon);
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
            short value = 0;
            for (m = 0; m < loop_size; ++m)
                value += input_ptr[m][n] * weight_ptr[m];
            n += w_stride_;
            p_out_buf[k] = ((value * adjust_div_value + norm_half_add_value) >> data_norm_move_const) + bias_value;
        }
    }
#endif

    return true;
}    
} // namespace eagleeye