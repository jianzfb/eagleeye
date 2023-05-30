#include "eagleeye/engine/nano/op/pose_affine_op.h"
namespace eagleeye{
namespace dataflow{
PoseAffineOp::PoseAffineOp(float x_scale, float y_scale, float x_offset, float y_offset)
    :m_x_scale(x_scale),
     m_y_scale(y_scale),
     m_x_offset(x_offset),
     m_y_offset(y_offset){

}

int PoseAffineOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}
int PoseAffineOp::runOnCpu(const std::vector<Tensor>& input){
    // x shape: N,Joints,2
    const Tensor x = input[0];
    // tm shape: N,2,3
    const Tensor tm = input[1];
    if(x.type() != EAGLEEYE_FLOAT){
        EAGLEEYE_LOGE("x type only support float");
        return -1;
    }
    Dim x_dims = x.dims();
    if(x_dims.size() != 3 || x_dims[2] != 2){
        EAGLEEYE_LOGE("x_dims must be N,JOINT_NU,2.");
        return -1;
    }

    if(tm.type() != EAGLEEYE_FLOAT){
        EAGLEEYE_LOGE("tm type only support float");
        return -1;
    }

    Dim tm_dims = tm.dims();
    if(tm_dims.size() != 3 || tm_dims[1] != 2 || tm_dims[2] != 3){
        EAGLEEYE_LOGE("tm_dims must be N,2,3.");
        return -1;
    }

    const float* x_data = x.cpu<float>();
    const float* tm_data = tm.cpu<float>();
    int x_b = x_dims[0];
    int x_joint_num = x_dims[1];
    int tm_offset = tm_dims[0] == 1 ? 0 : 6;
    Dim out_dim = this->m_outputs[0].dims();
    if(out_dim.size() == 0 || out_dim.production() != x_dims.production()){
        this->m_outputs[0] =             
            Tensor(x_dims.data(),
                    x.type(),
                    DataFormat::AUTO,
                    CPU_BUFFER); 
    }
    float* out_data = this->m_outputs[0].cpu<float>();
    for(int b_i=0; b_i<x_b; ++b_i){
        for(int joint_i=0; joint_i<x_joint_num; ++joint_i){
            out_data[0] = tm_data[0]*x_data[0] + tm_data[1]*x_data[1] + tm_data[2];
            out_data[1] = tm_data[3]*x_data[0] + tm_data[4]*x_data[1] + tm_data[5];

            out_data[0] = out_data[0] * m_x_scale + m_x_offset;
            out_data[1] = out_data[1] * m_y_scale + m_y_offset;

            x_data += 2;
            out_data += 2;
        }
        tm_data += tm_offset;
    }
    return 0;
}
int PoseAffineOp::runOnGpu(const std::vector<Tensor>& input){
    return 0;
}

} // namespace dataflow
} // namespace eagleeyn