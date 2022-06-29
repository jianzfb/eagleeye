#include "eagleeye/engine/nano/op/image_resize_op.h"
#if defined(__ANDROID__) || defined(ANDROID)  
#include "eagleeye/engine/math/arm/interpolate.h"
#endif
#include "eagleeye/common/EagleeyeLog.h"

namespace eagleeye{
namespace dataflow{
ImageResizeOp::ImageResizeOp(std::vector<int64_t> out_size, float scale, InterpolateOpType op_type){
    m_out_size = out_size;
    m_scale = scale;
    m_op_type = op_type;
}

ImageResizeOp::ImageResizeOp(const ImageResizeOp& op)
    :m_out_size(op.m_out_size), m_scale(op.m_scale), m_op_type(op.m_op_type){

}

ImageResizeOp::~ImageResizeOp(){

}

int ImageResizeOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}


int ImageResizeOp::runOnCpu(const std::vector<Tensor>& input){
    const Tensor x = input[0];
    Dim dimx = x.dims();
    if(dimx.size() != 4){
        EAGLEEYE_LOGE("InterpolateOp only support NHWC.");
        return -1;
    }
    if(dimx[3] != 3 && dimx[3] != 1){
        EAGLEEYE_LOGE("InterpolateOp only support NHWC (c==3 or c==1).");
        return -1;
    }

    if(x.type() != EAGLEEYE_UCHAR){
        EAGLEEYE_LOGE("InterpolateOp only support EAGLEEYE_UCHAR");
        return -1;
    }

    int out_height = m_out_size[0];
    int out_width = m_out_size[1];
    float scale = m_scale;    
    if(scale > 0.0f){
        // 如果设置scale,则使用scale
        out_height = (int)(x.dims()[2]*scale + 0.5f);
        out_width = (int)(x.dims()[3]*scale + 0.5f);
    }

    // do 
    int count = dimx[0];
    int in_height = dimx[1];
    int in_width = dimx[2];
    int channels = dimx[3];
    if(this->m_outputs[0].numel() != count*out_height*out_width*channels){
        this->m_outputs[0] = Tensor(
            std::vector<int64_t>{count, out_height, out_width, channels},
            x.type(),
            x.format(),
            CPU_BUFFER
        );
    }
    unsigned char* x_ptr = (unsigned char*)x.cpu();
    unsigned char* y_ptr = (unsigned char*)this->m_outputs[0].cpu();
#if defined(__ANDROID__) || defined(ANDROID)      
    if(channels == 3){
#pragma omp parallel for
        for (int i = 0; i < count; ++i) {
            math::arm::bilinear_rgb_8u_3d_interp(
                x_ptr+i*in_width*in_height*3,
                y_ptr+i*out_width*out_height*3,
                in_width,
                in_height,
                0,0,
                in_width,
                out_width,
                out_height
            );
        }
    }
    else{
#pragma omp parallel for
        for (int i = 0; i < count; ++i) {
            math::arm::bilinear_gray_8u_1d_interp(
                x_ptr+i*in_width*in_height,
                y_ptr+i*out_width*out_height,
                in_width,
                in_height,
                0,0,
                in_width,
                out_width,
                out_height
            );
        } 
    }
#endif
    return 0;
}

int ImageResizeOp::runOnGpu(const std::vector<Tensor>& input){
    EAGLEEYE_LOGE("Dont implement (GPU)");
    return -1;
}

/**********************      use shape resize      ***********************************/
ImageResizeWithShapeOp::ImageResizeWithShapeOp(InterpolateOpType op_type){
    m_op_type = op_type;
}
ImageResizeWithShapeOp::ImageResizeWithShapeOp(const ImageResizeWithShapeOp& op)
    :m_op_type(op.m_op_type){

}
ImageResizeWithShapeOp::~ImageResizeWithShapeOp(){

}

int ImageResizeWithShapeOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

int ImageResizeWithShapeOp::runOnCpu(const std::vector<Tensor>& input){
    // 0: x
    const Tensor x = input[0];
    Dim dimx = x.dims();
    if(dimx.size() != 4){
        EAGLEEYE_LOGE("ImageResizeWithShapeOp only support NHWC.");
        return -1;
    }
    if(dimx[3] != 3 && dimx[3] != 1){
        EAGLEEYE_LOGE("ImageResizeWithShapeOp only support NHWC (c==3 or c==1).");
        return -1;
    }

    if(x.type() != EAGLEEYE_UCHAR){
        EAGLEEYE_LOGE("ImageResizeWithShapeOp only support EAGLEEYE_UCHAR");
        return -1;
    }

    // 1: shape
    Tensor y = input[1];
    int* y_ptr = (int*)y.cpu();
    int out_height = y_ptr[0];
    int out_width = y_ptr[1];

    // do 
    int count = dimx[0];
    int in_height = dimx[1];
    int in_width = dimx[2];
    int channels = dimx[3];
    if(this->m_outputs[0].numel() != count*out_height*out_width*channels){
        this->m_outputs[0] = Tensor(
            std::vector<int64_t>{count, out_height, out_width, channels},
            x.type(),
            x.format(),
            CPU_BUFFER
        );
    }
    unsigned char* x_ptr = (unsigned char*)x.cpu();
    unsigned char* z_ptr = (unsigned char*)this->m_outputs[0].cpu();
#if defined(__ANDROID__) || defined(ANDROID)      
    if(channels == 3){
#pragma omp parallel for
        for (int i = 0; i < count; ++i) {
            math::arm::bilinear_rgb_8u_3d_interp(
                x_ptr+i*in_width*in_height*3,
                z_ptr+i*out_width*out_height*3,
                in_width,
                in_height,
                0,0,
                in_width,
                out_width,
                out_height
            );
        }
    }
    else{
#pragma omp parallel for
        for (int i = 0; i < count; ++i) {
            math::arm::bilinear_gray_8u_1d_interp(
                x_ptr+i*in_width*in_height,
                z_ptr+i*out_width*out_height,
                in_width,
                in_height,
                0,0,
                in_width,
                out_width,
                out_height
            );
        } 
    }
#endif
    return 0;
}

int ImageResizeWithShapeOp::runOnGpu(const std::vector<Tensor>& input){
    EAGLEEYE_LOGE("Dont implement (GPU)");
    return -1;
}

} // namespace dataflow
    
} // namespace eagleeye
