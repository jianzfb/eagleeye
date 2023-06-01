#include "eagleeye/engine/math/x86/preprocess.h"
namespace eagleeye{
namespace math{
namespace x86{
void bgrToTensorCHW(const uint8_t* src,
                       float* output,
                       int width,
                       int height,
                       float* means,
                       float* scales){
    int batch_size = 1;
    int offset = height * width * 3;
    int hw = height * width;
    for(int b_i=0; b_i<batch_size; ++b_i){
        float* b_output_ptr = output + b_i * offset;

        for(int c=0; c<3; ++c){
            float* b_c_output_ptr = b_output_ptr + c*hw;
            for(int p=0; p<hw; ++p){
                b_c_output_ptr[p] = ((float)(src[b_i*offset + p*3 + c]) - means[c]) * scales[c];
            }
        }
    }
}
void bgrToRgbTensorCHW(const uint8_t* src,
                       float* output,
                       int width,
                       int height,
                       float* means,
                       float* scales){
    int batch_size = 1;
    int offset = height * width * 3;
    int hw = height * width;
    for(int b_i=0; b_i<batch_size; ++b_i){
        float* b_output_ptr = output + b_i * offset;

        for(int c=0; c<3; ++c){
            float* b_c_output_ptr = b_output_ptr + c*hw;
            for(int p=0; p<hw; ++p){
                b_c_output_ptr[p] = ((float)(src[b_i*offset + p*3 + (3-c)]) - means[3-c]) * scales[3-c];
            }
        }
    }

}
} // namespace x86
} // namespace math
} // namespace eagleeye