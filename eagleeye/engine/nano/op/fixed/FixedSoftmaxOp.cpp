#include "eagleeye/engine/nano/op/FixedSoftmaxOp.h"
#include "eagleeye/common/EagleeyeLog.h"
#if defined (__ARM_NEON) || defined (__ARM_NEON__)
#include <arm_neon.h>
#include "eagleeye/engine/nano/util/neon_mathfunc.h"
#endif
namespace eagleeye
{
namespace nano
{
FixedSoftmaxOp::FixedSoftmaxOp(int input_data_num, int output_data_num, std::string op_name)
             :FixedCNNOp(input_data_num, output_data_num, FIXED_SOFTMAX, op_name){
    
}

FixedSoftmaxOp::~FixedSoftmaxOp(){

}

void FixedSoftmaxOp::foward_on_cpu(std::vector<Tensor<FixedType>>& output, std::vector<Tensor<FixedType>>& input){
    // format BxCxHxW
    int input_data_size = input[0].size();
    int output_data_size = output[0].size();

    FixedType* net_input = (FixedType*)input[0].cpu();
    FixedType* net_output = (FixedType*)output[0].cpu();

    int B = input[0].dim(0);    // B == 1
    int C = input[0].dim(1);
    int H = input[0].dim(2);
    int W = input[0].dim(3);
    int total_size = B*C*H*W;
    int HW = H*W;

	float *f_data = (float*)malloc(sizeof(float)*total_size);

    // dequantize
    for(int b=0; b<B; ++b){
        for(int c=0; c<C; ++c){
            for (int k = 0; k < HW; k++){
                f_data[k] = net_input[k] * this->m_input_multi_rate[c];
            }
        }
    }
    
    // exp ~ (1+(\frac{x}{m}))^{m}
#if defined (__ARM_NEON) || defined (__ARM_NEON__)
#pragma omp parallel for num_threads(4)
    for (int q=0; q<C; q++)
    {
        float* ptr = f_data + q*H*W;

        for (int i=0; i<H; i++)
        {
            float32x4_t _max = vdupq_n_f32(-FLT_MAX);
            for (int j=0; j<W; j++)
            {
                float32x4_t _p = vld1q_f32(ptr + j * 4);
                _max = vmaxq_f32(_max, _p);
            }

            float32x4_t _sum = vdupq_n_f32(0.f);
            for (int j=0; j<W; j++)
            {
                float32x4_t _p = vld1q_f32(ptr + j * 4);
                _p = exp_ps(vsubq_f32(_p, _max));
                vst1q_f32(ptr + j * 4, _p);
                _sum = vaddq_f32(_sum, _p);
            }

            for (int j=0; j<W; j++)
            {
                float32x4_t _p = vld1q_f32(ptr + j * 4);
#if __aarch64__
                _p = vdivq_f32(_p, _sum);
#else
                _p = div_ps(_p, _sum);
#endif
                vst1q_f32(ptr + j * 4, _p);
            }

            ptr += W * 4;
        }
    }

    // quantize;
    for(int c=0; c<C; ++c){
        float* ptr = f_data + c*H*W;

        for (int k = 0; k < HW; k++){
            net_output[k] = FixedType(ptr[k] / this->m_output_multi_rate[c] * QUANTIZER_DATA_NORM_VALUE);
        }

        ptr += C*HW;
        net_output += C*HW;
    }

#endif

    EAGLEEYE_SAFEFREE(f_data);
}

void FixedSoftmaxOp::foward_on_gpu(std::vector<Tensor<FixedType>>& output, std::vector<Tensor<FixedType>>& input){

}

void FixedSoftmaxOp::foward_on_dsp(std::vector<Tensor<FixedType>>& output, std::vector<Tensor<FixedType>>& input){

}


bool FixedSoftmaxOp::init(char *buf, int in_size){
    char* ptr = buf;
    int c = *(int*)ptr;
    ptr = ptr + sizeof(int);
    this->m_input_multi_rate = (float*)malloc(sizeof(float)* c);
    memcpy(this->m_input_multi_rate, ptr, sizeof(float)*c);
    ptr = ptr + sizeof(float)*c;

    c = *(int*)ptr;
    ptr = ptr + sizeof(int);
    this->m_output_multi_rate = (float*)malloc(sizeof(float)*c);
    memcpy(this->m_output_multi_rate, ptr, sizeof(float)*c);
    ptr = ptr + sizeof(float)*c;

    if(in_size != (ptr - buf)){
        EAGLEEYE_LOGE("%s init fail", this->m_op_name.c_str());
        return false;
    }
    
    return true;
}

int FixedSoftmaxOp::setShape(std::vector<int64_t> shape){
    return 0;
}
} // namespace nano
    
} // namespace eagleeye
