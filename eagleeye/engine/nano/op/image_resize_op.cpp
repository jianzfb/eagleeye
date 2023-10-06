#include "eagleeye/engine/nano/op/image_resize_op.h"
#if defined(__ANDROID__) || defined(ANDROID)  
#include "eagleeye/engine/math/arm/interpolate.h"
#else
#include "eagleeye/engine/math/x86/interpolate.h"
#endif
#include "eagleeye/common/EagleeyeLog.h"

namespace eagleeye{
namespace dataflow{
ResizeOp::ResizeOp(std::vector<int64_t> out_size, float scale, InterpolateOpType op_type){
    m_out_size = out_size;
    m_scale = scale;
    m_op_type = op_type;
}

ResizeOp::ResizeOp(const ResizeOp& op)
    :m_out_size(op.m_out_size), m_scale(op.m_scale), m_op_type(op.m_op_type){

}

ResizeOp::~ResizeOp(){
}

int ResizeOp::init(std::map<std::string, std::vector<float>> params){
    if(params.size() == 0){
        return 0;
    }

    // vector, scalar
    this->m_scale = 0.0f;
    if(params.find("out_size") != params.end()){
        this->m_out_size.resize(2);
        this->m_out_size[0] = int64_t(params["out_size"][0]);       // width
        this->m_out_size[1] = int64_t(params["out_size"][1]);       // height
    }
    else{
        this->m_scale = float(params["scale"][0]);                  // 各向同尺度
    }

    this->m_op_type = INTERPOLATE_BILINER;
    if(params.find("op_type") != params.end()){
        this->m_op_type = InterpolateOpType(params["op_type"][0]);
    }
    return 0;
}


int ResizeOp::runOnCpu(const std::vector<Tensor>& input){
    const Tensor x = input[0];
    Dim dimx = x.dims();

    // dimx.size() == 2, 3, 4
    if(dimx.size() != 2 && dimx.size() != 3 && dimx.size() != 4){
        EAGLEEYE_LOGE("ResizeOp only support dimx.size == 2,3,4");
        return -1;
    }
    else if(dimx.size() == 3){
        if(dimx[2] != 3){
            EAGLEEYE_LOGE("ResizeOp only support HxWx3");
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
    float scale = this->m_scale;
    if(scale > 0.0f){
        // 如果设置scale,则使用scale
        out_height = (int)(dimx[h_dim_i]*scale + 0.5f);
        out_width = (int)(dimx[w_dim_i]*scale + 0.5f);
    }

    // do 
    if(this->m_outputs[0].numel() != input[0].numel()){
        Dim new_dimx = input[0].dims();
        new_dimx[h_dim_i] = out_height;
        new_dimx[w_dim_i] = out_width;
        this->m_outputs[0] = Tensor(
            new_dimx.data(),
            x.type(),
            x.format(),
            CPU_BUFFER
        );
    }

    int count = (dimx.size() == 2 || dimx.size() == 3) ? 1 : dimx[0];
    int channels = dimx.size() == 2 ? 1 : 3;
    int in_height = dimx[h_dim_i];
    int in_width = dimx[w_dim_i];

    unsigned char* x_ptr = (unsigned char*)x.cpu();
    unsigned char* y_ptr = (unsigned char*)this->m_outputs[0].cpu();
#if defined(__ANDROID__) || defined(ANDROID)      
    if(channels == 3){
        // 三通道图
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
        // 灰度图
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
#else
    if(channels == 3){
        // 三通道图
#pragma omp parallel for
        for (int i = 0; i < count; ++i) {
            math::x86::bilinear_rgb_8u_3d_interp(
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
        // 灰度图
#pragma omp parallel for
        for (int i = 0; i < count; ++i) {
            math::x86::bilinear_gray_8u_1d_interp(
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

int ResizeOp::runOnGpu(const std::vector<Tensor>& input){
    EAGLEEYE_LOGE("Dont implement (GPU)");
    return -1;
}

/**********************      use shape resize      ***********************************/
ResizeWithShapeOp::ResizeWithShapeOp(InterpolateOpType op_type){
    m_op_type = op_type;
}
ResizeWithShapeOp::ResizeWithShapeOp(const ResizeWithShapeOp& op)
    :m_op_type(op.m_op_type){

}
ResizeWithShapeOp::~ResizeWithShapeOp(){

}

int ResizeWithShapeOp::init(std::map<std::string, std::vector<float>> params){
    if(params.size() == 0){
        return 0;
    }

    this->m_op_type = InterpolateOpType(params["op_type"][0]);    
    return 0;
}

int ResizeWithShapeOp::runOnCpu(const std::vector<Tensor>& input){
    // 0: x
    const Tensor x = input[0];
    Dim dimx = x.dims();

    // dimx.size() == 2, 3, 4
    if(dimx.size() != 2 && dimx.size() != 3 && dimx.size() != 4){
        EAGLEEYE_LOGE("ResizeOp only support dimx.size == 2,3,4");
        return -1;
    }
    else if(dimx.size() == 3){
        if(dimx[2] != 3){
            EAGLEEYE_LOGE("ResizeOp only support HxWx3");
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

    // 1: shape
    int* out_shape_ptr = (int*)input[1].cpu();
    int out_height = out_shape_ptr[0];
    int out_width = out_shape_ptr[1];

    // do 
    if(this->m_outputs[0].numel() != input[0].numel()){
        Dim new_dimx = input[0].dims();
        new_dimx[h_dim_i] = out_height;
        new_dimx[w_dim_i] = out_width;
        this->m_outputs[0] = Tensor(
            new_dimx.data(),
            x.type(),
            x.format(),
            CPU_BUFFER
        );
    }

    int count = (dimx.size() == 2 || dimx.size() == 3) ? 1 : dimx[0];
    int channels = dimx.size() == 2 ? 1 : 3;
    int in_height = dimx[h_dim_i];
    int in_width = dimx[w_dim_i];

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
#else
    if(channels == 3){
#pragma omp parallel for
        for (int i = 0; i < count; ++i) {
            math::x86::bilinear_rgb_8u_3d_interp(
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
            math::x86::bilinear_gray_8u_1d_interp(
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

int ResizeWithShapeOp::runOnGpu(const std::vector<Tensor>& input){
    EAGLEEYE_LOGE("Dont implement (GPU)");
    return -1;
}

} // namespace dataflow
    
} // namespace eagleeye
