#include "eagleeye/engine/nano/op/keepratio_op.h"
#include "eagleeye/common/EagleeyeLog.h"
#include <fstream>

namespace eagleeye{
namespace dataflow{
KeepRatioOp::KeepRatioOp(){
    this->m_ratio = 1.0f;
}
KeepRatioOp::KeepRatioOp(float ratio){
    this->m_ratio = ratio;
}

KeepRatioOp::KeepRatioOp(const KeepRatioOp& op){
    this->m_ratio = op.m_ratio;
}

KeepRatioOp::~KeepRatioOp(){

}

int KeepRatioOp::init(std::map<std::string, std::vector<float>> params){
    if(params.find("ratio") != params.end()){
        this->m_ratio = params["ratio"][0];
    }
    return 0;
}

int KeepRatioOp::runOnCpu(const std::vector<Tensor>& input){
    Tensor image = input[0];
    Dim image_dim = image.dims();
    int image_h = image_dim[0];
    int image_w = image_dim[1];

    if(image_dim.size() != 3 && image_dim.size() != 2){
        EAGLEEYE_LOGE("KeepRatioOp only support image rgb/bgr/gray");
        return -1;
    }

    // width/height == ratio
    int after_image_h = image_h;
    int after_image_w = image_w;
    float image_ratio = float(image_w)/float(image_h);
    if(image_ratio < this->m_ratio){
        after_image_w = this->m_ratio * image_h;
    }
    else{
        after_image_h = image_w / this->m_ratio;
    }

    int offset_x = (after_image_w - image_w)/2;
    int offset_y = (after_image_h - image_h)/2;

    // 保存布局信息
    this->m_outputs[1] = Tensor(std::vector<int64_t>{4}, EAGLEEYE_INT, DataFormat::AUTO, CPU_BUFFER);
    int* layout_ptr = this->m_outputs[1].cpu<int>();
    layout_ptr[0] = offset_x;
    layout_ptr[1] = offset_y;
    layout_ptr[2] = image_w;
    layout_ptr[3] = image_h;

    // 保存图像数据信息
    if(image_dim.size() == 3){
        // rgb/bgr
        if(this->m_outputs[0].numel() != after_image_h*after_image_w*3){
            Dim out_dim(std::vector<int64_t>{after_image_h, after_image_w, 3});
            this->m_outputs[0] = Tensor(out_dim.data(),image.type(),image.format(),CPU_BUFFER);
        }

        unsigned char* image_ptr = image.cpu<unsigned char>();
        unsigned char* output_ptr = this->m_outputs[0].cpu<unsigned char>();
        for(int i=0; i<image_h; ++i){
            unsigned char* row_output_ptr = output_ptr + (i+offset_y)*after_image_w*3 + offset_x*3;
            unsigned char* row_image_ptr = image_ptr + i*image_w*3;
            memcpy(row_output_ptr,row_image_ptr, image_w*3);
        }        
    }
    else{
        // gray
        if(this->m_outputs[0].numel() != after_image_h*after_image_w){
            Dim out_dim(std::vector<int64_t>{after_image_h, after_image_w});
            this->m_outputs[0] = Tensor(out_dim.data(),image.type(),image.format(),CPU_BUFFER);
        }

        unsigned char* image_ptr = image.cpu<unsigned char>();
        unsigned char* output_ptr = this->m_outputs[0].cpu<unsigned char>();
        for(int i=0; i<image_h; ++i){
            unsigned char* row_output_ptr = output_ptr + (i+offset_y)*after_image_w + offset_x;
            unsigned char* row_image_ptr = image_ptr + i*image_w;
            memcpy(row_output_ptr,row_image_ptr, image_w);
        }
    }

    return 0;
}

int KeepRatioOp::runOnGpu(const std::vector<Tensor>& input){
    return -1;
}
}
}