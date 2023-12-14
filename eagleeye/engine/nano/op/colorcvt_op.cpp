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

void ColorCvtOp::convertRGB2BGR(const Tensor src, Tensor& tgt){
    Dim image_dim = src.dims();
    int image_h = image_dim[0];
    int image_w = image_dim[1];

    const unsigned char* src_ptr = src.cpu<unsigned char>();
    unsigned char* tgt_ptr = tgt.cpu<unsigned char>();
    for(int i=0; i<image_h; ++i){
        const unsigned char* row_src_ptr = src_ptr + i*image_w*3;
        unsigned char* row_tgt_ptr = tgt_ptr + i*image_w*3;
        for(int j=0; j<image_w; ++j){
            int offset = j*3;
            row_tgt_ptr[offset]     = row_src_ptr[offset+2];
            row_tgt_ptr[offset+1]   = row_src_ptr[offset+1];
            row_tgt_ptr[offset+2]   = row_src_ptr[offset];
        }
    }
}
void ColorCvtOp::convertRGBA2BGR(const Tensor src, Tensor& tgt){
    Dim image_dim = src.dims();
    int image_h = image_dim[0];
    int image_w = image_dim[1];

    const unsigned char* src_ptr = src.cpu<unsigned char>();
    unsigned char* tgt_ptr = tgt.cpu<unsigned char>();
    for (int i = 0; i < image_h; i++) {
        for (int j = 0; j < image_w; j++) {
            *tgt_ptr++ = src_ptr[2];  // r
            *tgt_ptr++ = src_ptr[1];  // g
            *tgt_ptr++ = src_ptr[0];  // b
            // *dst++ = src[4];//a
            src_ptr += 4;
        }
    }
}

void ColorCvtOp::convertRGBA2RGB(const Tensor src, Tensor& tgt){
    Dim image_dim = src.dims();
    int image_h = image_dim[0];
    int image_w = image_dim[1];

    const unsigned char* src_ptr = src.cpu<unsigned char>();
    unsigned char* tgt_ptr = tgt.cpu<unsigned char>();
    for (int i = 0; i < image_h; i++) {
        for (int j = 0; j < image_w; j++) {
            *tgt_ptr++ = src_ptr[0];  // r
            *tgt_ptr++ = src_ptr[1];  // g
            *tgt_ptr++ = src_ptr[2];  // b
            // *dst++ = src[4];//a
            src_ptr += 4;
        }
    }
}

void ColorCvtOp::convertRGB2RGBA(const Tensor src, Tensor& tgt){
    Dim image_dim = src.dims();
    int image_h = image_dim[0];
    int image_w = image_dim[1];

    const unsigned char* src_ptr = src.cpu<unsigned char>();
    unsigned char* tgt_ptr = tgt.cpu<unsigned char>();
    for (int i = 0; i < image_h; i++) {
        for (int j = 0; j < image_w; j++) {
            *tgt_ptr++ = src_ptr[0];  // r
            *tgt_ptr++ = src_ptr[1];  // g
            *tgt_ptr++ = src_ptr[2];  // b
            *tgt_ptr++ = 0;           // a
            src_ptr += 3;
        }
    }
}

void ColorCvtOp::convertBGR2RGBA(const Tensor src, Tensor& tgt){
    Dim image_dim = src.dims();
    int image_h = image_dim[0];
    int image_w = image_dim[1];

    const unsigned char* src_ptr = src.cpu<unsigned char>();
    unsigned char* tgt_ptr = tgt.cpu<unsigned char>();
    for (int i = 0; i < image_h; i++) {
        for (int j = 0; j < image_w; j++) {
            *tgt_ptr++ = src_ptr[2];  // r
            *tgt_ptr++ = src_ptr[1];  // g
            *tgt_ptr++ = src_ptr[0];  // b
            *tgt_ptr++ = 0;           // a
            src_ptr += 3;
        }
    } 
}

int ColorCvtOp::runOnCpu(const std::vector<Tensor>& input){
    Dim in_dim = input[0].dims();
    std::vector<int64_t> out_size = {in_dim[0], in_dim[1], in_dim[2]};
    if(this->m_mode == COLOR_RGB2BGR || this->m_mode == COLOR_RGBA2BGR || this->m_mode == COLOR_RGBA2RGB){
        out_size[2] = 3;
    }
    else{
        out_size[2] = 4;
    }

    if(this->m_outputs[0].numel() != out_size[0]*out_size[1]*out_size[2]){
        this->m_outputs[0] = Tensor(out_size,input[0].type(),input[0].format(),CPU_BUFFER);
    }
    switch(this->m_mode){
        case COLOR_RGB2BGR:
            this->convertRGB2BGR(input[0], this->m_outputs[0]);
            break;
        case COLOR_RGBA2BGR:
            this->convertRGBA2BGR(input[0], this->m_outputs[0]);
            break;
        case COLOR_RGBA2RGB:
            this->convertRGBA2RGB(input[0], this->m_outputs[0]);
            break;
        case COLOR_RGB2RGBA:
            this->convertRGB2RGBA(input[0], this->m_outputs[0]);
            break;
        case COLOR_BGR2RGBA:
            this->convertBGR2RGBA(input[0], this->m_outputs[0]);
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