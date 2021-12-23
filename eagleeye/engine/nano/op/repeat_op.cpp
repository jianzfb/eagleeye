#include "eagleeye/engine/nano/op/repeat_op.h"
#include "eagleeye/common/EagleeyeLog.h"

namespace eagleeye{
namespace dataflow{
RepeatOp::RepeatOp(int repeat_times, int axis){
    m_repeat_times = repeat_times;
    m_axis = axis;
    if(m_repeat_times <= 0){
        EAGLEEYE_LOGE("repeat times should be > 0.");
    }
    if(axis < 0){
        EAGLEEYE_LOGE("axis should be > 0.");
    }
}
RepeatOp::RepeatOp(const RepeatOp& op)
    :m_repeat_times(op.m_repeat_times),
        m_axis(op.m_axis){
    if(m_repeat_times <= 0){
        EAGLEEYE_LOGE("repeat times should be > 0.");
    }
    if(m_axis < 0){
        EAGLEEYE_LOGE("axis should be > 0.");
    }
}

RepeatOp::~RepeatOp(){

}

int RepeatOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

int RepeatOp::runOnCpu(std::vector<Tensor> input){
    Tensor x = input[0];
    // 合法性判断
    if(x.type() != EAGLEEYE_FLOAT){
        EAGLEEYE_LOGE("x type only support float.");
        return -1;
    }

    Dim x_dim = x.dims();
    if(m_axis >= x_dim.size()){
        EAGLEEYE_LOGE("m_axis larger than x_dim size.");
        return -1;
    }

    Dim out_dim = this->m_outputs[0].dims();
    std::vector<int64_t> needed_out_shape = x_dim.data();
    needed_out_shape[m_axis] *= m_repeat_times;
    Dim needed_out_dim(needed_out_shape);
    if(needed_out_dim.production() != out_dim.production()){
        this->m_outputs[0] =             
            Tensor(needed_out_dim.data(),
                    x.type(),
                    x.format(),
                    CPU_BUFFER);   
    }
    float* din = (float*)x.cpu();
    float* dout = (float*)this->m_outputs[0].cpu();
    out_dim = this->m_outputs[0].dims();

    int num = x_dim.count(0, m_axis);
    int repeat_num = x_dim.count(m_axis, x_dim.size());
    float* din_ptr = din;
    float* dout_ptr = dout;
#pragma omp parallel for
    for(int n=0; n<num; ++n){
        din_ptr = din + n * repeat_num;
        for(int k=0; k<m_repeat_times; ++k){
            memcpy(dout_ptr, din_ptr, sizeof(float)*repeat_num);
            dout_ptr += repeat_num;
        }
    }
    return 0;
}

int RepeatOp::runOnGpu(std::vector<Tensor> input){
    return -1;
}
} // namespace dataflow
} // namespace eagleeye
