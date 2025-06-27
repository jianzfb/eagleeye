#include "eagleeye/engine/nano/op/interpolate_op.h"
#include "eagleeye/common/EagleeyeLog.h"
#if defined (__ARM_NEON) || defined (__ARM_NEON__)
#include "eagleeye/engine/math/arm/interpolate.h"
#endif


namespace eagleeye{
namespace dataflow{
InterpolateOp::InterpolateOp(std::vector<int64_t> out_size, float scale, bool align_corner, InterpolateOpType op_type){
    m_out_size = out_size;
    m_scale = scale;
    m_op_type = op_type;
    m_with_align = true;
    m_align_mode = (int)(align_corner);
    if(m_out_size.size() == 0){
        m_out_size = {0,0};
    }
}   

InterpolateOp::InterpolateOp(const InterpolateOp& op)
    :m_out_size(op.m_out_size),
        m_scale(op.m_scale),
        m_op_type(op.m_op_type),
        m_with_align(true),
        m_align_mode(op.m_align_mode){

}

InterpolateOp::~InterpolateOp(){

} 

int InterpolateOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

int InterpolateOp::runOnCpu(const std::vector<Tensor>& input){
    // 0: x
    const Tensor x = input[0];
    if(x.dims().size() != 4){
        EAGLEEYE_LOGE("InterpolateOp only support NCHW.");
        return -1;
    }
    if(x.type() != EAGLEEYE_FLOAT){
        EAGLEEYE_LOGE("InterpolateOp only support EAGLEEYE_FLOAT");
        return -1;
    }

    int out_height = m_out_size[0];
    int out_width = m_out_size[1];
    float scale = m_scale;    
    if(scale > 0.0f){
        out_height = x.dims()[2]*scale;
        out_width = x.dims()[3]*scale;
    }

    float height_scale = scale;
    float width_scale = scale;
    if (out_width > 0 && out_height > 0) {
        height_scale = static_cast<float>((float)(out_height) / x.dims()[2]);
        width_scale = static_cast<float>((float)(out_width) / x.dims()[3]);
    }
    if(height_scale <= 0.0f || width_scale <= 0.0f){
        EAGLEEYE_LOGE("InterpolateOp dont set correct scale or outsize.");
        return -1;
    }

    int num_cout = x.dims()[0];
    int c_cout = x.dims()[1];
    if(this->m_outputs[0].numel() != c_cout*num_cout*out_height*out_width){
        this->m_outputs[0] = Tensor(
            std::vector<int64_t>{num_cout,c_cout,out_height,out_width},
            x.type(),
            x.format(),
            CPU_BUFFER
        );
    }
    Tensor out = this->m_outputs[0];
    float* dout = (float*)out.cpu();
    float* din = (float*)x.cpu();

    int in_h = x.dims()[2];
    int in_w = x.dims()[3];
    int out_num = out.dims()[0];
    int out_c = out.dims()[1];
    int count = out_num * out_c;
    int out_h = out.dims()[2];
    int out_w = out.dims()[3];
    int spatial_in = in_h * in_w;
    int spatial_out = out_h * out_w;

#if defined (__ARM_NEON) || defined (__ARM_NEON__)
    if (m_op_type == INTERPOLATE_BILINER) {
// #pragma omp parallel for
        for (int i = 0; i < count; ++i) {
        math::arm::bilinear_interp(din + spatial_in * i,
                        in_w,
                        in_h,
                        dout + spatial_out * i,
                        out_w,
                        out_h,
                        1.f / width_scale,
                        1.f / height_scale,
                        m_with_align,
                        m_align_mode);
        }
    } else if (m_op_type == INTERPOLATE_NEAREST) {
// #pragma omp parallel for
        for (int i = 0; i < count; ++i) {
        math::arm::nearest_interp(din + spatial_in * i,
                        in_w,
                        in_h,
                        dout + spatial_out * i,
                        out_w,
                        out_h,
                        1.f / width_scale,
                        1.f / height_scale,
                        m_with_align);
        }
    }
#endif
    return 0;
}

int InterpolateOp::runOnGpu(const std::vector<Tensor>& input){
    EAGLEEYE_LOGE("Dont implement (GPU)");
    return -1;
}

/**************************   use shape resize    **********************************/
InterpolateWithShapeOp::InterpolateWithShapeOp(bool align_corner,InterpolateOpType op_type){
    m_op_type = op_type;
    m_with_align = true;
    m_align_mode = (int)(align_corner);
}
InterpolateWithShapeOp::InterpolateWithShapeOp(const InterpolateWithShapeOp& op)
    :m_op_type(op.m_op_type),
     m_align_mode(op.m_align_mode),
     m_with_align(true){

}
InterpolateWithShapeOp::~InterpolateWithShapeOp(){

}

int InterpolateWithShapeOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

int InterpolateWithShapeOp::runOnCpu(const std::vector<Tensor>& input){
    // 0: x
    const Tensor x = input[0];
    Dim dimx = x.dims();
    if(dimx.size() != 4){
        EAGLEEYE_LOGE("InterpolateWithShapeOp only support NCHW.");
        return -1;
    }

    if(x.type() != EAGLEEYE_FLOAT){
        EAGLEEYE_LOGE("InterpolateWithShapeOp only support EAGLEEYE_FLOAT");
        return -1;
    }

    // 1: shape
    Tensor y = input[1];
    int* y_ptr = (int*)y.cpu();
    int out_height = y_ptr[0]; int out_width = y_ptr[1];
    float height_scale = static_cast<float>((float)(out_height) / dimx[2]);
    float width_scale = static_cast<float>((float)(out_width) / dimx[3]);


    int num_cout = dimx[0];
    int c_cout =dimx[1];
    if(this->m_outputs[0].numel() != c_cout*num_cout*out_height*out_width){
        this->m_outputs[0] = Tensor(
            std::vector<int64_t>{num_cout,c_cout,out_height,out_width},
            x.type(),
            x.format(),
            CPU_BUFFER
        );
    }
    Tensor out = this->m_outputs[0];
    float* dout = (float*)out.cpu();
    float* din = (float*)x.cpu();

    int in_h = x.dims()[2];
    int in_w = x.dims()[3];
    int out_num = out.dims()[0];
    int out_c = out.dims()[1];
    int count = out_num * out_c;
    int out_h = out.dims()[2];
    int out_w = out.dims()[3];
    int spatial_in = in_h * in_w;
    int spatial_out = out_h * out_w;
#if defined (__ARM_NEON) || defined (__ARM_NEON__)
    if (m_op_type == INTERPOLATE_BILINER) {
// #pragma omp parallel for
        for (int i = 0; i < count; ++i) {
        math::arm::bilinear_interp(din + spatial_in * i,
                        in_w,
                        in_h,
                        dout + spatial_out * i,
                        out_w,
                        out_h,
                        1.f / width_scale,
                        1.f / height_scale,
                        m_with_align,
                        m_align_mode);
        }
    } else if (m_op_type == INTERPOLATE_NEAREST) {
// #pragma omp parallel for
        for (int i = 0; i < count; ++i) {
        math::arm::nearest_interp(din + spatial_in * i,
                        in_w,
                        in_h,
                        dout + spatial_out * i,
                        out_w,
                        out_h,
                        1.f / width_scale,
                        1.f / height_scale,
                        m_with_align);
        }
    }
#endif
    return 0;
}

int InterpolateWithShapeOp::runOnGpu(const std::vector<Tensor>& input){
    EAGLEEYE_LOGE("Dont implement (GPU)");
    return -1;
}
}    
}