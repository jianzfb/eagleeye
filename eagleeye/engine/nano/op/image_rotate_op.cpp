#include "eagleeye/engine/nano/op/image_rotate_op.h"
#include "eagleeye/common/EagleeyeLog.h"
#include <fstream>
#ifdef EAGLEEYE_RKCHIP
#include "im2d_version.h"
#include "RgaUtils.h"
#include "im2d_buffer.h"
#include "im2d_type.h"
#include "im2d_single.h"
#endif

namespace eagleeye{
namespace dataflow{
ImageRotateOp::ImageRotateOp(){
    m_image_rotate_mode = IMAGE_ROTATE_0;

    m_rk_tgt_handler = 0;
    m_rk_src_handler = 0;

    m_rk_src_image_ptr = NULL;
    m_rk_tgt_image_ptr = NULL;
}

ImageRotateOp::ImageRotateOp(ImageRotateMode mode){
    m_image_rotate_mode = mode;

    m_rk_tgt_handler = 0;
    m_rk_src_handler = 0;

    m_rk_src_image_ptr = NULL;
    m_rk_tgt_image_ptr = NULL;
}

ImageRotateOp::ImageRotateOp(const ImageRotateOp& op){
     this->m_image_rotate_mode = op.m_image_rotate_mode;
}

ImageRotateOp::~ImageRotateOp(){
#ifdef EAGLEEYE_RKCHIP
    if(this->m_rk_tgt_handler > 0){
        releasebuffer_handle(rga_buffer_handle_t(m_rk_tgt_handler));
    }
    if(this->m_rk_src_handler > 0){
        releasebuffer_handle(rga_buffer_handle_t(m_rk_src_handler));
    }
#endif
}

int ImageRotateOp::init(std::map<std::string, std::vector<float>> params){
    if(params.find("mode") != params.end()){
        this->m_image_rotate_mode = (ImageRotateMode)(params["mode"][0]);
    }
    return 0;
}

int ImageRotateOp::runOnCpu(const std::vector<Tensor>& input){
    Dim image_dim = input[0].dims();
    int image_h = image_dim[0];
    int image_w = image_dim[1];
    int image_c = image_dim[2];
    if(this->m_image_rotate_mode == IMAGE_ROTATE_0){
        if(this->m_outputs[0].numel() != image_dim[0]*image_dim[1]*image_dim[2]){
            this->m_outputs[0] = Tensor(std::vector<int64_t>{image_h, image_w, image_c}, input[0].type(), input[0].format(), CPU_BUFFER);
        }

        memcpy(this->m_outputs[0].cpu(), input[0].cpu(), image_h*image_w*image_c);
        return 0;
    }

    if(this->m_image_rotate_mode == IMAGE_ROTATE_90 || this->m_image_rotate_mode == IMAGE_ROTATE_270){
        if(this->m_outputs[0].numel() != image_dim[0]*image_dim[1]*image_dim[2]){
            this->m_outputs[0] = Tensor(
                std::vector<int64_t>{image_w, image_h, image_c}, input[0].type(), input[0].format(), CPU_BUFFER);
        }
    }
    else{
        if(this->m_outputs[0].numel() != image_dim[0]*image_dim[1]*image_dim[2]){
            this->m_outputs[0] = Tensor(
                std::vector<int64_t>{image_h, image_w, image_c}, input[0].type(), input[0].format(), CPU_BUFFER);
        }
    }

    if(image_c == 4){
        // RGBA/BGRA
#ifdef EAGLEEYE_RKCHIP
        if(m_rk_src_image_ptr != (void*)(const_cast<char*>(input[0].cpu<char>()))){
            // 创建 handler
            if(m_rk_src_handler > 0){
                releasebuffer_handle(rga_buffer_handle_t(m_rk_src_handler));
            }
            m_rk_src_handler = importbuffer_virtualaddr(const_cast<char*>(input[0].cpu<char>()), image_h * image_w * get_bpp_from_format(RK_FORMAT_RGBA_8888));
            m_rk_src_image_ptr = (void*)(const_cast<char*>(input[0].cpu<char>()));
        }

        if(m_rk_tgt_image_ptr != (void*)(this->m_outputs[0].cpu<char>())){
            // 创建 handler
            if(m_rk_tgt_handler > 0){
                releasebuffer_handle(rga_buffer_handle_t(m_rk_tgt_handler));
            }
            m_rk_tgt_handler = importbuffer_virtualaddr(this->m_outputs[0].cpu<char>(), image_h * image_w * get_bpp_from_format(RK_FORMAT_RGBA_8888));
            m_rk_tgt_image_ptr = (void*)(this->m_outputs[0].cpu<char>());
        }
        rga_buffer_t src_img, dst_img;
        // rga_buffer_handle_t src_handle, dst_handle;
        int src_width, src_height, src_format;
        int dst_width, dst_height, dst_format;
        src_width = image_w;
        src_height = image_h;
        if(this->m_image_rotate_mode == IMAGE_ROTATE_90 || this->m_image_rotate_mode == IMAGE_ROTATE_270){
            dst_width = src_height;
            dst_height = src_width;
        }
        else{
            dst_width = src_width;
            dst_height = src_height;
        }

        memset(&src_img, 0, sizeof(src_img));
        memset(&dst_img, 0, sizeof(dst_img));
        src_format = RK_FORMAT_RGBA_8888;
        dst_format = RK_FORMAT_RGBA_8888;

        int src_buf_size = image_h * image_w * get_bpp_from_format(src_format);
        int dst_buf_size = image_h * image_w * get_bpp_from_format(dst_format);
        src_img = wrapbuffer_handle(rga_buffer_handle_t(m_rk_src_handler), src_width, src_height, src_format);
        dst_img = wrapbuffer_handle(rga_buffer_handle_t(m_rk_tgt_handler), dst_width, dst_height, dst_format);

        // IM_HAL_TRANSFORM_ROT_90, IM_HAL_TRANSFORM_ROT_180, IM_HAL_TRANSFORM_ROT_270, IM_HAL_TRANSFORM_FLIP_H
        switch(m_image_rotate_mode){
            case IMAGE_ROTATE_90:
                imrotate(src_img, dst_img, IM_HAL_TRANSFORM_ROT_90);
                break;
            case IMAGE_ROTATE_180:
                imrotate(src_img, dst_img, IM_HAL_TRANSFORM_ROT_180);
                break;
            case IMAGE_ROTATE_270:
                imrotate(src_img, dst_img, IM_HAL_TRANSFORM_ROT_270);
                break;
            case IMAGE_FLIP_H:
                imrotate(src_img, dst_img, IM_HAL_TRANSFORM_FLIP_H);
                break;
        }
#else
        // CPU
        // TODO, support
#endif
    }
    else{
        // RGB/BGR
#ifdef EAGLEEYE_RKCHIP
        // RK_FORMAT_RGB_888， RK_FORMAT_BGR_888
        if(m_rk_src_image_ptr != (void*)(const_cast<char*>(input[0].cpu<char>()))){
            // 创建 handler
            if(m_rk_src_handler > 0){
                releasebuffer_handle(rga_buffer_handle_t(m_rk_src_handler));
            }
            m_rk_src_handler = importbuffer_virtualaddr(const_cast<char*>(input[0].cpu<char>()), image_h * image_w * get_bpp_from_format(RK_FORMAT_RGB_888));
            m_rk_src_image_ptr = (void*)(const_cast<char*>(input[0].cpu<char>()));
        }

        if(m_rk_tgt_image_ptr != (void*)(this->m_outputs[0].cpu<char>())){
            // 创建 handler
            if(m_rk_tgt_handler > 0){
                releasebuffer_handle(rga_buffer_handle_t(m_rk_tgt_handler));
            }
            m_rk_tgt_handler = importbuffer_virtualaddr(this->m_outputs[0].cpu<char>(), image_h * image_w * get_bpp_from_format(RK_FORMAT_RGB_888));
            m_rk_tgt_image_ptr = (void*)(this->m_outputs[0].cpu<char>());
        }

        rga_buffer_t src_img, dst_img;
        rga_buffer_handle_t src_handle, dst_handle;
        int src_width, src_height, src_format;
        int dst_width, dst_height, dst_format;
        src_width = image_w;
        src_height = image_h;
        if(this->m_image_rotate_mode == IMAGE_ROTATE_90 || this->m_image_rotate_mode == IMAGE_ROTATE_270){
            dst_width = src_height;
            dst_height = src_width;
        }
        else{
            dst_width = src_width;
            dst_height = src_height;
        }

        memset(&src_img, 0, sizeof(src_img));
        memset(&dst_img, 0, sizeof(dst_img));
        src_format = RK_FORMAT_RGB_888;
        dst_format = RK_FORMAT_RGB_888;

        int src_buf_size = image_h * image_w * get_bpp_from_format(src_format);
        int dst_buf_size = image_h * image_w * get_bpp_from_format(dst_format);
        src_img = wrapbuffer_handle(rga_buffer_handle_t(m_rk_src_handler), src_width, src_height, src_format);
        dst_img = wrapbuffer_handle(rga_buffer_handle_t(m_rk_tgt_handler), dst_width, dst_height, dst_format);

        // IM_HAL_TRANSFORM_ROT_90, IM_HAL_TRANSFORM_ROT_180, IM_HAL_TRANSFORM_ROT_270, IM_HAL_TRANSFORM_FLIP_H
        switch(m_image_rotate_mode){
            case IMAGE_ROTATE_90:
                imrotate(src_img, dst_img, IM_HAL_TRANSFORM_ROT_90);
                break;
            case IMAGE_ROTATE_180:
                imrotate(src_img, dst_img, IM_HAL_TRANSFORM_ROT_180);
                break;
            case IMAGE_ROTATE_270:
                imrotate(src_img, dst_img, IM_HAL_TRANSFORM_ROT_270);
                break;
            case IMAGE_FLIP_H:
                imrotate(src_img, dst_img, IM_HAL_TRANSFORM_FLIP_H);
                break;
        }    
#else
        // CPU
        // TODO, support
#endif
    }
    return 0;
}

int ImageRotateOp::runOnGpu(const std::vector<Tensor>& input){
    return 0;    
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
ImageRotateDynamicOp::ImageRotateDynamicOp(){
    m_rk_tgt_handler = 0;
    m_rk_src_handler = 0;

    m_rk_src_image_ptr = NULL;
    m_rk_tgt_image_ptr = NULL;
}

ImageRotateDynamicOp::ImageRotateDynamicOp(const ImageRotateDynamicOp& op){
}

ImageRotateDynamicOp::~ImageRotateDynamicOp(){
#ifdef EAGLEEYE_RKCHIP
    if(this->m_rk_tgt_handler > 0){
        releasebuffer_handle(rga_buffer_handle_t(m_rk_tgt_handler));
    }
    if(this->m_rk_src_handler > 0){
        releasebuffer_handle(rga_buffer_handle_t(m_rk_src_handler));
    }
#endif
}

int ImageRotateDynamicOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

int ImageRotateDynamicOp::runOnCpu(const std::vector<Tensor>& input){
    Dim image_dim = input[0].dims();
    int image_h = image_dim[0];
    int image_w = image_dim[1];
    int image_c = image_dim[2];

    const int* rotate_info = input[1].cpu<int>();
    ImageRotateMode image_rotate = (ImageRotateMode)rotate_info[0];
    if(image_rotate == IMAGE_ROTATE_0){
        if(this->m_outputs[0].numel() != image_dim[0]*image_dim[1]*image_dim[2]){
            this->m_outputs[0] = Tensor(std::vector<int64_t>{image_h, image_w, image_c}, input[0].type(), input[0].format(), CPU_BUFFER);
        }

        memcpy(this->m_outputs[0].cpu(), input[0].cpu(), image_h*image_w*image_c);
        return 0;
    }

    if(image_rotate == IMAGE_ROTATE_90 || image_rotate == IMAGE_ROTATE_270){
        if(this->m_outputs[0].numel() != image_dim[0]*image_dim[1]*image_dim[2]){
            this->m_outputs[0] = Tensor(
                std::vector<int64_t>{image_w, image_h, image_c}, input[0].type(), input[0].format(), CPU_BUFFER);
        }
    }
    else{
        if(this->m_outputs[0].numel() != image_dim[0]*image_dim[1]*image_dim[2]){
            this->m_outputs[0] = Tensor(
                std::vector<int64_t>{image_h, image_w, image_c}, input[0].type(), input[0].format(), CPU_BUFFER);
        }
    }

    if(image_c == 4){
        // RGBA/BGRA
#ifdef EAGLEEYE_RKCHIP
        if(m_rk_src_image_ptr != (void*)(const_cast<char*>(input[0].cpu<char>()))){
            // 创建 handler
            if(m_rk_src_handler > 0){
                releasebuffer_handle(rga_buffer_handle_t(m_rk_src_handler));
            }
            m_rk_src_handler = importbuffer_virtualaddr(const_cast<char*>(input[0].cpu<char>()), image_h * image_w * get_bpp_from_format(RK_FORMAT_RGBA_8888));
            m_rk_src_image_ptr = (void*)(const_cast<char*>(input[0].cpu<char>()));
        }

        if(m_rk_tgt_image_ptr != (void*)(this->m_outputs[0].cpu<char>())){
            // 创建 handler
            if(m_rk_tgt_handler > 0){
                releasebuffer_handle(rga_buffer_handle_t(m_rk_tgt_handler));
            }
            m_rk_tgt_handler = importbuffer_virtualaddr(this->m_outputs[0].cpu<char>(), image_h * image_w * get_bpp_from_format(RK_FORMAT_RGBA_8888));
            m_rk_tgt_image_ptr = (void*)(this->m_outputs[0].cpu<char>());
        }

        rga_buffer_t src_img, dst_img;
        // rga_buffer_handle_t src_handle, dst_handle;
        int src_width, src_height, src_format;
        int dst_width, dst_height, dst_format;
        src_width = image_w;
        src_height = image_h;
        if(image_rotate == IMAGE_ROTATE_90 || image_rotate == IMAGE_ROTATE_270){
            dst_width = src_height;
            dst_height = src_width;
        }
        else{
            dst_width = src_width;
            dst_height = src_height;
        }

        memset(&src_img, 0, sizeof(src_img));
        memset(&dst_img, 0, sizeof(dst_img));
        src_format = RK_FORMAT_RGBA_8888;
        dst_format = RK_FORMAT_RGBA_8888;

        int src_buf_size = image_h * image_w * get_bpp_from_format(src_format);
        int dst_buf_size = image_h * image_w * get_bpp_from_format(dst_format);
        src_img = wrapbuffer_handle(rga_buffer_handle_t(m_rk_src_handler), src_width, src_height, src_format);
        dst_img = wrapbuffer_handle(rga_buffer_handle_t(m_rk_tgt_handler), dst_width, dst_height, dst_format);

        // IM_HAL_TRANSFORM_ROT_90, IM_HAL_TRANSFORM_ROT_180, IM_HAL_TRANSFORM_ROT_270, IM_HAL_TRANSFORM_FLIP_H
        switch(image_rotate){
            case IMAGE_ROTATE_90:
                imrotate(src_img, dst_img, IM_HAL_TRANSFORM_ROT_90);
                break;
            case IMAGE_ROTATE_180:
                imrotate(src_img, dst_img, IM_HAL_TRANSFORM_ROT_180);
                break;
            case IMAGE_ROTATE_270:
                imrotate(src_img, dst_img, IM_HAL_TRANSFORM_ROT_270);
                break;
            case IMAGE_FLIP_H:
                imrotate(src_img, dst_img, IM_HAL_TRANSFORM_FLIP_H);
                break;
        }
#else
        // CPU
        // TODO, support
#endif
    }
    else{
        // RGB/BGR
#ifdef EAGLEEYE_RKCHIP
        // RK_FORMAT_RGB_888， RK_FORMAT_BGR_888
        if(m_rk_src_image_ptr != (void*)(const_cast<char*>(input[0].cpu<char>()))){
            // 创建 handler
            if(m_rk_src_handler > 0){
                releasebuffer_handle(rga_buffer_handle_t(m_rk_src_handler));
            }
            m_rk_src_handler = importbuffer_virtualaddr(const_cast<char*>(input[0].cpu<char>()), image_h * image_w * get_bpp_from_format(RK_FORMAT_RGB_888));
            m_rk_src_image_ptr = (void*)(const_cast<char*>(input[0].cpu<char>()));
        }

        if(m_rk_tgt_image_ptr != (void*)(this->m_outputs[0].cpu<char>())){
            // 创建 handler
            if(m_rk_tgt_handler > 0){
                releasebuffer_handle(rga_buffer_handle_t(m_rk_tgt_handler));
            }
            m_rk_tgt_handler = importbuffer_virtualaddr(this->m_outputs[0].cpu<char>(), image_h * image_w * get_bpp_from_format(RK_FORMAT_RGB_888));
            m_rk_tgt_image_ptr = (void*)(this->m_outputs[0].cpu<char>());
        }

        rga_buffer_t src_img, dst_img;
        rga_buffer_handle_t src_handle, dst_handle;
        int src_width, src_height, src_format;
        int dst_width, dst_height, dst_format;
        src_width = image_w;
        src_height = image_h;
        if(image_rotate == IMAGE_ROTATE_90 || image_rotate == IMAGE_ROTATE_270){
            dst_width = src_height;
            dst_height = src_width;
        }
        else{
            dst_width = src_width;
            dst_height = src_height;
        }

        memset(&src_img, 0, sizeof(src_img));
        memset(&dst_img, 0, sizeof(dst_img));
        src_format = RK_FORMAT_RGB_888;
        dst_format = RK_FORMAT_RGB_888;

        int src_buf_size = image_h * image_w * get_bpp_from_format(src_format);
        int dst_buf_size = image_h * image_w * get_bpp_from_format(dst_format);
        src_img = wrapbuffer_handle(rga_buffer_handle_t(m_rk_src_handler), src_width, src_height, src_format);
        dst_img = wrapbuffer_handle(rga_buffer_handle_t(m_rk_tgt_handler), dst_width, dst_height, dst_format);

        // IM_HAL_TRANSFORM_ROT_90, IM_HAL_TRANSFORM_ROT_180, IM_HAL_TRANSFORM_ROT_270, IM_HAL_TRANSFORM_FLIP_H
        switch(image_rotate){
            case IMAGE_ROTATE_90:
                imrotate(src_img, dst_img, IM_HAL_TRANSFORM_ROT_90);
                break;
            case IMAGE_ROTATE_180:
                imrotate(src_img, dst_img, IM_HAL_TRANSFORM_ROT_180);
                break;
            case IMAGE_ROTATE_270:
                imrotate(src_img, dst_img, IM_HAL_TRANSFORM_ROT_270);
                break;
            case IMAGE_FLIP_H:
                imrotate(src_img, dst_img, IM_HAL_TRANSFORM_FLIP_H);
                break;
        }    
#else
        // CPU
        // TODO, support
#endif
    }
    return 0;
}

int ImageRotateDynamicOp::runOnGpu(const std::vector<Tensor>& input){
    return 0;    
}
}
}