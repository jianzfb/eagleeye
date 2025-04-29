#include "eagleeye/engine/nano/op/keepratio_op.h"
#include "eagleeye/common/EagleeyeLog.h"
#if defined (__ARM_NEON) || defined (__ARM_NEON__)  
#include "eagleeye/engine/math/arm/interpolate.h"
#else
#include "eagleeye/engine/math/x86/interpolate.h"
#endif
#include <fstream>
#include <opencv2/opencv.hpp>
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
    if(params.find("aspect_ratio") != params.end()){
        this->m_ratio = params["aspect_ratio"][0];
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
    this->m_outputs[1] = Tensor(std::vector<int64_t>{7}, EAGLEEYE_INT, DataFormat::AUTO, CPU_BUFFER);
    int* layout_ptr = this->m_outputs[1].cpu<int>();
    layout_ptr[0] = offset_x;
    layout_ptr[1] = offset_y;
    layout_ptr[2] = image_w;
    layout_ptr[3] = image_h;
    layout_ptr[4] = image_w;
    layout_ptr[5] = image_h;
    layout_ptr[6] = 0;

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


ResizeKeepRatioOp::ResizeKeepRatioOp(){
    this->m_out_size = std::vector<int64_t>{64,64};
    this->m_op_type = INTERPOLATE_BILINER;
}

ResizeKeepRatioOp::ResizeKeepRatioOp(std::vector<int64_t> out_size, InterpolateOpType op_type){
    this->m_out_size = out_size;
    this->m_op_type = op_type;
}

ResizeKeepRatioOp::ResizeKeepRatioOp(const ResizeKeepRatioOp& op){
    this->m_out_size = op.m_out_size;
    this->m_op_type = op.m_op_type;
}

ResizeKeepRatioOp::~ResizeKeepRatioOp(){
}

int ResizeKeepRatioOp::init(std::map<std::string, std::vector<float>> params){
    if(params.size() == 0){
        return 0;
    }
    if(params.find("out_size") != params.end()){
        this->m_out_size.resize(2);
        this->m_out_size[0] = int64_t(params["out_size"][0]);       // width
        this->m_out_size[1] = int64_t(params["out_size"][1]);       // height
    }
    this->m_op_type = INTERPOLATE_BILINER;
    if(params.find("op_type") != params.end()){
        this->m_op_type = InterpolateOpType(params["op_type"][0]);
    }
    return 0;
}

int ResizeKeepRatioOp::runOnCpu(const std::vector<Tensor>& input){
    const Tensor x = input[0];
    Dim dimx = x.dims();

    // dimx.size() == 2, 3, 4
    if(dimx.size() != 2 && dimx.size() != 3 && dimx.size() != 4){
        EAGLEEYE_LOGE("ResizeOp only support dimx.size == 2,3,4");
        return -1;
    }
    else if(dimx.size() == 3){
        if(dimx[2] != 3 && dimx[2] != 4){
            EAGLEEYE_LOGE("ResizeOp only support HxWx3, HxWx4");
            return -1;
        }
    }
    else if(dimx.size() == 4){
        if(dimx[3] != 3){
            EAGLEEYE_LOGE("ResizeOp only support NxHxWx3");
            return -1;
        }
    }
    // other, NxHxW or HxWx3

    if(x.type() != EAGLEEYE_UCHAR){
        EAGLEEYE_LOGE("ResizeOp only support EAGLEEYE_UCHAR");
        return -1;
    }

    int h_dim_i = 0; int w_dim_i = 0;
    if(dimx.size() == 2){
        h_dim_i = 0; w_dim_i = 1; // H,W
    }
    else if(dimx.size() == 3){
        h_dim_i = 0; w_dim_i = 1; // H,W,3
    }
    else{
        h_dim_i = 1; w_dim_i = 2; // N,H,W,3
    }

    int out_width = this->m_out_size[0];
    int out_height = this->m_out_size[1];

    float im_ratio = float(dimx[h_dim_i]) / float(dimx[w_dim_i]);
    float target_ratio = float(this->m_out_size[1]) / float(this->m_out_size[0]);
    int new_height = 0;
    int new_width = 0;
    if(im_ratio > target_ratio){
        new_height = this->m_out_size[1];
        new_width = int((float)new_height / im_ratio);
    }
    else{
        new_width = this->m_out_size[0];
        new_height = int((float)new_width * im_ratio);
    }

    Dim out_dimx = input[0].dims();
    out_dimx[h_dim_i] = out_height;
    out_dimx[w_dim_i] = out_width;
    if(this->m_outputs[0].numel() != out_dimx.production()){
        // 需要重新申请输出内存
        this->m_outputs[0] = Tensor(
            out_dimx.data(),
            x.type(),
            x.format(),
            CPU_BUFFER
        );
    }

    if(this->m_outputs[1].empty()){
        this->m_outputs[1] = Tensor(
            std::vector<int64_t>{1},
            EAGLEEYE_FLOAT,
            DataFormat::NONE,
            CPU_BUFFER
        );
    }
    float* scale_ptr = this->m_outputs[1].cpu<float>();
    scale_ptr[0] = float(dimx[h_dim_i]) / float(new_height);

    int count = (dimx.size() == 2 || dimx.size() == 3) ? 1 : dimx[0];
    int channels = dimx.size() == 2 ? 1 : dimx[dimx.size()-1];
    int in_height = dimx[h_dim_i];
    int in_width = dimx[w_dim_i];

    if(channels == 4){
        EAGLEEYE_LOGE("Todo support channel = 4, image resize.");
        return 0;
    }

    unsigned char* x_ptr = (unsigned char*)x.cpu();
    unsigned char* y_ptr = (unsigned char*)this->m_outputs[0].cpu();
#if defined (__ARM_NEON) || defined (__ARM_NEON__)      
    if(channels == 3){
        // 三通道图
// #pragma omp parallel for
        for (int i = 0; i < count; ++i) {
            math::arm::bilinear_rgb_8u_3d_interp(
                x_ptr+i*in_width*in_height*3,
                y_ptr+i*out_width*out_height*3,
                in_width,
                in_height,
                0,0,
                in_width,
                new_width,
                new_height,
                out_width
            );
        }
    }
    else{
        // 灰度图
// #pragma omp parallel for
        for (int i = 0; i < count; ++i) {
            math::arm::bilinear_gray_8u_1d_interp(
                x_ptr+i*in_width*in_height,
                y_ptr+i*out_width*out_height,
                in_width,
                in_height,
                0,0,
                in_width,
                new_width,
                new_height,
                out_width
            );
        }
    }
#else
    if(channels == 3){
        // 三通道图
// #pragma omp parallel for
        for (int i = 0; i < count; ++i) {
            math::x86::bilinear_rgb_8u_3d_interp(
                x_ptr+i*in_width*in_height*3,
                y_ptr+i*out_width*out_height*3,
                in_width,
                in_height,
                0,0,
                in_width,
                new_width,
                new_height,
                out_width
            );
        }
    }
    else{
        // 灰度图
// #pragma omp parallel for
        for (int i = 0; i < count; ++i) {
            math::x86::bilinear_gray_8u_1d_interp(
                x_ptr+i*in_width*in_height,
                y_ptr+i*out_width*out_height,
                in_width,
                in_height,
                0,0,
                in_width,
                new_width,
                new_height,
                out_width
            );
        } 
    }
#endif

    return 0;
}

int ResizeKeepRatioOp::runOnGpu(const std::vector<Tensor>& input){
    EAGLEEYE_LOGE("Dont implement (GPU)");
    return -1;
}
}
}