#include "eagleeye/engine/nano/op/image_rotate_op.h"
#include <iostream>
#include <fstream>
#ifdef EAGLEEYE_RKCHIP
#include "im2d_version.h"
#include "rk_type.h"
#include "RgaUtils.h"
#include "im2d_buffer.h"
#include "im2d_type.h"
#include "im2d_single.h"
#endif


namespace eagleeye{
namespace dataflow{
ImageRotateOp::~ImageRotateOp(){
#ifdef EAGLEEYE_RKCHIP    
    if (m_src_handler){
        releasebuffer_handle(m_src_handler);
    }
    if (m_tgt_handler){
        releasebuffer_handle(m_tgt_handler);
    }
#endif
}

int ImageRotateOp::init(std::map<std::string, std::vector<float>> params){
    if(params.find("rotate_mode") != params.end()){
        m_rotate_mode = (ImageRotateMode)(int(params["rotate_mode"][0]));
    }
    return 0;
}

int ImageRotateOp::runOnCpu(const std::vector<Tensor>& input){
    // height, width, channels
    int input_h = input[0].dims()[0];
    int input_w = input[0].dims()[1];
    int input_c = input[0].dims()[2];

    int output_h = input_h;
    int output_w = input_w;
    if(m_rotate_mode == IMAGE_ROTATE_90 || m_rotate_mode == IMAGE_ROTATE_270){
        output_h = input_w;
        output_w = input_h;
    }

    if(this->m_outputs[0].numel() != input[0].numel()){
        this->m_outputs[0] = Tensor(
            std::vector<int64_t>{output_h, output_w, input_c},
            input[0].type(),
            input[0].format(),
            CPU_BUFFER
        );
    }
    if(m_rotate_mode == IMAGE_ROTATE_0){
        memcpy(this->m_outputs[0].cpu(), input[0].cpu(), input_h*input_w*input_c);
        return 0;
    }

#ifdef EAGLEEYE_RKCHIP
    // RK加速，仅支持4通道
    if(input_c == 4){
        if(m_src_ptr == NULL || m_src_ptr != input[0].cpu()){
            if(m_src_ptr != NULL){
                releasebuffer_handle(m_src_handler);
            }

            m_src_ptr = input[0].cpu();
            m_src_handler = importbuffer_virtualaddr(m_src_ptr, input_h*input_w*4);
        }

        if(m_tgt_ptr == NULL || m_tgt_ptr != m_outputs[0].cpu()){
            if(m_tgt_ptr != NULL){
                releasebuffer_handle(m_tgt_handler);
            }

            m_tgt_ptr = m_outputs[0].cpu();
            m_tgt_handler = importbuffer_virtualaddr(m_tgt_ptr, output_h*output_w*4);
        }
        
        rga_buffer_t src_img, dst_img;
        memset(&src_img, 0, sizeof(src_img));
        memset(&dst_img, 0, sizeof(dst_img));

        src_img = wrapbuffer_handle(m_src_handler, input_w, input_h, RK_FORMAT_RGBA_8888);
        dst_img = wrapbuffer_handle(m_tgt_handler, output_w, output_h, RK_FORMAT_RGBA_8888);

        switch(m_rotate_mode){
            case IMAGE_ROTATE_90:
                imrotate(src_img, dst_img, IM_HAL_TRANSFORM_ROT_90);
                break;
            case IMAGE_ROTATE_180:
                imrotate(src_img, dst_img, IM_HAL_TRANSFORM_ROT_180);
                break;
            case IMAGE_ROTATE_270:
                imrotate(src_img, dst_img, IM_HAL_TRANSFORM_ROT_270);
                break;
        }
        return 0;
    }
#endif

    if(input_c == 4){
        EAGLEEYE_LOGE("Todo support image rotate.");
        return 0;
    }

    return 0;
}

int ImageRotateOp::runOnGpu(const std::vector<Tensor>& input){
    // height, width, channels
    int input_h = input[0].dims()[0];
    int input_w = input[0].dims()[1];
    int input_c = input[0].dims()[2];

    int output_h = input_h;
    int output_w = input_w;
    if(m_rotate_mode == IMAGE_ROTATE_90 || m_rotate_mode == IMAGE_ROTATE_270){
        output_h = input_w;
        output_w = input_h;
    }

    if(this->m_outputs[0].numel() != input[0].numel()){
        this->m_outputs[0] = Tensor(
            std::vector<int64_t>{output_h, output_w, input_c},
            input[0].type(),
            input[0].format(),
            CPU_BUFFER
        );
    }
    if(m_rotate_mode == IMAGE_ROTATE_0){
        memcpy(this->m_outputs[0].cpu(), input[0].cpu(), input_h*input_w*input_c);
        return 0;
    }

#ifdef EAGLEEYE_RKCHIP
    // RK加速，仅支持4通道
    if(input_c == 4){
        if(m_src_ptr == NULL || m_src_ptr != input[0].cpu()){
            if(m_src_ptr != NULL){
                releasebuffer_handle(m_src_handler);
            }

            m_src_ptr = input[0].cpu();
            m_src_handler = importbuffer_virtualaddr(m_src_ptr, input_h*input_w*4);
        }

        if(m_tgt_ptr == NULL || m_tgt_ptr != m_outputs[0].cpu()){
            if(m_tgt_ptr != NULL){
                releasebuffer_handle(m_tgt_handler);
            }

            m_tgt_ptr = m_outputs[0].cpu();
            m_tgt_handler = importbuffer_virtualaddr(m_tgt_ptr, output_h*output_w*4);
        }
        
        rga_buffer_t src_img, dst_img;
        memset(&src_img, 0, sizeof(src_img));
        memset(&dst_img, 0, sizeof(dst_img));

        src_img = wrapbuffer_handle(m_src_handler, input_w, input_h, RK_FORMAT_RGBA_8888);
        dst_img = wrapbuffer_handle(m_tgt_handler, output_w, output_h, RK_FORMAT_RGBA_8888);

        switch(m_rotate_mode){
            case IMAGE_ROTATE_90:
                imrotate(src_img, dst_img, IM_HAL_TRANSFORM_ROT_90);
                break;
            case IMAGE_ROTATE_180:
                imrotate(src_img, dst_img, IM_HAL_TRANSFORM_ROT_180);
                break;
            case IMAGE_ROTATE_270:
                imrotate(src_img, dst_img, IM_HAL_TRANSFORM_ROT_270);
                break;
        }
        return 0;
    }
#endif

    if(input_c == 4){
        EAGLEEYE_LOGE("Todo support image rotate.");
        return 0;
    }
   
    return 0;
}

///////////////////
int ImageRotateDynamicOp::runOnCpu(const std::vector<Tensor>& input){
    // height, width, channels
    int input_h = input[0].dims()[0];
    int input_w = input[0].dims()[1];
    int input_c = input[0].dims()[2];

    ImageRotateMode rotate_mode = IMAGE_ROTATE_0;
    const int* rotate_info = input[1].cpu<int>();
    rotate_mode = (ImageRotateMode)(rotate_info[0]);

    int output_h = input_h;
    int output_w = input_w;
    if(rotate_mode == IMAGE_ROTATE_90 || rotate_mode == IMAGE_ROTATE_270){
        output_h = input_w;
        output_w = input_h;
    }

    if(this->m_outputs[0].numel() != input[0].numel()){
        this->m_outputs[0] = Tensor(
            std::vector<int64_t>{output_h, output_w, input_c},
            input[0].type(),
            input[0].format(),
            CPU_BUFFER
        );
    }
    if(rotate_mode == IMAGE_ROTATE_0){
        memcpy(this->m_outputs[0].cpu(), input[0].cpu(), input_h*input_w*input_c);
        return 0;
    }

#ifdef EAGLEEYE_RKCHIP
    // RK加速，仅支持4通道
    if(input_c == 4){
        if(m_src_ptr == NULL || m_src_ptr != input[0].cpu()){
            if(m_src_ptr != NULL){
                releasebuffer_handle(m_src_handler);
            }

            m_src_ptr = input[0].cpu();
            m_src_handler = importbuffer_virtualaddr(m_src_ptr, input_h*input_w*4);
        }

        if(m_tgt_ptr == NULL || m_tgt_ptr != m_outputs[0].cpu()){
            if(m_tgt_ptr != NULL){
                releasebuffer_handle(m_tgt_handler);
            }

            m_tgt_ptr = m_outputs[0].cpu();
            m_tgt_handler = importbuffer_virtualaddr(m_tgt_ptr, output_h*output_w*4);
        }
        
        rga_buffer_t src_img, dst_img;
        memset(&src_img, 0, sizeof(src_img));
        memset(&dst_img, 0, sizeof(dst_img));

        src_img = wrapbuffer_handle(m_src_handler, input_w, input_h, RK_FORMAT_RGBA_8888);
        dst_img = wrapbuffer_handle(m_tgt_handler, output_w, output_h, RK_FORMAT_RGBA_8888);

        switch(rotate_mode){
            case IMAGE_ROTATE_90:
                imrotate(src_img, dst_img, IM_HAL_TRANSFORM_ROT_90);
                break;
            case IMAGE_ROTATE_180:
                imrotate(src_img, dst_img, IM_HAL_TRANSFORM_ROT_180);
                break;
            case IMAGE_ROTATE_270:
                imrotate(src_img, dst_img, IM_HAL_TRANSFORM_ROT_270);
                break;
        }
        return 0;
    }
#endif

    if(input_c == 4){
        EAGLEEYE_LOGE("Todo support image rotate.");
        return 0;
    }
   
    return 0;
    return 0;
}

int ImageRotateDynamicOp::runOnGpu(const std::vector<Tensor>& input){
    return 0;
}

}    
}