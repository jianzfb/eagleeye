#include "eagleeye/engine/nano/op/facealign_op.h"
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
#include "eagleeye/common/EagleeyeTime.h"
#include <fstream>

namespace eagleeye{
namespace dataflow{
FaceAlignOp::FaceAlignOp(int target_h, int target_w, int margin){
    this->m_target_h = target_h;
    this->m_target_w = target_w;
    this->m_margin = margin;

    this->m_rk_src_handler = 0;
    this->m_rk_cache_handler = 0;
    m_rk_mid_cache_handler = 0;
    m_rk_mid_cache_size = 0;
    m_rk_src_image_ptr = NULL;
}
FaceAlignOp::FaceAlignOp(){
    this->m_target_h = 32;
    this->m_target_w = 32;
    this->m_margin = 0;

    this->m_rk_src_handler = 0;
    this->m_rk_cache_handler = 0;
    m_rk_mid_cache_handler = 0;
    m_rk_mid_cache_size = 0;    
    m_rk_src_image_ptr = NULL;
}

FaceAlignOp::FaceAlignOp(const FaceAlignOp& op){
    this->m_target_h = op.m_target_h;
    this->m_target_w = op.m_target_w;
    this->m_margin = op.m_margin;
}

FaceAlignOp::~FaceAlignOp(){
#ifdef EAGLEEYE_RKCHIP
    if(this->m_rk_cache_handler > 0){
        releasebuffer_handle(rga_buffer_handle_t(m_rk_cache_handler));
        m_rk_cache_handler = 0;
    }
    if(this->m_rk_src_handler > 0){
        releasebuffer_handle(rga_buffer_handle_t(m_rk_src_handler));
        m_rk_src_handler = 0;
    }    
    if(this->m_rk_mid_cache_handler > 0){
        releasebuffer_handle(rga_buffer_handle_t(m_rk_mid_cache_handler));
        m_rk_mid_cache_handler = 0;
    }
    if(this->m_rk_mid_cache_ptr != NULL){
        free(this->m_rk_mid_cache_ptr);
        m_rk_mid_cache_ptr = NULL;
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
    if(image_dim.size() != 3){
        EAGLEEYE_LOGE("FaceAlignOp dont support image HxWx3, HxWx4(RK)");
        return -1;
    }
    int image_h = image_dim[0];
    int image_w = image_dim[1];
    int image_c = image_dim[2];
    unsigned char* image_ptr = image.cpu<unsigned char>();

    float* bbox_ptr = bbox.cpu<float>();
    float face_cx = (bbox_ptr[0]+bbox_ptr[2])/2.0f;
    float face_cy = (bbox_ptr[1]+bbox_ptr[3])/2.0f;
    float face_w = bbox_ptr[2]-bbox_ptr[0];
    float face_h = bbox_ptr[3]-bbox_ptr[1];
    float face_half_size = std::max(face_w, face_h) / 2;
    face_half_size = face_half_size + this->m_margin;

    // 保证4字节对齐
    int x0 = int(face_cx - face_half_size + 0.5f);
    x0 = (std::max(x0, 0) / 4) * 4;
    int y0 = int(face_cy - face_half_size + 0.5f);
    y0 = (std::max(y0, 0) / 4) * 4;
    int x1 = int(face_cx + face_half_size + 0.5f);
    x1 = (std::min(x1, image_w) / 4) * 4;
    int y1 = int(face_cy + face_half_size + 0.5f);
    y1 = (std::min(y1, image_h) / 4) * 4;

    int crop_face_w = x1 - x0;
    int crop_face_h = y1 - y0;
    if(crop_face_h == 0 || crop_face_w == 0){
        EAGLEEYE_LOGD("Face Crop zero.");
        return 0;
    }

#ifdef EAGLEEYE_RKCHIP
    if(image_c == 4){
        if(this->m_outputs[0].numel() != this->m_target_h*this->m_target_w*4){
            Dim out_dim(std::vector<int64_t>{this->m_target_h, this->m_target_w, 4});
            this->m_outputs[0] = Tensor(out_dim.data(),image.type(),image.format(),CPU_BUFFER);

            if(m_rk_cache_handler > 0){
                releasebuffer_handle(rga_buffer_handle_t(m_rk_cache_handler));
            }
            m_rk_cache_handler = importbuffer_virtualaddr(this->m_outputs[0].cpu<char>(), m_target_h * m_target_w * get_bpp_from_format(RK_FORMAT_RGBA_8888));
        }

        if(m_rk_src_image_ptr != (void*)(image_ptr)){
            if(m_rk_src_handler > 0){
                releasebuffer_handle(rga_buffer_handle_t(m_rk_src_handler));
            }
            m_rk_src_handler = importbuffer_virtualaddr(image_ptr, image_h * image_w * get_bpp_from_format(RK_FORMAT_RGBA_8888));
            m_rk_src_image_ptr = (void*)(image_ptr);
        }

        // crop and resize
        int src_width, src_height, src_format;
        int dst_width, dst_height, dst_format;
        int mid_width, mid_height, mid_format;
        int mid_buf_size;

        rga_buffer_t src_img, dst_img, mid_img;
        im_rect rect;

        memset(&src_img, 0, sizeof(src_img));
        memset(&dst_img, 0, sizeof(dst_img));
        memset(&mid_img, 0, sizeof(mid_img));
        memset(&rect, 0, sizeof(rect));
        src_width = image_w;
        src_height = image_h;
        src_format = RK_FORMAT_RGBA_8888;

        mid_width = crop_face_w;
        mid_height = crop_face_h;
        mid_format = RK_FORMAT_RGBA_8888;

        dst_width = m_target_w;
        dst_height = m_target_h;
        dst_format = RK_FORMAT_RGBA_8888;
        mid_buf_size = mid_width * mid_height * get_bpp_from_format(mid_format);
        if(m_rk_mid_cache_size < mid_buf_size){
            if(m_rk_mid_cache_handler > 0){
                releasebuffer_handle(rga_buffer_handle_t(m_rk_mid_cache_handler));
                m_rk_mid_cache_handler = 0;
            }
            if(m_rk_mid_cache_ptr != NULL){
                delete[] ((char*)m_rk_mid_cache_ptr);
                m_rk_mid_cache_ptr = NULL;
            }
            m_rk_mid_cache_ptr = (void*)(new char[mid_buf_size]);
            m_rk_mid_cache_handler = importbuffer_virtualaddr((char*)m_rk_mid_cache_ptr, mid_buf_size);
            m_rk_mid_cache_size = mid_buf_size;
        }

        src_img = wrapbuffer_handle(rga_buffer_handle_t(m_rk_src_handler), src_width, src_height, src_format);
        dst_img = wrapbuffer_handle(rga_buffer_handle_t(m_rk_cache_handler), dst_width, dst_height, dst_format);
        mid_img = wrapbuffer_handle(rga_buffer_handle_t(m_rk_mid_cache_handler), mid_width, mid_height, mid_format);

        rect.x = x0;
        rect.y = y0;
        rect.width = crop_face_w;
        rect.height = crop_face_h;
        imcrop(src_img, mid_img, rect);
        imresize(mid_img, dst_img);
        return 0;
    }
#endif

    // rgb/bgr
    if(image_c != 3){
        EAGLEEYE_LOGE("FaceAlignOp only support rgb/bgr image");
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
    return 0;
}

int FaceAlignOp::runOnGpu(const std::vector<Tensor>& input){
    return -1;
}
}
}