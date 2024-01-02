#include "eagleeye/engine/nano/op/auto_keepratio_op.h"
#include "eagleeye/common/EagleeyeRGBRotate.h"
#if defined(__ANDROID__) || defined(ANDROID)  
#include "eagleeye/engine/math/arm/interpolate.h"
#else
#include "eagleeye/engine/math/x86/interpolate.h"
#endif

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
AutoKeepRatioOp::AutoKeepRatioOp(){
    m_target_w = 64;
    m_target_h = 64;
    m_rotate = IMAGE_ROTATE_0;

    m_src_handler = 0;
    m_tgt_handler = 0;
    m_src_ptr = NULL;
    m_tgt_ptr = NULL;

    m_resize_handler = 0;
    m_resize_buf_size = 0;
    m_resize_ptr = NULL;

    m_rotate_handler = 0;
    m_rotate_buf_size = 0;
    m_rotate_ptr = NULL;
}

AutoKeepRatioOp::AutoKeepRatioOp(int width, int height, ImageRotateMode rotate){
    m_target_w = width;
    m_target_h = height;
    m_rotate = rotate;

    m_src_handler = 0;
    m_tgt_handler = 0;
    m_src_ptr = NULL;
    m_tgt_ptr = NULL;

    m_resize_handler = 0;
    m_resize_buf_size = 0;
    m_resize_ptr = NULL;

    m_rotate_handler = 0;
    m_rotate_buf_size = 0;
    m_rotate_ptr = NULL;
}

AutoKeepRatioOp::~AutoKeepRatioOp(){
#ifdef EAGLEEYE_RKCHIP
    if (m_src_handler){
        releasebuffer_handle(m_src_handler);
    }
    if (m_tgt_handler){
        releasebuffer_handle(m_tgt_handler);
    }

    if (m_resize_handler){
        releasebuffer_handle(m_resize_handler);
    }
    if(m_resize_ptr != NULL){
        free(m_resize_ptr);
    }

    if (m_rotate_handler){
        releasebuffer_handle(m_rotate_handler);
    }
    if(m_rotate_ptr != NULL){
        free(m_rotate_ptr);
    }    
#endif
}

int AutoKeepRatioOp::init(std::map<std::string, std::vector<float>> params){
    if(params.find("target_w") != params.end()){
        m_target_w = int(params["target_w"][0]);
    }
    if(params.find("target_h") != params.end()){
        m_target_h = int(params["target_h"][0]);
    }

    if(params.find("target_w") != params.end()){
        m_rotate = (ImageRotateMode)(int(params["rotate"][0]));
    }
    return 0;
}

int AutoKeepRatioOp::runOnCpu(const std::vector<Tensor>& input){
    // HxWx3, HxWx4
    int image_h = input[0].dims()[0];
    int image_w = input[0].dims()[1];
    int image_c = input[0].dims()[2];    
    if(image_c != 3 && image_c != 4){
        EAGLEEYE_LOGE("AutoKeepRatioOp only support channel 3,4");
        return -1;
    }

    if(this->m_outputs[0].numel() != m_target_w * m_target_h * image_c){
        this->m_outputs[0] = Tensor(
            std::vector<int64_t>{m_target_h, m_target_w, image_c},
            input[0].type(),
            input[0].format(),
            CPU_BUFFER
        );
    }

    if(this->m_outputs[1].numel() != 7){
        this->m_outputs[1] = Tensor(
            std::vector<int64_t>{7},
            EAGLEEYE_INT32,
            DataFormat::AUTO,
            CPU_BUFFER
        );
    }

    // 预先计算resize后的大小
    float resized_scale = 1.0f;
    int resized_width = 0;
    int resized_height = 0;
    if(m_rotate == IMAGE_ROTATE_0 || m_rotate == IMAGE_ROTATE_180){
        resized_scale = std::min(float(m_target_w)/float(image_w), float(m_target_h)/float(image_h));
    }
    else{
        resized_scale = std::min(float(m_target_w)/float(image_h), float(m_target_h)/float(image_w));
    }

    // 保证4对齐
    resized_width = (int)(resized_scale * image_w) / 4 * 4;
    resized_height = (int)(resized_scale * image_h) / 4 * 4;

    // 预先计算填充区域
    int fill_x = 0;
    int fill_y = 0;
    int fill_w = 0;
    int fill_h = 0;
    if(m_rotate == IMAGE_ROTATE_0 || m_rotate == IMAGE_ROTATE_180){
        fill_x = (m_target_w - resized_width)/2;
        fill_y = (m_target_h - resized_height)/2;

        fill_w = resized_width;
        fill_h = resized_height;
    }
    else{
        fill_x = (m_target_w - resized_height)/2;
        fill_y = (m_target_h - resized_width)/2;

        fill_w = resized_height;
        fill_h = resized_width;        
    }

    int* layout_ptr = this->m_outputs[1].cpu<int>();
    layout_ptr[0] = fill_x;
    layout_ptr[1] = fill_y;
    layout_ptr[2] = fill_w;
    layout_ptr[3] = fill_h;

    layout_ptr[4] = image_w;
    layout_ptr[5] = image_h;
    layout_ptr[6] = int(m_rotate);
    
    // resize -> rotate -> fill 
#ifdef EAGLEEYE_RKCHIP
    if(image_c == 4){
        if(m_src_ptr == NULL || m_src_ptr != input[0].cpu()){
            if(m_src_ptr != NULL){
                releasebuffer_handle(m_src_handler);
            }

            m_src_ptr = input[0].cpu();
            m_src_handler = importbuffer_virtualaddr(m_src_ptr, image_h*image_w*4);
        }

        if(m_tgt_ptr == NULL || m_tgt_ptr != m_outputs[0].cpu()){
            if(m_tgt_ptr != NULL){
                releasebuffer_handle(m_tgt_handler);
            }

            m_tgt_ptr = m_outputs[0].cpu();
            m_tgt_handler = importbuffer_virtualaddr(m_tgt_ptr, this->m_target_h*this->m_target_w*4);
        }

        // step 1: resize
        if(m_resize_buf_size == 0 || m_resize_buf_size != (resized_width*resized_height*4)){
            if(m_resize_buf_size > 0){
                releasebuffer_handle(m_resize_handler);
                free(m_resize_ptr);
                m_resize_ptr = NULL;            
            }

            m_resize_ptr = malloc(resized_width*resized_height*4);
            m_resize_buf_size = resized_width*resized_height*4;
            m_resize_handler = importbuffer_virtualaddr(m_resize_ptr, m_resize_buf_size);
        }

        rga_buffer_t src_img, resized_img;
        memset(&src_img, 0, sizeof(src_img));
        memset(&resized_img, 0, sizeof(resized_img));
        src_img = wrapbuffer_handle(m_src_handler, image_w, image_h, RK_FORMAT_RGBA_8888);
        resized_img = wrapbuffer_handle(m_resize_handler, resized_width, resized_height, RK_FORMAT_RGBA_8888);
        imresize(src_img, resized_img);

        // step 2: rotate
        rga_buffer_t rotated_img = resized_img;
        if(m_rotate != IMAGE_ROTATE_0){
            if(m_rotate_buf_size == 0 || m_rotate_buf_size != m_resize_buf_size){
                if(m_rotate_buf_size > 0){
                    releasebuffer_handle(m_rotate_handler);
                    free(m_rotate_ptr);
                    m_rotate_ptr = NULL;            
                }

                m_rotate_ptr = malloc(m_resize_buf_size);
                m_rotate_buf_size = m_resize_buf_size;
                m_rotate_handler = importbuffer_virtualaddr(m_rotate_ptr, m_rotate_buf_size);
            }

            if(m_rotate == IMAGE_ROTATE_90){
                rotated_img = wrapbuffer_handle(m_rotate_handler, resized_height, resized_width, RK_FORMAT_RGBA_8888);
                imrotate(resized_img, rotated_img, IM_HAL_TRANSFORM_ROT_90);
            }
            else if(m_rotate == IMAGE_ROTATE_180){
                rotated_img = wrapbuffer_handle(m_rotate_handler, resized_width, resized_height, RK_FORMAT_RGBA_8888);
                imrotate(resized_img, rotated_img, IM_HAL_TRANSFORM_ROT_180);
            }
            else{
                rotated_img = wrapbuffer_handle(m_rotate_handler, resized_height, resized_width, RK_FORMAT_RGBA_8888);
                imrotate(resized_img, rotated_img, IM_HAL_TRANSFORM_ROT_270);
            }
        }

        // step 3: fill
        im_rect src_rect, dst_rect;
        src_rect.x = 0;
        src_rect.y = 0;
        src_rect.width = fill_w;
        src_rect.height = fill_h;

        dst_rect.x = fill_x;
        dst_rect.y = fill_y;
        dst_rect.width = fill_w;
        dst_rect.height = fill_h;

        rga_buffer_t dst_img;
        dst_img = wrapbuffer_handle(m_tgt_handler, m_target_w, m_target_h, RK_FORMAT_RGBA_8888);
        improcess(rotated_img, dst_img, {}, src_rect, dst_rect, {}, IM_SYNC);
        return 0;
    }
#endif

    if(image_c == 4){
        EAGLEEYE_LOGE("AutoKeepRatioOp dont support channel 4.");
        return -1;
    }

    // support image_c = 3
    if(m_resize_buf_size == 0 || m_resize_buf_size != resized_width*resized_height*3){
        if(m_resize_buf_size > 0){
            free(m_resize_ptr);
        }

        m_resize_buf_size = resized_width*resized_height*3;
        m_resize_ptr = malloc(m_resize_buf_size);
    }

    // resize
    unsigned char* x_ptr = (unsigned char*)input[0].cpu();
#if defined(__ANDROID__) || defined(ANDROID)    
        math::arm::bilinear_rgb_8u_3d_interp(
            x_ptr,
            (unsigned char*)m_resize_ptr,
            image_w,
            image_h,
            0,0,
            image_w,
            resized_width,
            resized_height
        );
#else
        math::x86::bilinear_rgb_8u_3d_interp(
            x_ptr,
            (unsigned char*)m_resize_ptr,
            image_w,
            image_h,
            0,0,
            image_w,
            resized_width,
            resized_height
        );
#endif

    // rotate
    unsigned char* use_rotate_ptr = (unsigned char*)m_resize_ptr;
    int use_rotate_width = resized_width;
    int use_rotate_height = resized_height;
    if(m_rotate != IMAGE_ROTATE_0){
        if(m_rotate_buf_size == 0 || m_rotate_buf_size != m_resize_buf_size){
            if(m_rotate_buf_size > 0){
                free(m_rotate_ptr);
                m_rotate_ptr = NULL;            
            }

            m_rotate_ptr = malloc(m_resize_buf_size);
            m_rotate_buf_size = m_resize_buf_size;
        }

        use_rotate_ptr = (unsigned char*)m_rotate_ptr;
        if(m_rotate == IMAGE_ROTATE_90){
            // 顺时针旋转90
            use_rotate_width = resized_height;
            use_rotate_height = resized_width;

            bgr_rotate_hwc((unsigned char*)m_resize_ptr, use_rotate_ptr, resized_width, resized_height, 90);
        }
        else if(m_rotate == IMAGE_ROTATE_180){
            // 顺时针旋转180
            use_rotate_width = resized_width;
            use_rotate_height = resized_height;

            bgr_rotate_hwc((unsigned char*)m_resize_ptr, use_rotate_ptr, resized_width, resized_height, 180);
        }
        else{
            // 顺时针旋转270
            use_rotate_width = resized_height;
            use_rotate_height = resized_width;

            bgr_rotate_hwc((unsigned char*)m_resize_ptr, use_rotate_ptr, resized_width, resized_height, 270);
        }
    }

    // fill
    unsigned char* tgt_ptr = this->m_outputs[0].cpu<unsigned char>();
    for(int i=0; i<use_rotate_height; ++i){
        unsigned char* tgt_fill_ptr = tgt_ptr + (i + fill_y) * m_target_w * 3 + fill_x*3;
        memcpy(tgt_fill_ptr, use_rotate_ptr+i*use_rotate_width*3, use_rotate_width*3);
    }
    return 0;
}

int AutoKeepRatioOp::runOnGpu(const std::vector<Tensor>& input){
    return 0;
}

}    
}