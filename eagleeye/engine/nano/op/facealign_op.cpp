#include "eagleeye/engine/nano/op/facealign_op.h"
#if defined(__ANDROID__) || defined(ANDROID)  
#include "eagleeye/engine/math/arm/interpolate.h"
#else
#include "eagleeye/engine/math/x86/interpolate.h"
#endif
#include "eagleeye/common/EagleeyeLog.h"
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
FaceAlignOp::FaceAlignOp(){
    m_src_handler = 0;
    m_tgt_handler = 0;

    m_src_ptr = NULL;
    m_tgt_ptr = NULL;

    this->m_margin = 10;
}
FaceAlignOp::FaceAlignOp(int target_h, int target_w, int margin){
    this->m_target_h = target_h;
    this->m_target_w = target_w;
    this->m_margin = margin;

    m_src_handler = 0;
    m_tgt_handler = 0;

    m_src_ptr = NULL;
    m_tgt_ptr = NULL;
}

FaceAlignOp::~FaceAlignOp(){
#ifdef EAGLEEYE_RKCHIP    
    if (m_src_handler){
        releasebuffer_handle(m_src_handler);
    }
    if (m_tgt_handler){
        releasebuffer_handle(m_tgt_handler);
    }
#endif
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
    if(image_dim.size() == 4){
        EAGLEEYE_LOGE("FaceAlignOp dont support batch image");
        return -1;
    }

    int image_h = image_dim[0];
    int image_w = image_dim[1];
    int image_c = image_dim[2];
    unsigned char* image_ptr = image.cpu<unsigned char>();

    if(bbox.dims()[0] == 0){
        EAGLEEYE_LOGD("No face.");
        if(this->m_outputs[0].numel() != this->m_target_h*this->m_target_w*image_c){
            Dim out_dim(std::vector<int64_t>{this->m_target_h, this->m_target_w, image_c});
            this->m_outputs[0] = Tensor(out_dim.data(),image.type(),image.format(),CPU_BUFFER);
        }
        return 0;
    }
    float* bbox_ptr = bbox.cpu<float>();
    float face_cx = (bbox_ptr[0]+bbox_ptr[2])/2.0f;
    float face_cy = (bbox_ptr[1]+bbox_ptr[3])/2.0f;
    float face_w = bbox_ptr[2]-bbox_ptr[0];
    float face_h = bbox_ptr[3]-bbox_ptr[1];
    float face_half_size = std::max(face_w, face_h) / 2;
    face_half_size = face_half_size + this->m_margin;

    // 保证4宽度对齐（对于一些硬件加速使用）
    int x0 = int(face_cx - face_half_size + 0.5f);
    x0 = std::max(x0, 0) / 4 * 4;
    int y0 = int(face_cy - face_half_size + 0.5f);
    y0 = std::max(y0, 0) / 4 * 4;
    int x1 = int(face_cx + face_half_size + 0.5f);
    x1 = (std::min(x1+3, image_w)) / 4 * 4;
    int y1 = int(face_cy + face_half_size + 0.5f);
    y1 = (std::min(y1+3, image_h)) / 4 * 4;

    face_w = x1 - x0;
    face_h = y1 - y0;
    if(face_w == 0 || face_h == 0){
        return 0;
    }

#ifdef EAGLEEYE_RKCHIP
    // ->crop -> resize
    if(image_c == 4){
        if(m_src_ptr == NULL || m_src_ptr != input[0].cpu()){
            if(m_src_ptr != NULL){
                releasebuffer_handle(m_src_handler);
            }

            m_src_ptr = input[0].cpu();
            m_src_handler = importbuffer_virtualaddr(m_src_ptr, image_h*image_w*4);
        }

        if(this->m_outputs[0].numel() != this->m_target_h*this->m_target_w*4){
            Dim out_dim(std::vector<int64_t>{this->m_target_h, this->m_target_w, 4});
            this->m_outputs[0] = Tensor(out_dim.data(),image.type(),image.format(),CPU_BUFFER);
        }

        if(m_tgt_ptr == NULL || m_tgt_ptr != m_outputs[0].cpu()){
            if(m_tgt_ptr != NULL){
                releasebuffer_handle(m_tgt_handler);
            }

            m_tgt_ptr = m_outputs[0].cpu();
            m_tgt_handler = importbuffer_virtualaddr(m_tgt_ptr, this->m_target_h*this->m_target_w*4);
        }

        char* crop_buf = (char *)malloc(face_w*face_h*4);
        memset(crop_buf, 0x80, face_w*face_h*4);
        rga_buffer_handle_t crop_handle = importbuffer_virtualaddr(crop_buf, face_w*face_h*4);

        rga_buffer_t src_img, crop_img, dst_img;
        memset(&src_img, 0, sizeof(src_img));
        memset(&crop_img, 0, sizeof(crop_img));
        memset(&dst_img, 0, sizeof(dst_img));
        im_rect rect;
        memset(&rect, 0, sizeof(rect));
        rect.x = x0;
        rect.y = y0;
        rect.width = face_w;
        rect.height = face_h;

        src_img = wrapbuffer_handle(m_src_handler, image_w, image_h, RK_FORMAT_RGBA_8888);
        crop_img = wrapbuffer_handle(crop_handle, face_w, face_h, RK_FORMAT_RGBA_8888);
        dst_img = wrapbuffer_handle(m_tgt_handler, m_target_w, m_target_h, RK_FORMAT_RGBA_8888);

        imcrop(src_img, crop_img, rect);
        imresize(crop_img, dst_img);

        free(crop_buf);
        releasebuffer_handle(crop_handle);
        return 0;
    }
#endif
    if(image_c == 4){
        EAGLEEYE_LOGE("Todo support channel = 4 facealign.");
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