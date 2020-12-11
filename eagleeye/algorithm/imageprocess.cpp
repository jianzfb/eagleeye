#include "eagleeye/algorithm/imageprocess.h"
#ifdef EAGLEEYE_NEON_OPTIMIZATION    
#include <arm_neon.h>
#endif

namespace eagleeye
{
#ifdef EAGLEEYE_NEON_OPTIMIZATION    
struct MinMaxVec8u{   
    typedef uint8x16_t ElemType;
    inline void load(float* data, uint8x16_t& target, float32x4_t& scale){
        float32x4_t a32 = vld1q_f32(data);
        uint32x4_t a32u = vcvtq_u32_f32(vmulq_f32(a32, scale));
        uint16x4_t a16u = vmovn_u32(a32u);
       
        float32x4_t b32 = vld1q_f32(data+4);
        uint32x4_t b32u = vcvtq_u32_f32(vmulq_f32(b32, scale));
        uint16x4_t b16u = vmovn_u32(b32u);

        uint16x8_t ab16u = vcombine_u16(a16u, b16u);
        uint8x8_t ab8u_1 = vmovn_u16(ab16u);
       
        a32 = vld1q_f32(data+8);
        a32u = vcvtq_u32_f32(vmulq_f32(a32, scale));
        a16u = vmovn_u32(a32u);
       
        b32 = vld1q_f32(data+12);
        b32u = vcvtq_u32_f32(vmulq_f32(b32, scale));
        b16u = vmovn_u32(b32u);

        ab16u = vcombine_u16(a16u, b16u);
        uint8x8_t ab8u_2 = vmovn_u16(ab16u);

        target = vcombine_u8(ab8u_1, ab8u_2);
    }
    inline void store(unsigned char* data, uint8x16_t& target){
        vst1q_u8(data, target);
    };

    inline void operator()(uint8x16_t& a, uint8x16_t& b) const{
        uint8x16_t t = a;
        a = vminq_u8(a, b);
        b = vmaxq_u8(b, t);
    }
};

Matrix<float> medianFilterK5(Matrix<float> data){
    int height = data.rows();
    int width = data.cols();

    Matrix<float> filtered_fscore = data.clone();
    Matrix<unsigned char> filtered_score(height, width);
    memset(filtered_score.dataptr(), 0, sizeof(unsigned char)*height*width);
    
    unsigned char* output_buffer = (unsigned char*)filtered_score.dataptr();
    float* input_buffer = (float*)data.dataptr();

    int radius = 5 / 2; // radius = 2
    int step = 16;
    const int w_start = -radius; 
    const int w_end   = radius;
    const int h_start = -radius; 
    const int h_end = radius;
    const int w_loop_start = std::max(0, -w_start);
    const int w_loop_end   = (width - w_end - w_loop_start) / step * step + w_loop_start;
    const int h_loop_start = std::max(0, -h_start);
    const int h_loop_end = height - h_end;

    MinMaxVec8u op;
    float32x4_t quant_scale;
    quant_scale = vdupq_n_f32(255.0f);
    for (int h = h_loop_start; h < h_loop_end; ++h) {
        const int h_lower_bound = std::max(0, h + h_start);
        const int h_upper_bound = std::min(height - 1, h + h_end);

        typename MinMaxVec8u::ElemType p[25];
        for (int w = w_loop_start; w < w_loop_end; w += step) {
            const int w_left_bound = std::max(0, w + w_start);
            // row 0
            int base_offset = h_lower_bound*width + w_left_bound;
            int offset = base_offset;                   // 0
            op.load(input_buffer+offset, p[0], quant_scale);
            offset += 1;                                // 1
            op.load(input_buffer+offset, p[1], quant_scale);
            offset += 1;                                // 2
            op.load(input_buffer+offset, p[2], quant_scale);
            offset += 1;                                // 3
            op.load(input_buffer+offset, p[3], quant_scale);
            offset += 1;                                // 4
            op.load(input_buffer+offset, p[4], quant_scale);

            // row 1
            base_offset = (h_lower_bound+1)*width + w_left_bound;
            offset = base_offset;                       // 0
            op.load(input_buffer+offset, p[5], quant_scale);
            offset += 1;                                // 1
            op.load(input_buffer+offset, p[6], quant_scale);
            offset += 1;                                // 2
            op.load(input_buffer+offset, p[7], quant_scale);
            offset += 1;                                // 3
            op.load(input_buffer+offset, p[8], quant_scale);
            offset += 1;                                // 4
            op.load(input_buffer+offset, p[9], quant_scale);

            // row 2
            base_offset = (h_lower_bound+2)*width + w_left_bound;
            offset = base_offset;                       // 0
            op.load(input_buffer+offset, p[10], quant_scale);
            offset += 1;                                // 1
            op.load(input_buffer+offset, p[11], quant_scale);
            offset += 1;                                // 2
            op.load(input_buffer+offset, p[12], quant_scale);
            offset += 1;                                // 3
            op.load(input_buffer+offset, p[13], quant_scale);
            offset += 1;                                // 4
            op.load(input_buffer+offset, p[14], quant_scale);

            // row 3
            base_offset = (h_lower_bound+3)*width + w_left_bound;
            offset = base_offset;                       // 0
            op.load(input_buffer+offset, p[15], quant_scale);
            offset += 1;                                // 1
            op.load(input_buffer+offset, p[16], quant_scale);
            offset += 1;                                // 2
            op.load(input_buffer+offset, p[17], quant_scale);
            offset += 1;                                // 3
            op.load(input_buffer+offset, p[18], quant_scale);
            offset += 1;                                // 4
            op.load(input_buffer+offset, p[19], quant_scale);

            // row 4
            base_offset = (h_lower_bound+4)*width + w_left_bound;
            offset = base_offset;                       // 0
            op.load(input_buffer+offset, p[20], quant_scale);
            offset += 1;                                // 1
            op.load(input_buffer+offset, p[21], quant_scale);
            offset += 1;                                // 2
            op.load(input_buffer+offset, p[22], quant_scale);
            offset += 1;                                // 3
            op.load(input_buffer+offset, p[23], quant_scale);
            offset += 1;                                // 4
            op.load(input_buffer+offset, p[24], quant_scale);

            op(p[1], p[2]); op(p[0], p[1]); op(p[1], p[2]); op(p[4], p[5]); op(p[3], p[4]);
            op(p[4], p[5]); op(p[0], p[3]); op(p[2], p[5]); op(p[2], p[3]); op(p[1], p[4]);
            op(p[1], p[2]); op(p[3], p[4]); op(p[7], p[8]); op(p[6], p[7]); op(p[7], p[8]);
            op(p[10], p[11]); op(p[9], p[10]); op(p[10], p[11]); op(p[6], p[9]); op(p[8], p[11]);
            op(p[8], p[9]); op(p[7], p[10]); op(p[7], p[8]); op(p[9], p[10]); op(p[0], p[6]);
            op(p[4], p[10]); op(p[4], p[6]); op(p[2], p[8]); op(p[2], p[4]); op(p[6], p[8]);
            op(p[1], p[7]); op(p[5], p[11]); op(p[5], p[7]); op(p[3], p[9]); op(p[3], p[5]);
            op(p[7], p[9]); op(p[1], p[2]); op(p[3], p[4]); op(p[5], p[6]); op(p[7], p[8]);
            op(p[9], p[10]); op(p[13], p[14]); op(p[12], p[13]); op(p[13], p[14]); op(p[16], p[17]);
            op(p[15], p[16]); op(p[16], p[17]); op(p[12], p[15]); op(p[14], p[17]); op(p[14], p[15]);
            op(p[13], p[16]); op(p[13], p[14]); op(p[15], p[16]); op(p[19], p[20]); op(p[18], p[19]);
            op(p[19], p[20]); op(p[21], p[22]); op(p[23], p[24]); op(p[21], p[23]); op(p[22], p[24]);
            op(p[22], p[23]); op(p[18], p[21]); op(p[20], p[23]); op(p[20], p[21]); op(p[19], p[22]);
            op(p[22], p[24]); op(p[19], p[20]); op(p[21], p[22]); op(p[23], p[24]); op(p[12], p[18]);
            op(p[16], p[22]); op(p[16], p[18]); op(p[14], p[20]); op(p[20], p[24]); op(p[14], p[16]);
            op(p[18], p[20]); op(p[22], p[24]); op(p[13], p[19]); op(p[17], p[23]); op(p[17], p[19]);
            op(p[15], p[21]); op(p[15], p[17]); op(p[19], p[21]); op(p[13], p[14]); op(p[15], p[16]);
            op(p[17], p[18]); op(p[19], p[20]); op(p[21], p[22]); op(p[23], p[24]); op(p[0], p[12]);
            op(p[8], p[20]); op(p[8], p[12]); op(p[4], p[16]); op(p[16], p[24]); op(p[12], p[16]);
            op(p[2], p[14]); op(p[10], p[22]); op(p[10], p[14]); op(p[6], p[18]); op(p[6], p[10]);
            op(p[10], p[12]); op(p[1], p[13]); op(p[9], p[21]); op(p[9], p[13]); op(p[5], p[17]);
            op(p[13], p[17]); op(p[3], p[15]); op(p[11], p[23]); op(p[11], p[15]); op(p[7], p[19]);
            op(p[7], p[11]); op(p[11], p[13]); op(p[11], p[12]);

            op.store(output_buffer+h*width+w,p[12]);
        }
    }
    
    for(int i = h_loop_start; i < h_loop_end; ++i){
        float* filtered_fscore_ptr = filtered_fscore.row(i);
        unsigned char* filtered_score_ptr = filtered_score.row(i);
        for(int j = w_loop_start; j < w_loop_end; ++j){
            filtered_fscore_ptr[j] = filtered_score_ptr[j] / 255.0f;
        }
    }
    return filtered_fscore;
}
#endif       
} // namespace eagleeye
