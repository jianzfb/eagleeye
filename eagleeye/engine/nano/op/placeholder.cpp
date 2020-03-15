#include "eagleeye/engine/nano/op/placeholder.h"
namespace eagleeye
{
namespace nano{    
Placeholder::Placeholder(std::string op_name):
             FixedCNNOp(0,1,FIXED_PLACEHOLDER, op_name){

}
Placeholder::~Placeholder(){

}

void Placeholder::run_on_cpu(std::vector<Tensor<FixedType>>& output){
    // do nothing
}

void Placeholder::run_on_gpu(std::vector<Tensor<FixedType>>& output){
    // do nothing
}

void Placeholder::run_on_dsp(std::vector<Tensor<FixedType>>& output){
    // do nothing  
}

bool Placeholder::init(char* buf, int in_size){
    // 从模型获取配置参数
    // do nothing
    return true;
}

int Placeholder::setShape(std::vector<int64_t> shape){
    // 分配输出tensor大小
    m_output_shape.resize(output_data_num_);
    m_output_shape[0] = shape;
    return 0;
}

void Placeholder::setInput(std::vector<Tensor<float>>& data){
    // 1.step transform float to fixed data

}
}
} // namespace eagleeye
