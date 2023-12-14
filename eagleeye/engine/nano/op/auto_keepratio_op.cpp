#include "eagleeye/engine/nano/op/auto_keepratio_op.h"
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
AutoKeepRatioOp::AutoKeepRatioOp(){
    this->m_target_h = 128;
    this->m_target_w = 128;
    this->m_image_rotate_mode = IMAGE_ROTATE_0;

    m_rk_src_image_ptr = NULL;
    m_rk_resize_image_ptr = NULL;
    m_rk_rotate_image_ptr = NULL;
    m_rk_paste_image_ptr = NULL;

    m_rk_src_handler = 0;
    m_rk_resize_tgt_handler = 0;
    m_rk_rotate_tgt_handler = 0;
    m_rk_paste_tgt_handler = 0;

    m_resize_dst_buf_size = 0;
    m_rotate_dst_buf_size = 0;
}
AutoKeepRatioOp::AutoKeepRatioOp(int target_h, int target_w, ImageRotateMode image_rotate_mode){
    this->m_target_h = target_h;
    this->m_target_w = target_w;
    this->m_image_rotate_mode = image_rotate_mode;

    m_rk_src_image_ptr = NULL;
    m_rk_resize_image_ptr = NULL;
    m_rk_rotate_image_ptr = NULL;
    m_rk_paste_image_ptr = NULL;

    m_rk_src_handler = 0;
    m_rk_resize_tgt_handler = 0;
    m_rk_rotate_tgt_handler = 0;
    m_rk_paste_tgt_handler = 0;

    m_resize_dst_buf_size = 0;
    m_rotate_dst_buf_size = 0;    
}

AutoKeepRatioOp::AutoKeepRatioOp(const AutoKeepRatioOp& op){
    this->m_target_h = op.m_target_h;
    this->m_target_w = op.m_target_w;
    this->m_image_rotate_mode = op.m_image_rotate_mode;
}

AutoKeepRatioOp::~AutoKeepRatioOp(){
#ifdef EAGLEEYE_RKCHIP
    if(m_rk_src_handler > 0){
        releasebuffer_handle(rga_buffer_handle_t(m_rk_src_handler));
    }
    if(m_rk_resize_tgt_handler > 0){
        releasebuffer_handle(rga_buffer_handle_t(m_rk_resize_tgt_handler));
    }
    if(m_rk_rotate_tgt_handler > 0){
        releasebuffer_handle(rga_buffer_handle_t(m_rk_rotate_tgt_handler));
    }
    if(m_rk_paste_tgt_handler){
        releasebuffer_handle(rga_buffer_handle_t(m_rk_paste_tgt_handler));
    }

    if(m_rk_resize_image_ptr != NULL){
        free(m_rk_resize_image_ptr);
    }
    if(m_rk_rotate_image_ptr != NULL){
        free(m_rk_rotate_image_ptr);
    }
#endif
}

int AutoKeepRatioOp::init(std::map<std::string, std::vector<float>> params){
    if(params.find("target_h") != params.end()){
        this->m_target_h = int(params["target_h"][0]);
    }
    if(params.find("target_w") != params.end()){
        this->m_target_w = int(params["target_w"][0]);
    }
    if(params.find("image_rotate_mode") != params.end()){
        this->m_image_rotate_mode = (ImageRotateMode)(int(params["image_rotate_mode"][0]));
    }
    return 0;
}

int AutoKeepRatioOp::runOnCpu(const std::vector<Tensor>& input){
    Tensor image = input[0];
    Dim image_dim = image.dims();
    int image_h = image_dim[0];
    int image_w = image_dim[1];

    if(image_dim.size() != 3){
        EAGLEEYE_LOGE("AutoKeepRatioOp only support image rgb/bgr");
        return -1;
    }
    int image_c = image_dim[2];

    char* image_ptr = const_cast<char*>(input[0].cpu<char>());

    // 图像信息
    if(this->m_outputs[0].numel() != m_target_h * m_target_w * image_c){
        Dim out_dim(std::vector<int64_t>{m_target_h, m_target_w, image_c});
        this->m_outputs[0] = Tensor(out_dim.data(),image.type(),image.format(),CPU_BUFFER);
    }
    // 布局信息
    this->m_outputs[1] = Tensor(std::vector<int64_t>{7}, EAGLEEYE_INT, DataFormat::AUTO, CPU_BUFFER);

#ifdef EAGLEEYE_RKCHIP
    if(image_c == 4){
        // step 1: 缩小到目标大小
        float resize_scale = 1.0f;
        if(m_image_rotate_mode == IMAGE_ROTATE_0 || m_image_rotate_mode == IMAGE_ROTATE_180 || m_image_rotate_mode == IMAGE_FLIP_H){
            resize_scale = fmin(float(m_target_h)/float(image_h), float(m_target_w)/float(image_w));
        }
        else{
            int rotated_image_h = image_w;
            int rotated_image_w = image_h;
            resize_scale = fmin(float(m_target_h)/float(rotated_image_h), float(m_target_w)/float(rotated_image_w));
        }

        int src_width, src_height, src_format;
        int resize_dst_width, resize_dst_height, resize_dst_format;        
        int src_buf_size, resize_dst_buf_size;

        rga_buffer_t src_img, resize_dst_img;
        memset(&src_img, 0, sizeof(src_img));
        memset(&resize_dst_img, 0, sizeof(resize_dst_img));

        src_width = image_w;
        src_height = image_h;
        src_format = RK_FORMAT_RGBA_8888;

        resize_dst_width = image_w * resize_scale;
        resize_dst_height = image_h * resize_scale;

        // 保持四字节对齐
        resize_dst_width = (resize_dst_width / 4) * 4;
        resize_dst_height = (resize_dst_height / 4) * 4;
        resize_dst_format = RK_FORMAT_RGBA_8888;

        src_buf_size = src_width * src_height * get_bpp_from_format(src_format);
        resize_dst_buf_size = resize_dst_width * resize_dst_height * get_bpp_from_format(resize_dst_format);
        if(m_rk_src_image_ptr != (void*)(image_ptr)){
            // 创建 handler
            if(m_rk_src_handler > 0){
                releasebuffer_handle(rga_buffer_handle_t(m_rk_src_handler));
            }
            m_rk_src_handler = importbuffer_virtualaddr(image_ptr, src_buf_size);
            m_rk_src_image_ptr = (void*)(image_ptr);
        }

        if(m_resize_dst_buf_size < resize_dst_buf_size){
            if(m_rk_resize_tgt_handler > 0){
                releasebuffer_handle(rga_buffer_handle_t(m_rk_resize_tgt_handler));
            }
            if(m_rk_resize_image_ptr != NULL){
                free(m_rk_resize_image_ptr);
                m_rk_resize_image_ptr = NULL;
            }

            m_rk_resize_image_ptr = malloc(resize_dst_buf_size);
            m_rk_resize_tgt_handler = importbuffer_virtualaddr(m_rk_resize_image_ptr, resize_dst_buf_size);
            m_resize_dst_buf_size = resize_dst_buf_size;
        }

        src_img = wrapbuffer_handle(rga_buffer_handle_t(m_rk_src_handler), src_width, src_height, src_format);
        resize_dst_img = wrapbuffer_handle(rga_buffer_handle_t(m_rk_resize_tgt_handler), resize_dst_width, resize_dst_height, resize_dst_format);            
        imresize(src_img, resize_dst_img);

        // step 2: 旋转
        rga_buffer_t next_dst_img = resize_dst_img;
        if(m_image_rotate_mode == IMAGE_ROTATE_90 || m_image_rotate_mode == IMAGE_ROTATE_180 || m_image_rotate_mode == IMAGE_ROTATE_270 || m_image_rotate_mode == IMAGE_FLIP_H){
            int rotate_dst_width, rotate_dst_height, rotate_dst_format;
            rotate_dst_format = RK_FORMAT_RGBA_8888;
            if(m_image_rotate_mode == IMAGE_ROTATE_90 || m_image_rotate_mode == IMAGE_ROTATE_270){
                rotate_dst_width = resize_dst_height;
                rotate_dst_height = resize_dst_width;
            }
            else{
                rotate_dst_width = resize_dst_width;
                rotate_dst_height = resize_dst_height;
            }

            int rotate_dst_buf_size = rotate_dst_width * rotate_dst_height * get_bpp_from_format(rotate_dst_format);
            rga_buffer_t rotate_dst_img;
            memset(&rotate_dst_img, 0, sizeof(rotate_dst_img));

            if(m_rotate_dst_buf_size < rotate_dst_buf_size){
                if(m_rk_rotate_tgt_handler > 0){
                    releasebuffer_handle(rga_buffer_handle_t(m_rk_rotate_tgt_handler));
                }
                if(m_rk_rotate_image_ptr != NULL){
                    free(m_rk_rotate_image_ptr);
                    m_rk_rotate_image_ptr = NULL;
                }

                m_rk_rotate_image_ptr = malloc(rotate_dst_buf_size);
                m_rk_rotate_tgt_handler = importbuffer_virtualaddr(m_rk_rotate_image_ptr, rotate_dst_buf_size);
                m_rotate_dst_buf_size = rotate_dst_buf_size;
            }
            rotate_dst_img = wrapbuffer_handle(rga_buffer_handle_t(m_rk_rotate_tgt_handler), rotate_dst_width, rotate_dst_height, rotate_dst_format);
            switch(m_image_rotate_mode){
                case IMAGE_ROTATE_90:
                    imrotate(resize_dst_img, rotate_dst_img, IM_HAL_TRANSFORM_ROT_90);
                    break;
                case IMAGE_ROTATE_180:
                    imrotate(resize_dst_img, rotate_dst_img, IM_HAL_TRANSFORM_ROT_180);
                    break;
                case IMAGE_ROTATE_270:
                    imrotate(resize_dst_img, rotate_dst_img, IM_HAL_TRANSFORM_ROT_270);
                    break;
                case IMAGE_FLIP_H:
                    imrotate(resize_dst_img, rotate_dst_img, IM_HAL_TRANSFORM_FLIP_H);
                    break;
            }

            next_dst_img = rotate_dst_img;   
        }

        // step 3: 复制到目标区域
        int paste_dst_width, paste_dst_height, paste_dst_format;
        paste_dst_width = m_target_w;
        paste_dst_height = m_target_h;
        paste_dst_format = RK_FORMAT_RGBA_8888;

        int paste_dst_buf_size;
        paste_dst_buf_size = paste_dst_width * paste_dst_height * get_bpp_from_format(paste_dst_format);

        rga_buffer_t paste_dst_img;
        memset(&paste_dst_img, 0, sizeof(paste_dst_img));

        if(m_rk_paste_image_ptr != (void*)(this->m_outputs[0].cpu<char>())){
            if(m_rk_paste_tgt_handler > 0){
                releasebuffer_handle(rga_buffer_handle_t(m_rk_paste_tgt_handler));
            }

            m_rk_paste_image_ptr = (void*)(this->m_outputs[0].cpu<char>());
            m_rk_paste_tgt_handler = importbuffer_virtualaddr(m_rk_paste_image_ptr, paste_dst_buf_size);
        }
        paste_dst_img = wrapbuffer_handle(rga_buffer_handle_t(m_rk_paste_tgt_handler), paste_dst_width, paste_dst_height, paste_dst_format);

        im_rect paste_rect;
        if(m_image_rotate_mode == IMAGE_ROTATE_90 || m_image_rotate_mode == IMAGE_ROTATE_270){
            paste_rect.x = (m_target_w - resize_dst_height) / 2;
            paste_rect.y = (m_target_h - resize_dst_width) / 2;
            paste_rect.width = resize_dst_height;
            paste_rect.height = resize_dst_width;
        }
        else{
            paste_rect.x = (m_target_w - resize_dst_width) / 2;
            paste_rect.y = (m_target_h - resize_dst_height) / 2;
            paste_rect.width = resize_dst_width;
            paste_rect.height = resize_dst_height;
        }
        improcess(next_dst_img, paste_dst_img, {}, {}, paste_rect, {}, IM_SYNC);

        int* layout_ptr = this->m_outputs[1].cpu<int>();
        layout_ptr[0] = paste_rect.x;
        layout_ptr[1] = paste_rect.y;
        layout_ptr[2] = paste_rect.width;
        layout_ptr[3] = paste_rect.height;
        if(m_image_rotate_mode == IMAGE_ROTATE_90 || m_image_rotate_mode == IMAGE_ROTATE_270){
            layout_ptr[4] = image_h;
            layout_ptr[5] = image_w;
        }
        else{
            layout_ptr[4] = image_w;
            layout_ptr[5] = image_h;
        }
        layout_ptr[6] = int(m_image_rotate_mode);
        return 0;
    }
#endif

    if(image_c == 4){
        EAGLEEYE_LOGE("AutoKeepRatioOp only support image rgb/bgr");
        return -1;
    }
    return 0;
}

int AutoKeepRatioOp::runOnGpu(const std::vector<Tensor>& input){
    return -1;
}
}
}