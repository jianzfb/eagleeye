#include "eagleeye/engine/nano/op/FixedResizeOp.h"
#include "eagleeye/engine/nano/util/quantization.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeTime.h"
#if defined (__ARM_NEON) || defined (__ARM_NEON__)
#include <arm_neon.h>
#define QRBAR_SHIFTBITS    7
#define QRBAR_ROUND0(x)  (x>>QRBAR_SHIFTBITS)
#define QRBAR_ROUND1(x)  (ROUND0(x))+1
#ifndef QRBAR_CLIP
#define QRBAR_CLIP(x) ( x<0 ? 0 : (x>255 ? 255 : x) )
#endif
#endif
namespace eagleeye
{
namespace nano
{
FixedResizeOp::FixedResizeOp(int input_data_num, int output_data_num, std::string op_name)
             :FixedCNNOp(input_data_num, output_data_num, FIXED_RESIZE, op_name){
    this->m_des_width = 40;
    this->m_des_height = 40;
}    

FixedResizeOp::~FixedResizeOp(){

}

void FixedResizeOp::foward_on_cpu(std::vector<Tensor<FixedType>>& output, 
                                  std::vector<Tensor<FixedType>>& input){
    int input_data_size = input[0].size();
    int output_data_size = output[0].size();

    FixedType* net_input = (FixedType*)input[0].cpu();
    FixedType* net_output = (FixedType*)output[0].cpu();

    // input[0].dim(0) == 1
    int C = input[0].dim(1);
    int H = input[0].dim(2);
    int W = input[0].dim(3);
    FixedOpType* op_input_data = (FixedOpType*)malloc(sizeof(FixedOpType)*input_data_size);

    // 1.step quantization
    quant_sym_16b_8b(net_input,op_input_data,H,W,C);

    // 2.step resize
#if defined (__ARM_NEON) || defined (__ARM_NEON__)
    int offset = 0;
    int HW = H*W;
    short *coord_x, *coord_y;
	signed char *bi_coef_x, *bi_coef_y;
	signed char *sub_bi_coef_x, *sub_bi_coef_y;
	signed char *feature_map_left_top, *feature_map_left_down;
	signed char *feature_map_right_top, *feature_map_right_down;
    const signed char *pSrc, *pSrc1;

	int float_value;
	const short std_1_value = (1 << QRBAR_SHIFTBITS);
	const short std_and_value = ((1 << QRBAR_SHIFTBITS) - 1);

	feature_map_left_top = (signed char *)malloc(sizeof(char)*this->m_des_width);
	feature_map_left_down = (signed char *)malloc(sizeof(char)*this->m_des_width);
	feature_map_right_top = (signed char *)malloc(sizeof(char)*this->m_des_width);
	feature_map_right_down = (signed char *)malloc(sizeof(char)*this->m_des_width);

	coord_x = (short *)malloc(sizeof(short)*this->m_des_width);
	coord_y = (short *)malloc(sizeof(short)*this->m_des_height);
	bi_coef_x = (signed char *)malloc(sizeof(char)*this->m_des_width);
	bi_coef_y = (signed char *)malloc(sizeof(char)*this->m_des_height);
	sub_bi_coef_x = (signed char *)malloc(sizeof(char)*this->m_des_width);
	sub_bi_coef_y = (signed char *)malloc(sizeof(char)*this->m_des_height);

    int nRateW = (W << QRBAR_SHIFTBITS) / (this->m_des_width);
    int nRateH = (H << QRBAR_SHIFTBITS) / (this->m_des_height);
    int i,j;

	for (i = 0; i < this->m_des_height; ++i){
		float_value = i * nRateH;
		int a = float_value & std_and_value;        
		if (!a)
			a = 1;
        
        bi_coef_y[i] = a;
		sub_bi_coef_y[i] = std_1_value - a;
		coord_y[i] = QRBAR_ROUND0(float_value);

		if (coord_y[i] > H - 2)
			coord_y[i] = H - 2;
	}

	for (i = 0; i < this->m_des_width; ++i){
		float_value = i * nRateW;
		int b = float_value & std_and_value;
		if (!b)
			b = 1;
        
        bi_coef_x[i] = b;
		sub_bi_coef_x[i] = std_1_value - b;
		coord_x[i] = QRBAR_ROUND0(float_value);

		if (coord_x[i] > W - 2)
			coord_x[i] = W - 2;
	}

    // int16x4_t zero_point = vdup_n_s16(127<<QRBAR_SHIFTBITS);
    int16_t scale = (int16_t)(float(QUANTIZER_DATA_NORM_VALUE) / QUANTIZER_MAX_NORM_VALUE + 0.5f); 
    int16x4_t vscale = vdup_n_s16(scale);

    for(int c=0; c<C; ++c){
        offset = c*this->m_des_height*this->m_des_width;
        FixedType* ptr_out = net_output + offset;

        offset = c*HW;
        FixedOpType *ptr_in = op_input_data + offset;

        for (i = 0; i < this->m_des_height; i++){
            // pSrc = pSrcImg + coord_y[i] * srcWidth;
            pSrc = ptr_in + coord_y[i]*W;
            for (j = 0; j < this->m_des_width; j++){
                pSrc1 = pSrc;
                pSrc1 += coord_x[j];
                feature_map_left_top[j] = *pSrc1;
                feature_map_right_top[j] = *(pSrc1 + 1);
                // pSrc1 += srcWidth;
                pSrc1 += W;
                feature_map_left_down[j] = *pSrc1;
                feature_map_right_down[j] = *(pSrc1 + 1);
            }

            signed char *img1, *img2, *img3, *img4;
            signed char *coef1, *coef2, *coef3, *coef4;
            signed char *coef_x1, *coef_x2;
            
            int8x8_t value_y1, value_y2;
            value_y1 = vdup_n_s8(sub_bi_coef_y[i]);
            value_y2 = vdup_n_s8(bi_coef_y[i]);
            img1 = feature_map_left_top;
            img2 = feature_map_right_top;
            img3 = feature_map_left_down;
            img4 = feature_map_right_down;
            ptr_out = net_output + offset + i*this->m_des_width;

            coef_x1 = sub_bi_coef_x;
            coef_x2 = bi_coef_x;
            for (j = 0; j < this->m_des_width; j += 8){
                int8x8_t v1, v2, v3, v4;
                int8x8_t value_x1, value_x2;
                int8x8_t bires_x0, bires_x1, bires_xy;
                int16x8_t multi_value;
                int16x4_t part1, part2;
                int32x4_t part11,part22;

                v1 = vld1_s8(img1);
                v2 = vld1_s8(img2);
                v3 = vld1_s8(img3);
                v4 = vld1_s8(img4);

                value_x1 = vld1_s8(coef_x1);
                value_x2 = vld1_s8(coef_x2);
                multi_value = vmull_s8(v1, value_x1);
                multi_value = vmlal_s8(multi_value, v2, value_x2);
                bires_x0 = vshrn_n_s16(multi_value, QRBAR_SHIFTBITS);
                multi_value = vmull_s8(v3, value_x1);
                multi_value = vmlal_s8(multi_value, v4, value_x2);
                bires_x1 = vshrn_n_s16(multi_value, QRBAR_SHIFTBITS);

                multi_value = vmull_s8(bires_x0, value_y1);
                multi_value = vmlal_s8(multi_value, bires_x1, value_y2);

                part1 = vget_low_s16(multi_value);
                part2 = vget_high_s16(multi_value);
                part11 = vmull_s16(part1, vscale);
                part22 = vmull_s16(part2, vscale);

                part1 = vshrn_n_s32(part11,QRBAR_SHIFTBITS);
                part2 = vshrn_n_s32(part22,QRBAR_SHIFTBITS);
                multi_value = vcombine_s16(part1,part2);
                vst1q_s16(ptr_out, multi_value);

                ptr_out += 8;
                img1 += 8;
                img2 += 8;
                img3 += 8;
                img4 += 8;
                coef_x1 += 8;
                coef_x2 += 8;
            } 
            for (; j < this->m_des_width; j++){
                short res_x0 = (((*img1)*sub_bi_coef_x[j] + (*img2)*bi_coef_x[j]) >> QRBAR_SHIFTBITS);
                short res_x1 = (((*img3)*sub_bi_coef_x[j] + (*img4)*bi_coef_x[j]) >> QRBAR_SHIFTBITS);
                short res_xy = res_x0*sub_bi_coef_y[i] + res_x1*bi_coef_y[i];

                *ptr_out = (res_xy >> QRBAR_SHIFTBITS)*scale;
                ptr_out++;
                img1++;
                img2++;
                img3++;
                img4++;
            }
        }
    }

    EAGLEEYE_SAFEFREE(feature_map_left_top);
    EAGLEEYE_SAFEFREE(feature_map_left_down);
    EAGLEEYE_SAFEFREE(feature_map_right_top);
    EAGLEEYE_SAFEFREE(feature_map_right_down);
    EAGLEEYE_SAFEFREE(coord_x);
    EAGLEEYE_SAFEFREE(coord_y);
    EAGLEEYE_SAFEFREE(bi_coef_x);
    EAGLEEYE_SAFEFREE(bi_coef_y);
    EAGLEEYE_SAFEFREE(sub_bi_coef_x);
    EAGLEEYE_SAFEFREE(sub_bi_coef_y);
    EAGLEEYE_SAFEFREE(op_input_data);
#endif
}   

void FixedResizeOp::foward_on_gpu(std::vector<Tensor<FixedType>>& output, 
                                    std::vector<Tensor<FixedType>>& input){

}

void FixedResizeOp::foward_on_dsp(std::vector<Tensor<FixedType>>& output, 
                                    std::vector<Tensor<FixedType>>& input){

}

bool FixedResizeOp::init(char* buf, int in_size){
    return false;
}

int FixedResizeOp::setShape(std::vector<int64_t> shape){
    return 1;
}

} // namespace nano
    
} // namespace eagleeye
