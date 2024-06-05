#include "eagleeye/engine/nano/op/pool2d_op.h"

#if defined (__ARM_NEON) || defined (__ARM_NEON__)
#include "eagleeye/engine/math/arm/pooling.h"
#endif

namespace eagleeye{
namespace dataflow{
Pool2dOp::Pool2dOp(Pool2dType pool_type, int ksize_h, int ksize_w, int stride_h, int stride_w, int padding_h, int padding_w)
    :m_ksize_h(ksize_h),
     m_ksize_w(ksize_w),
     m_stride_h(stride_h),
     m_stride_w(stride_w),
     m_padding_h(padding_h),
     m_padding_w(padding_w),
     m_pool_type(pool_type){
    
}

Pool2dOp::Pool2dOp(const Pool2dOp& op)
    :m_ksize_h(op.m_ksize_h),
     m_ksize_w(op.m_ksize_w),
     m_stride_h(op.m_stride_h),
     m_stride_w(op.m_stride_w),
     m_padding_h(op.m_padding_h),
     m_padding_w(op.m_padding_w),
     m_pool_type(op.m_pool_type){

}

Pool2dOp::~Pool2dOp(){

}

int Pool2dOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

int Pool2dOp::runOnCpu(const std::vector<Tensor>& input){
    const Tensor x = input[0];
    if(x.type() != EAGLEEYE_FLOAT){
        EAGLEEYE_LOGE("x type only support float.");
        return -1;
    }
    Dim x_dims = x.dims();
    if(x_dims.size() != 4){
        EAGLEEYE_LOGE("x dims size must be 4 but received %d", x_dims.size());
        return -1;
    }
    int64_t x_b = x_dims[0];
    int64_t x_c = x_dims[1];
    int64_t x_h = x_dims[2];
    int64_t x_w = x_dims[3];

    Dim out_dim = this->m_outputs[0].dims();
    std::vector<int64_t> out_shape={x_b, x_c, ((x_h+2*m_padding_h-(m_ksize_h-1)-1)/m_stride_h + 1), ((x_w+2*m_padding_w-(m_ksize_w-1)-1)/m_stride_w + 1)};

    Dim needed_out_dim(out_shape);
    if(out_dim.size() == 0 || out_dim.production() != needed_out_dim.production()){
        this->m_outputs[0] =             
            Tensor(needed_out_dim.data(),
                    x.type(),
                    DataFormat::AUTO,
                    CPU_BUFFER); 
    }

    const float* x_data = x.cpu<float>();
    float* o_data = this->m_outputs[0].cpu<float>();
    if(this->m_ksize_h==2 && this->m_ksize_w==2 && 
        this->m_stride_h==2 && this->m_stride_w==2 && 
        this->m_padding_h==0 && this->m_padding_w==0){
        // pooling2x2s2p0_max
        // pooling2x2s2p0_avg
        if(m_pool_type == MAXPool2D){
#if defined (__ARM_NEON) || defined (__ARM_NEON__)            
            math::arm::pooling2x2s2p0_max(
                x_data, 
                o_data, 
                x_b, 
                needed_out_dim[1], needed_out_dim[2],needed_out_dim[3],
                x_dims[1],x_dims[2],x_dims[3], m_padding_h,m_padding_w
                );
#endif
        }
        else{
#if defined (__ARM_NEON) || defined (__ARM_NEON__)  
            math::arm::pooling2x2s2p0_avg(
                x_data, 
                o_data, 
                x_b, 
                needed_out_dim[1], needed_out_dim[2],needed_out_dim[3],
                x_dims[1],x_dims[2],x_dims[3], true, m_padding_h,m_padding_w
                );
#endif
        }
    }
    else if(this->m_ksize_h==2 && this->m_ksize_w==2 && 
        this->m_stride_h==2 && this->m_stride_w==2 && 
        this->m_padding_h==1 && this->m_padding_w==1){
        // pooling2x2s2p1_max
        // pooling2x2s2p1_avg
        if(m_pool_type == MAXPool2D){
#if defined (__ARM_NEON) || defined (__ARM_NEON__)            
            math::arm::pooling2x2s2p1_max(
                x_data, 
                o_data, 
                x_b, 
                needed_out_dim[1], needed_out_dim[2],needed_out_dim[3],
                x_dims[1],x_dims[2],x_dims[3], m_padding_h,m_padding_w
                );
#endif
        }
        else{
#if defined (__ARM_NEON) || defined (__ARM_NEON__)  
            math::arm::pooling2x2s2p1_avg(
                x_data, 
                o_data, 
                x_b, 
                needed_out_dim[1], needed_out_dim[2],needed_out_dim[3],
                x_dims[1],x_dims[2],x_dims[3], true, m_padding_h,m_padding_w
                );
#endif
        }   
    }
    else if(this->m_ksize_h==3 && this->m_ksize_w==3 && 
        this->m_stride_h==1 && this->m_stride_w==1 && 
        this->m_padding_h==1 && this->m_padding_w==1){
        // pooling3x3s1p1_max
        // pooling3x3s1p1_avg
        if(m_pool_type == MAXPool2D){
#if defined (__ARM_NEON) || defined (__ARM_NEON__)            
            math::arm::pooling3x3s1p1_max(
                x_data, 
                o_data, 
                x_b, 
                needed_out_dim[1], needed_out_dim[2],needed_out_dim[3],
                x_dims[1],x_dims[2],x_dims[3], m_padding_h,m_padding_w
                );
#endif
        }
        else{
#if defined (__ARM_NEON) || defined (__ARM_NEON__)  
            math::arm::pooling3x3s1p1_avg(
                x_data, 
                o_data, 
                x_b, 
                needed_out_dim[1], needed_out_dim[2],needed_out_dim[3],
                x_dims[1],x_dims[2],x_dims[3], true, m_padding_h,m_padding_w
                );
#endif
        }          
    }
    else if(this->m_ksize_h==3 && this->m_ksize_w==3 && 
        this->m_stride_h==2 && this->m_stride_w==2 && 
        this->m_padding_h==1 && this->m_padding_w==1){
        // pooling3x3s2p1_max
        // pooling3x3s2p1_avg
        if(m_pool_type == MAXPool2D){
#if defined (__ARM_NEON) || defined (__ARM_NEON__)            
            math::arm::pooling3x3s2p1_max(
                x_data, 
                o_data, 
                x_b, 
                needed_out_dim[1], needed_out_dim[2],needed_out_dim[3],
                x_dims[1],x_dims[2],x_dims[3], m_padding_h,m_padding_w
                );
#endif
        }
        else{
#if defined (__ARM_NEON) || defined (__ARM_NEON__)  
            math::arm::pooling3x3s2p1_avg(
                x_data, 
                o_data, 
                x_b, 
                needed_out_dim[1], needed_out_dim[2],needed_out_dim[3],
                x_dims[1],x_dims[2],x_dims[3], true, m_padding_h,m_padding_w
                );
#endif
        }            
    }
    else if(this->m_ksize_h==3 && this->m_ksize_w==3 && 
        this->m_stride_h==1 && this->m_stride_w==1 && 
        this->m_padding_h==0 && this->m_padding_w==0){
        // pooling3x3s1p0_max
        // pooling3x3s1p0_avg
        if(m_pool_type == MAXPool2D){
#if defined (__ARM_NEON) || defined (__ARM_NEON__)            
            math::arm::pooling3x3s1p0_max(
                x_data, 
                o_data, 
                x_b, 
                needed_out_dim[1], needed_out_dim[2],needed_out_dim[3],
                x_dims[1],x_dims[2],x_dims[3], m_padding_h,m_padding_w
                );
#endif
        }
        else{
#if defined (__ARM_NEON) || defined (__ARM_NEON__)  
            math::arm::pooling3x3s1p0_avg(
                x_data, 
                o_data, 
                x_b, 
                needed_out_dim[1], needed_out_dim[2],needed_out_dim[3],
                x_dims[1],x_dims[2],x_dims[3], true, m_padding_h,m_padding_w
                );
#endif
        }           
    }    
    else if(this->m_ksize_h==3 && this->m_ksize_w==3 && 
        this->m_stride_h==2 && this->m_stride_w==2 && 
        this->m_padding_h==1 && this->m_padding_w==1){
        // pooling3x3s2p1_max
        // pooling3x3s2p1_avg
        if(m_pool_type == MAXPool2D){
#if defined (__ARM_NEON) || defined (__ARM_NEON__)            
            math::arm::pooling3x3s2p1_max(
                x_data, 
                o_data, 
                x_b, 
                needed_out_dim[1], needed_out_dim[2],needed_out_dim[3],
                x_dims[1],x_dims[2],x_dims[3], m_padding_h,m_padding_w
                );
#endif
        }
        else{
#if defined (__ARM_NEON) || defined (__ARM_NEON__)  
            math::arm::pooling3x3s2p1_avg(
                x_data, 
                o_data, 
                x_b, 
                needed_out_dim[1], needed_out_dim[2],needed_out_dim[3],
                x_dims[1],x_dims[2],x_dims[3], true, m_padding_h,m_padding_w
                );
#endif
        }                  
    }     
    else if(this->m_ksize_h==3 && this->m_ksize_w==3 && 
        this->m_stride_h==2 && this->m_stride_w==2 && 
        this->m_padding_h==0 && this->m_padding_w==0){
        // pooling3x3s2p0_max
        // pooling3x3s2p0_avg
        if(m_pool_type == MAXPool2D){
#if defined (__ARM_NEON) || defined (__ARM_NEON__)            
            math::arm::pooling3x3s2p0_max(
                x_data, 
                o_data, 
                x_b, 
                needed_out_dim[1], needed_out_dim[2],needed_out_dim[3],
                x_dims[1],x_dims[2],x_dims[3], m_padding_h,m_padding_w
                );
#endif
        }
        else{
#if defined (__ARM_NEON) || defined (__ARM_NEON__)  
            math::arm::pooling3x3s2p0_avg(
                x_data, 
                o_data, 
                x_b, 
                needed_out_dim[1], needed_out_dim[2],needed_out_dim[3],
                x_dims[1],x_dims[2],x_dims[3], true, m_padding_h,m_padding_w
                );
#endif
        }           
    }        
    return 0;
}

int Pool2dOp::runOnGpu(const std::vector<Tensor>& input){
    return 0;
}
} // namespace math
} // namespace eagleeye
