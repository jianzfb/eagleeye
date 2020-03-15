#include "eagleeye/engine/nano/op/FixedDepthwiseConvOp.h"
#include "eagleeye/engine/nano/util/conv_pool_2d_util.h"
#include "eagleeye/common/EagleeyeLog.h"

namespace eagleeye
{
namespace nano
{
FixedDepthwiseConvOp::FixedDepthwiseConvOp(int input_data_num, int output_data_num, std::string op_name)
    :FixedCNNOp(input_data_num, output_data_num, FIXED_DEPTHWISE_CONV, op_name){

}   

FixedDepthwiseConvOp::~FixedDepthwiseConvOp(){

}

void FixedDepthwiseConvOp::forward_on_cpu(std::vector<Tensor<FixedType>>& output, std::vector<Tensor<FixedType>>& input){
    int input_data_size = input[0].size();
    // int output_data_size = output[0].size();

    FixedType* net_input = (FixedType*)input[0].cpu();
    // FixedType* net_output = (FixedType*)output[0].cpu();

    int C = input[0].dim(1);    
    int H = input[0].dim(2);
    int W = input[0].dim(3);

    std::vector<index_t> output_shape(4);
    std::vector<int> paddings(2);

    Tensor<FixedType> filter = this->m_filter;
    if(input.size() >= 2){
        filter = input[1];
    }

    if (this->m_paddings.empty()) {
      CalcPaddingAndOutputSize(input[0].shape().data(),
                               DataFormat::NCHW,
                               filter.shape().data(),
                               DataFormat::OIHW,
                               this->m_dilations.data(),
                               this->m_strides.data(),
                               this->m_padding,
                               output_shape.data(),
                               paddings.data());
    } else {
      paddings = this->m_paddings;
      CalcOutputSize(input[0].shape().data(),
                     DataFormat::NCHW,
                     filter.shape().data(),
                     DataFormat::OIHW,
                     this->m_paddings.data(),
                     this->m_dilations.data(),
                     this->m_strides.data(),
                     RoundType::FLOOR,
                     output_shape.data());
    }

    EAGLEEYE_CHECK(output[0].dim(0) == input[0].dim(0), "Input/Output batch size mismatch");
    // EAGLEEYE_CHECK(filter.dim(2) == input[0].dim(3), filter.dim(2), " != ", input[0].dim(3));

    index_t out_channels = output_shape[3];
    index_t stride_h = this->m_strides[0];
    index_t stride_w = this->m_strides[1];
    index_t dilation_h = this->m_dilations[0];
    index_t dilation_w = this->m_dilations[1];
    int pad_top = paddings[0] >> 1;
    int pad_left = paddings[1] >> 1;

    // 
    FixedOpType* op_input_data = (FixedOpType*)malloc(sizeof(FixedOpType)*input_data_size);

    // 1.step quantization
    quant_sym_16b_8b(net_input, op_input_data, H, W, C);

    // 2.step compute
    float output_multiplier = input[0].scale() * filter.scale() / output[0].scale();
    const int pad_hw[2] = {pad_top, pad_left};

    
}

void FixedDepthwiseConvOp::forward_on_dsp(std::vector<Tensor<FixedType>>& output, std::vector<Tensor<FixedType>>& input){

}

void FixedDepthwiseConvOp::forward_on_gpu(std::vector<Tensor<FixedType>>& output, std::vector<Tensor<FixedType>>& input){

}

bool FixedDepthwiseConvOp::init(char* buf, int in_size){
    return true;
}

int FixedDepthwiseConvOp::setShape(std::vector<int64_t> shape){
    return 0;
}
} // namespace nano
    
} // namespace eagleeye
