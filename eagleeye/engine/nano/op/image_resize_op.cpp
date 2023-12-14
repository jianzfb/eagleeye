#include "eagleeye/engine/nano/op/image_resize_op.h"
#if defined(__ANDROID__) || defined(ANDROID)  
#include "eagleeye/engine/math/arm/interpolate.h"
#else
#include "eagleeye/engine/math/x86/interpolate.h"
#endif

#ifdef EAGLEEYE_RKCHIP
#include "im2d_version.h"
#include "RgaUtils.h"
#include "im2d_buffer.h"
#include "im2d_type.h"
#include "im2d_single.h"
#endif
#include "eagleeye/common/EagleeyeLog.h"

namespace eagleeye{
namespace dataflow{
ResizeOp::ResizeOp(){
    m_out_size = std::vector<int64_t>({32,32});
    m_scale = 0.0f;
    m_op_type = INTERPOLATE_BILINER;
    
    m_rk_tgt_handler = 0;
    m_rk_src_handler = 0;

    m_rk_src_image_ptr = NULL;
    m_rk_tgt_image_ptr = NULL;  
}
ResizeOp::ResizeOp(std::vector<int64_t> out_size, float scale, InterpolateOpType op_type){
    m_out_size = out_size;
    m_scale = scale;
    m_op_type = op_type;

    m_rk_tgt_handler = 0;
    m_rk_src_handler = 0;

    m_rk_src_image_ptr = NULL;
    m_rk_tgt_image_ptr = NULL;    
}

ResizeOp::ResizeOp(const ResizeOp& op)
    :m_out_size(op.m_out_size), m_scale(op.m_scale), m_op_type(op.m_op_type){

}

ResizeOp::~ResizeOp(){
#ifdef EAGLEEYE_RKCHIP
    if(this->m_rk_tgt_handler > 0){
        releasebuffer_handle(rga_buffer_handle_t(m_rk_tgt_handler));
    }
    if(this->m_rk_src_handler > 0){
        releasebuffer_handle(rga_buffer_handle_t(m_rk_src_handler));
    }
#endif
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
    int channel_num = 3;

    // dimx.size() == 2, 3, 4
    if(dimx.size() != 2 && dimx.size() != 3 && dimx.size() != 4){
        EAGLEEYE_LOGE("ResizeOp only support dimx.size == 2,3,4");
        return -1;
    }
    else if(dimx.size() == 3){
        if(dimx[2] != 3 && dimx[2] != 4){
            EAGLEEYE_LOGE("ResizeOp only support HxWx3, HxWx4, NxHxWx3, NxHxWx4");
            return -1;
        }
        channel_num = dimx[2];
    }
    else if(dimx.size() == 4){
        if(dimx[3] != 3 && dimx[3] != 4){
            EAGLEEYE_LOGE("ResizeOp only support HxWx3, HxWx4, NxHxWx3, NxHxWx4");
            return -1;
        }
        channel_num = dimx[3];
    }

    if(x.type() != EAGLEEYE_UCHAR){
        EAGLEEYE_LOGE("ResizeOp only support EAGLEEYE_UCHAR");
        return -1;
    }

    int h_dim_i = 0; int w_dim_i = 0;
    if(dimx.size() == 2){
        h_dim_i = 0; w_dim_i = 1; // H,W
    }
    else if(dimx.size() == 3){
        h_dim_i = 0; w_dim_i = 1; // H,W,3; H,W,4
    }
    else{
        h_dim_i = 1; w_dim_i = 2; // N,H,W,3; N,H,W,4
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
    Dim output_dimx = input[0].dims();
    output_dimx[h_dim_i] = out_height;
    output_dimx[w_dim_i] = out_width;    
    if(this->m_outputs[0].numel() != output_dimx.production()){
        this->m_outputs[0] = Tensor(
            output_dimx.data(),
            x.type(),
            x.format(),
            CPU_BUFFER
        );
    }

    int count = (dimx.size() == 2 || dimx.size() == 3) ? 1 : dimx[0];
    int channels = dimx.size() == 2 ? 1 : channel_num;
    int in_height = dimx[h_dim_i];
    int in_width = dimx[w_dim_i];

    unsigned char* x_ptr = (unsigned char*)x.cpu();
    unsigned char* y_ptr = (unsigned char*)this->m_outputs[0].cpu();
#ifdef EAGLEEYE_RKCHIP
    // 使用RK图像处理芯片处理 (channel 4)
    if(channels != 1 && channels != 3){
        for (int i = 0; i < count; ++i) {
            int src_width, src_height, src_format;
            int dst_width, dst_height, dst_format;
            int src_buf_size, dst_buf_size;

            rga_buffer_t src_img, dst_img;
            memset(&src_img, 0, sizeof(src_img));
            memset(&dst_img, 0, sizeof(dst_img));

            src_width = in_width;
            src_height = in_height;
            src_format = RK_FORMAT_RGBA_8888;
            dst_format = RK_FORMAT_RGBA_8888;

            dst_width = out_width;
            dst_height = out_height;

            src_buf_size = src_width * src_height * get_bpp_from_format(src_format);
            dst_buf_size = dst_width * dst_height * get_bpp_from_format(dst_format);

            if(m_rk_src_image_ptr != (void*)(x_ptr+i*in_width*in_height*channels)){
                // 创建 handler
                if(m_rk_src_handler > 0){
                    releasebuffer_handle(rga_buffer_handle_t(m_rk_src_handler));
                }
                m_rk_src_handler = importbuffer_virtualaddr(x_ptr+i*in_width*in_height*channels, src_buf_size);
                m_rk_src_image_ptr = (void*)(x_ptr+i*in_width*in_height*channels);
            }

            if(m_rk_tgt_image_ptr != (void*)(y_ptr+i*out_width*out_height*channels)){
                // 创建 handler
                if(m_rk_tgt_handler > 0){
                    releasebuffer_handle(rga_buffer_handle_t(m_rk_tgt_handler));
                }
                m_rk_tgt_handler = importbuffer_virtualaddr(y_ptr+i*out_width*out_height*channels, dst_buf_size);
                m_rk_tgt_image_ptr = (void*)(y_ptr+i*out_width*out_height*channels);
            }

            src_img = wrapbuffer_handle(rga_buffer_handle_t(m_rk_src_handler), src_width, src_height, src_format);
            dst_img = wrapbuffer_handle(rga_buffer_handle_t(m_rk_tgt_handler), dst_width, dst_height, dst_format);            
            imresize(src_img, dst_img);
        }
        return 0;
    }
#endif

    // TODO, support channels = 4
    if(channels == 4){
        EAGLEEYE_LOGE("Dont support channels=4 resize.");
        return 0;
    }

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
ResizeWithShapeOp::ResizeWithShapeOp(){
    m_op_type = INTERPOLATE_BILINER;
    m_rk_tgt_handler = 0;
    m_rk_src_handler = 0;

    m_rk_src_image_ptr = NULL;
    m_rk_tgt_image_ptr = NULL;  
}
ResizeWithShapeOp::ResizeWithShapeOp(InterpolateOpType op_type){
    m_op_type = op_type;
    m_rk_tgt_handler = 0;
    m_rk_src_handler = 0;

    m_rk_src_image_ptr = NULL;
    m_rk_tgt_image_ptr = NULL;  
}
ResizeWithShapeOp::ResizeWithShapeOp(const ResizeWithShapeOp& op)
    :m_op_type(op.m_op_type){

}
ResizeWithShapeOp::~ResizeWithShapeOp(){
#ifdef EAGLEEYE_RKCHIP
    if(this->m_rk_tgt_handler > 0){
        releasebuffer_handle(rga_buffer_handle_t(m_rk_tgt_handler));
    }
    if(this->m_rk_src_handler > 0){
        releasebuffer_handle(rga_buffer_handle_t(m_rk_src_handler));
    }
#endif
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
    int channel_num = 3;

    // dimx.size() == 2, 3, 4
    if(dimx.size() != 2 && dimx.size() != 3 && dimx.size() != 4){
        EAGLEEYE_LOGE("ResizeOp only support dimx.size == 2,3,4");
        return -1;
    }
    else if(dimx.size() == 3){
        if(dimx[2] != 3 && dimx[2] != 4){
            EAGLEEYE_LOGE("ResizeOp only support HxWx3, HxWx4, NxHxWx3, NxHxWx4");
            return -1;
        }
        channel_num = dimx[2];
    }
    else if(dimx.size() == 4){
        if(dimx[3] != 3 && dimx[3] != 4){
            EAGLEEYE_LOGE("ResizeOp only support HxWx3, HxWx4, NxHxWx3, NxHxWx4");
            return -1;
        }
        channel_num = dimx[3];
    }
    // other, NxHxW or HxWx3 or HxWx4

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
    int channels = dimx.size() == 2 ? 1 : channel_num;
    int in_height = dimx[h_dim_i];
    int in_width = dimx[w_dim_i];

    unsigned char* x_ptr = (unsigned char*)x.cpu();
    unsigned char* y_ptr = (unsigned char*)this->m_outputs[0].cpu();
#ifdef EAGLEEYE_RKCHIP
    // 使用RK图像处理芯片处理 (channel 4)
    if(channels != 1 && channels != 3){
        for (int i = 0; i < count; ++i) {
            int src_width, src_height, src_format;
            int dst_width, dst_height, dst_format;
            int src_buf_size, dst_buf_size;

            rga_buffer_t src_img, dst_img;
            memset(&src_img, 0, sizeof(src_img));
            memset(&dst_img, 0, sizeof(dst_img));

            src_width = in_width;
            src_height = in_height;
            src_format = RK_FORMAT_RGBA_8888;
            dst_format = RK_FORMAT_RGBA_8888;

            dst_width = out_width;
            dst_height = out_height;

            src_buf_size = src_width * src_height * get_bpp_from_format(src_format);
            dst_buf_size = dst_width * dst_height * get_bpp_from_format(dst_format);

            if(m_rk_src_image_ptr != (void*)(x_ptr+i*in_width*in_height*channels)){
                // 创建 handler
                if(m_rk_src_handler > 0){
                    releasebuffer_handle(rga_buffer_handle_t(m_rk_src_handler));
                }
                m_rk_src_handler = importbuffer_virtualaddr(x_ptr+i*in_width*in_height*channels, src_buf_size);
                m_rk_src_image_ptr = (void*)(x_ptr+i*in_width*in_height*channels);
            }

            if(m_rk_tgt_image_ptr != (void*)(y_ptr+i*out_width*out_height*channels)){
                // 创建 handler
                if(m_rk_tgt_handler > 0){
                    releasebuffer_handle(rga_buffer_handle_t(m_rk_tgt_handler));
                }
                m_rk_tgt_handler = importbuffer_virtualaddr(y_ptr+i*out_width*out_height*channels, dst_buf_size);
                m_rk_tgt_image_ptr = (void*)(y_ptr+i*out_width*out_height*channels);
            }

            src_img = wrapbuffer_handle(m_rk_src_handler, src_width, src_height, src_format);
            dst_img = wrapbuffer_handle(m_rk_tgt_handler, dst_width, dst_height, dst_format); 
            imresize(src_img, dst_img);
        }
        return 0;
    }
#endif

    // TODO, support channels = 4
    if(channels == 4){
        EAGLEEYE_LOGE("Dont support channels=4 resize.");
        return 0;
    }

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
