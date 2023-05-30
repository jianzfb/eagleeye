#include "eagleeye/engine/nano/op/warp_affine_op.h"
#include "eagleeye/engine/math/common/pixel_affine.h"

namespace eagleeye{
namespace dataflow{
WarpAffineOp::WarpAffineOp(AffineParamMode mode, int64_t out_h, int64_t out_w, int fill_type, unsigned int v)
    :m_fill_type(fill_type),
     m_out_h(out_h),
     m_out_w(out_w),
     m_v(v),
     m_mode(mode){
}
WarpAffineOp::~WarpAffineOp(){

}

int WarpAffineOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

int WarpAffineOp::runOnCpu(const std::vector<Tensor>& input){
    // void warpaffine_bilinear_c4(const unsigned char* src, int srcw, int srch, unsigned char* dst, int w, int h, const float* tm, int type, unsigned int v);
    // void warpaffine_bilinear_c3(const unsigned char* src, int srcw, int srch, unsigned char* dst, int w, int h, const float* tm, int type, unsigned int v);
    // void warpaffine_bilinear_c2(const unsigned char* src, int srcw, int srch, unsigned char* dst, int w, int h, const float* tm, int type, unsigned int v);
    // void warpaffine_bilinear_c1(const unsigned char* src, int srcw, int srch, unsigned char* dst, int w, int h, const float* tm, int type, unsigned int v);
    const Tensor x = input[0];
    if(x.type() != EAGLEEYE_CHAR && x.type() != EAGLEEYE_UCHAR){
        EAGLEEYE_LOGE("x type only support uchar.");
        return -1;
    }

    if(x.format() != DataFormat::NHWC){
        EAGLEEYE_LOGE("x format only support NHWC.");
        return -1;
    }
    Dim x_dims = x.dims();
    if(x_dims[3] != 1 && x_dims[3] != 2 && x_dims[3] != 3 && x_dims[3] != 4){
        EAGLEEYE_LOGE("x channels only support 1,2,3,4.");
        return -1;
    }

    const Tensor affine_params = input[1];
    if(affine_params.type() != EAGLEEYE_FLOAT){
        EAGLEEYE_LOGE("affine parameter only support float.");
        return -1;
    }    
    Dim affine_params_dims = affine_params.dims();
    if(m_mode == RotationMode){
        if((affine_params_dims.size() == 2 && affine_params_dims[1] != 4) || (affine_params_dims.size() == 1 && affine_params_dims[0] != 4)){
            EAGLEEYE_LOGE("affine parameter must be rotation,scale,center_x,center_y (RotationMode).");
            return -1;
        }
    }
    else if(m_mode == AffineMode){
        if((affine_params_dims.size() == 2 && affine_params_dims[1] % 4 != 0) || (affine_params_dims.size() == 1 && affine_params_dims[0] % 4 != 0)){
            EAGLEEYE_LOGE("affine parameter must be 3 pair points (AffineMode)");
            return -1;
        }
    }
    else{
        if((affine_params_dims.size() == 2 && affine_params_dims[1] != 6) || (affine_params_dims.size() == 1 && affine_params_dims[0] != 6)){
            EAGLEEYE_LOGE("affine parameter must be 3 pair points (AffineMode)");
            return -1;
        }
    }

    const unsigned char* x_data = x.cpu<unsigned char>();
    int batch_size = x_dims[0];
    int batch_offset = x_dims.count(1, 4);
    int H = x_dims[1]; int W = x_dims[2]; int C = x_dims[3];
    const unsigned char* batch_x_data = x_data;
    Dim out_dim = this->m_outputs[0].dims();

    Dim affine_params_dim = affine_params.dims();
    int out_batch_size = batch_size;
    if(affine_params_dims.size() == 2 && batch_size == 1){
        out_batch_size = affine_params_dims[0];
        batch_offset = 0;
    }

    if(out_dim.size() == 0 || out_dim.production() != out_batch_size * m_out_h * m_out_w *C){
        this->m_outputs[0] =             
            Tensor(std::vector<int64_t>{out_batch_size, m_out_h, m_out_w, C},
                    x.type(),
                    DataFormat::NHWC,
                    CPU_BUFFER); 
    }
    if(this->m_outputs[1].dims().size() == 0 || this->m_outputs[1].dims()[0] != out_batch_size){
        this->m_outputs[1] =             
            Tensor(std::vector<int64_t>{out_batch_size, 2, 3},
                    EAGLEEYE_FLOAT,
                    DataFormat::AUTO,
                    CPU_BUFFER); 
        this->m_outputs[2] =             
            Tensor(std::vector<int64_t>{out_batch_size, 2, 3},
                    EAGLEEYE_FLOAT,
                    DataFormat::AUTO,
                    CPU_BUFFER); 
    }

    unsigned char* batch_out_data = this->m_outputs[0].cpu<unsigned char>();
    int batch_out_offset = m_out_w * m_out_h * C;
    const float* affine_params_data = affine_params.cpu<float>();
    int affine_param_offset = m_mode==RotationMode ? 4 : affine_params_dims[1];
    if(affine_params.dims().size() == 1){
        affine_param_offset = 0;
    }

    float* tm_out_data = this->m_outputs[1].cpu<float>();
    float* inv_tm_out_data = this->m_outputs[2].cpu<float>();

    for(int batch_i=0; batch_i<out_batch_size; ++batch_i){
        // 计算变换参数
        if(m_mode == RotationMode){
            float affine_rotation = affine_params_data[0];
            float affine_scale = affine_params_data[1];
            int affine_center_x = (int)(affine_params_data[2]);
            int affine_center_y = (int)(affine_params_data[3]);        
            eagleeye::math::get_rotation_matrix(affine_rotation, affine_scale, affine_center_x, affine_center_y, tm_out_data);
        }
        else if(m_mode == AffineMode){
            const float* points_from = affine_params_data;
            const float* points_to = affine_params_data + 2*(affine_params_dims[1]/4);
            
            eagleeye::math::get_affine_transform(points_to, points_from, affine_params_dims[1]/4, tm_out_data);
        }
        else{
            memcpy(tm_out_data, affine_params_data, sizeof(float)*6);
        }
        // 计算逆变换
        eagleeye::math::invert_affine_transform(tm_out_data, inv_tm_out_data);

        // 仿射变换
        switch (C)
        {
        case 1:
            eagleeye::math::warpaffine_bilinear_c1(batch_x_data, W, H, batch_out_data, m_out_w, m_out_h, tm_out_data, m_fill_type, m_v);
            break;
        case 2:
            eagleeye::math::warpaffine_bilinear_c2(batch_x_data, W, H, batch_out_data, m_out_w, m_out_h, tm_out_data, m_fill_type, m_v);
            break;
        case 3:
            eagleeye::math::warpaffine_bilinear_c3(batch_x_data, W, H, batch_out_data, m_out_w, m_out_h, tm_out_data, m_fill_type, m_v);
            break;
        case 4:
            eagleeye::math::warpaffine_bilinear_c4(batch_x_data, W, H, batch_out_data, m_out_w, m_out_h, tm_out_data, m_fill_type, m_v);
            break;
        default:
            break;
        }

        batch_x_data += batch_offset;
        batch_out_data += batch_out_offset;

        affine_params_data += affine_param_offset;
        tm_out_data += 6;
        inv_tm_out_data += 6;
    }
    return 0;
}
int WarpAffineOp::runOnGpu(const std::vector<Tensor>& input){
    return 0;
}


} // namespace dataflow

} // namespace eagleeye
