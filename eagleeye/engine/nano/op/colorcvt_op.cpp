#include "eagleeye/engine/nano/op/colorcvt_op.h"
#include "eagleeye/common/EagleeyeLog.h"
#include <fstream>

namespace eagleeye{
namespace dataflow{
ColorCvtOp::ColorCvtOp(){
    this->m_mode = COLOR_RGB2BGR;
}
ColorCvtOp::ColorCvtOp(ColorCvtMode mode){
    this->m_mode = mode;
}

ColorCvtOp::ColorCvtOp(const ColorCvtOp& op){
    this->m_mode = op.m_mode;
}

ColorCvtOp::~ColorCvtOp(){

}

int ColorCvtOp::init(std::map<std::string, std::vector<float>> params){
    if(params.find("mode") != params.end()){
        this->m_mode = ColorCvtMode((int)(params["mode"][0]));
    }
    return 0;
}

void ColorCvtOp::convertRGB2BGR(Tensor src, Tensor& tgt){
    Dim image_dim = src.dims();
    int image_h = image_dim[0];
    int image_w = image_dim[1];

    unsigned char* src_ptr = src.cpu<unsigned char>();
    unsigned char* tgt_ptr = tgt.cpu<unsigned char>();
    for(int i=0; i<image_h; ++i){
        unsigned char* row_src_ptr = src_ptr + i*image_w*3;
        unsigned char* row_tgt_ptr = tgt_ptr + i*image_w*3;
        for(int j=0; j<image_w; ++j){
            int offset = j*3;
            row_tgt_ptr[offset] = row_src_ptr[offset+2];
            row_tgt_ptr[offset+1] = row_src_ptr[offset+1];
            row_tgt_ptr[offset+2] = row_src_ptr[offset];
        }
    }
}

int ColorCvtOp::runOnCpu(const std::vector<Tensor>& input){
    if(this->m_outputs[0].numel() != input[0].numel()){
        Dim out_dim = input[0].dims();
        this->m_outputs[0] = Tensor(out_dim.data(),input[0].type(),input[0].format(),CPU_BUFFER);
    }

    switch(this->m_mode){
        case COLOR_RGB2BGR:
            this->convertRGB2BGR(input[0], this->m_outputs[0]);
            break;
        default:
            break;
    }

    return 0;
}

int ColorCvtOp::runOnGpu(const std::vector<Tensor>& input){
    return -1;
}
}
}