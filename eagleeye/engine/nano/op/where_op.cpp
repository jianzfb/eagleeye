#include "eagleeye/engine/nano/op/where_op.h"
namespace eagleeye{
namespace dataflow{
WhereOp::WhereOp(){
    OP_SUPPORT(CPU);
}    
WhereOp::WhereOp(const WhereOp& op){
    OP_SUPPORT(CPU);
}
int WhereOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

template <typename T>
void where_kernel(const bool* cond_data, const T* x_data, const T* y_data, T* out_data, Dim dims) {
  auto numel = dims.production();
  for (int i = 0; i < numel; i++) {
    out_data[i] = cond_data[i] ? x_data[i] : y_data[i];
  }
}

int WhereOp::runOnCpu(const std::vector<Tensor>& input){
    const Tensor condition = input[0];    // bool
    const Tensor x = input[1];
    const Tensor y = input[2];

    if(condition.type() != EAGLEEYE_BOOL){
        EAGLEEYE_LOGE("condition only support bool.");
        return -1;
    }
    if(x.type() != EAGLEEYE_FLOAT && x.type() != EAGLEEYE_INT){
        EAGLEEYE_LOGE("x only support float,int.");
        return -1;
    }
    if(y.type() != EAGLEEYE_FLOAT && y.type() != EAGLEEYE_INT){
        EAGLEEYE_LOGE("y only support float,int.");
        return -1;
    }
    if(x.numel() != y.numel()){
        EAGLEEYE_LOGE("x and y size must be same.");
        return -1;
    }

    Dim out_dim = this->m_outputs[0].dims();
    if(out_dim.size() == 0 || out_dim.production() != x.numel()){
        this->m_outputs[0] = Tensor(x.dims().data(),
                    x.type(),
                    DataFormat::AUTO,
                    CPU_BUFFER); 
    }

    switch (x.type()) {
        case EAGLEEYE_FLOAT:
            where_kernel<float>(condition.cpu<bool>(), x.cpu<float>(), y.cpu<float>(), this->m_outputs[0].cpu<float>(), x.dims());
            break;
        case EAGLEEYE_INT:
            where_kernel<int>(condition.cpu<bool>(), x.cpu<int>(), y.cpu<int>(), this->m_outputs[0].cpu<int>(), x.dims());
            break;
        default:
            EAGLEEYE_LOGE("Where does not implement for the input type: %d", static_cast<int>(x.type()));
    }
    return 0;
}
int WhereOp::runOnGpu(const std::vector<Tensor>& input){
    return 0;
}
} // namespace dataflow
} // namespace eagleeye
