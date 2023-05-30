#include "eagleeye/engine/nano/op/argmax_op.h"
#include "eagleeye/engine/math/common/argmax.h"
namespace eagleeye{
namespace dataflow{
ArgmaxOp::ArgmaxOp(int64_t axis)
    :m_axis(axis){

}

ArgmaxOp::~ArgmaxOp(){

}

int ArgmaxOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

int ArgmaxOp::runOnCpu(const std::vector<Tensor>& input){
    const Tensor x = input[0];
    if(x.type() != EAGLEEYE_FLOAT){
        EAGLEEYE_LOGE("x type only support float.");
        return -1;
    }    
    if(x.dims().size() == 0 || x.dims()[0] == 0){
        return -1;
    }

    Dim out_dim = this->m_outputs[0].dims();
    Dim needed_out_dim = x.dims();
    needed_out_dim[m_axis] = 1;

    if(out_dim.size() == 0 || out_dim.production() != needed_out_dim.production()){
        this->m_outputs[0] =             
            Tensor(needed_out_dim.data(),
                    EAGLEEYE_INT,
                    DataFormat::AUTO,
                    CPU_BUFFER); 
    }

    math::argmax_func<float, int32_t>(x, m_axis, m_outputs[0]);
    return 0;
}

int ArgmaxOp::runOnGpu(const std::vector<Tensor>& input){
    return 0;
}

} // namespace dataflow
} // namespace eagleeye
