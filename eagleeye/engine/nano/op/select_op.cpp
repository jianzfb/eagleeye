#include "eagleeye/engine/nano/op/select_op.h"
namespace eagleeye{
namespace dataflow{
SelectOp::SelectOp(int begin, int end)
    :m_begin(begin),
     m_end(end){

}

SelectOp::SelectOp(const SelectOp& op)
    :m_begin(op.m_begin),
     m_end(op.m_end){

}

SelectOp::~SelectOp(){

}

int SelectOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}
int SelectOp::runOnCpu(const std::vector<Tensor>& input){
    const Tensor x = input[0];
    const Tensor offset_index = input[1];
    if(x.type() != EAGLEEYE_FLOAT || offset_index.type() != EAGLEEYE_INT){
        EAGLEEYE_LOGE("x must be float, offset must be int32.");
        return -1;
    }
    
    Dim x_dims = x.dims();
    int x_b = x_dims[0];
    int x_inner_size = x_dims.count(1,x_dims.size());
    const float* x_data = x.cpu<float>();
    const int32_t* offset_index_data = offset_index.cpu<int32_t>();
    if(this->m_end == -1){
        this->m_end = this->m_begin+1;
    }
    if(this->m_end - this->m_begin == 0){
        return -1;
    }

    int out_size = offset_index_data[this->m_end] - offset_index_data[this->m_begin];
    Dim out_dim = this->m_outputs[0].dims();
    if(out_dim.size() == 0 || out_dim.production() != out_size*x_inner_size){
        std::vector<int64_t> out_shape = x_dims.data();
        out_shape[0] = out_size;
        this->m_outputs[0] =             
            Tensor(out_shape,
                    EAGLEEYE_FLOAT,
                    DataFormat::AUTO,
                    CPU_BUFFER); 
    }

    float* out_data = this->m_outputs[0].cpu<float>();
    for(int i=offset_index_data[this->m_begin]; i<offset_index_data[this->m_end]; ++i){
        memcpy(out_data, x_data+i*x_inner_size, sizeof(float)*x_inner_size);
        out_data += x_inner_size;
    }
    return 0;
}
int SelectOp::runOnGpu(const std::vector<Tensor>& input){
    return 0;
}

} // namespace dataflow
} // namespace eagleeye