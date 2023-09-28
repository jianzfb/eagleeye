#include "eagleeye/engine/nano/op/facealign_op.h"
#if defined(__ANDROID__) || defined(ANDROID)  
#include "eagleeye/engine/math/arm/interpolate.h"
#else
#include "eagleeye/engine/math/x86/interpolate.h"
#endif
#include "eagleeye/common/EagleeyeLog.h"
#include <fstream>

namespace eagleeye{
namespace dataflow{
FaceAlignOp::FaceAlignOp(int target_h, int target_w, int margin){
    this->m_target_h = target_h;
    this->m_target_w = target_w;
    this->m_margin = margin;
}

FaceAlignOp::FaceAlignOp(const FaceAlignOp& op){
    this->m_target_h = op.m_target_h;
    this->m_target_w = op.m_target_w;
    this->m_margin = op.m_margin;
}

FaceAlignOp::~FaceAlignOp(){

}

int FaceAlignOp::init(std::map<std::string, std::vector<float>> params){
    if(params.find("target_h") != params.end()){
        this->m_target_h = params["target_h"][0];
    }

    if(params.find("target_w") != params.end()){
        this->m_target_w = params["target_w"][0];
    }

    if(params.find("margin") != params.end()){
        this->m_margin = params["margin"][0];
    }    
    return 0;
}

int FaceAlignOp::runOnCpu(const std::vector<Tensor>& input){
    Tensor image = input[0];
    Tensor bbox = input[1];
    Dim image_dim = image.dims();
    int image_h = image_dim[0];
    int image_w = image_dim[1];
    unsigned char* image_ptr = image.cpu<unsigned char>();

    float* bbox_ptr = bbox.cpu<float>();
    int x0 = int(bbox_ptr[0] - this->m_margin + 0.5);
    x0 = std::max(x0, 0);
    int y0 = int(bbox_ptr[1] - this->m_margin + 0.5);
    y0 = std::max(y0, 0);
    int x1 = int(bbox_ptr[2] + this->m_margin + 0.5);
    x1 = std::min(x1, image_w);
    int y1 = int(bbox_ptr[3] + this->m_margin + 0.5);
    y1 = std::min(y1, image_h);

    if(image_dim.size() == 4){
        EAGLEEYE_LOGE("FaceAlignOp dont support batch image");
        return -1;
    }
    if(image_dim.size() == 3){
        // rgb/bgr
        if(image_dim[2] != 3){
            EAGLEEYE_LOGE("FaceAlignOp only support rgb/bgr/gray image");
            return -1;
        }

        if(this->m_outputs[0].numel() != this->m_target_h*this->m_target_w*3){
            Dim out_dim(std::vector<int64_t>{this->m_target_h, this->m_target_w, 3});
            this->m_outputs[0] = Tensor(out_dim.data(),image.type(),image.format(),CPU_BUFFER);
        }

        unsigned char* output_ptr = this->m_outputs[0].cpu<unsigned char>();
#if defined(__ANDROID__) || defined(ANDROID)    
        math::arm::bilinear_rgb_8u_3d_interp(
            image_ptr,
            output_ptr,
            x1-x0,
            y1-y0,
            x0, y0, image_w,
            this->m_target_w,
            this->m_target_h
        );
#else
        math::x86::bilinear_rgb_8u_3d_interp(
            image_ptr,
            output_ptr,
            x1-x0,
            y1-y0,
            x0, y0, image_w,
            this->m_target_w,
            this->m_target_h
        );
#endif
    }
    else{
        // gray 
        if(this->m_outputs[0].numel() != this->m_target_h*this->m_target_w){
            Dim out_dim(std::vector<int64_t>{this->m_target_h, this->m_target_w});
            this->m_outputs[0] = Tensor(out_dim.data(),image.type(),image.format(),CPU_BUFFER);
        }

        unsigned char* output_ptr = this->m_outputs[0].cpu<unsigned char>();

#if defined(__ANDROID__) || defined(ANDROID)    
        math::arm::bilinear_gray_8u_1d_interp(
            image_ptr,
            output_ptr,
            image_w,
            image_h,
            x0,y0,image_w,
            this->m_target_w,
            this->m_target_h
        );
#else
        math::x86::bilinear_gray_8u_1d_interp(
            image_ptr,
            output_ptr,
            image_w,
            image_h,
            x0,y0,image_w,
            this->m_target_w,
            this->m_target_h
        );
#endif
    }

    return 0;
}

int FaceAlignOp::runOnGpu(const std::vector<Tensor>& input){
    return -1;
}
}
}