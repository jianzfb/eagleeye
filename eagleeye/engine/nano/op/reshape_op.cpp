#include "eagleeye/engine/nano/op/reshape_op.h"
#include <string>

namespace eagleeye{
namespace dataflow{
ReshapeOp::ReshapeOp(std::vector<int64_t> shape, bool in_place)
    :m_shape(shape),
        m_in_place(in_place){
    OP_SUPPORT(CPU);
}

ReshapeOp::ReshapeOp(const ReshapeOp& op)
    :m_shape(op.m_shape),
        m_in_place(op.m_in_place){
    OP_SUPPORT(CPU);
}

ReshapeOp::~ReshapeOp(){

}

int ReshapeOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

int ReshapeOp::runOnCpu(std::vector<Tensor> input){
    Tensor x = input[0];
    std::vector<int64_t> x_shape = x.shape();
    int x_size = x.numel();
    
    // 确定输出形状
    std::vector<int64_t> out_shape;
    for(int i=0; i<m_shape.size(); ++i){
        out_shape.push_back(m_shape[i]);
    }
    int m = 1;
    for(int i=0; i<out_shape.size(); ++i){
        if(out_shape[i] > 0){
            m *= out_shape[i];
        }
    }
    int n = x_size / m;
    if(n*m != x_size){
        EAGLEEYE_LOGE("Shape abnormal.");
        return -1;
    }

    int dynamic_pos = 0;
    for(int i=0; i<out_shape.size(); ++i){
        if(out_shape[i] < 0){
            out_shape[i] = n;
            dynamic_pos += 1;
        }
    }
    if(dynamic_pos > 1){
        EAGLEEYE_LOGE("More than one place is -1.");
        return -1;
    }

    // 内存共享，仅修改形状数据
    if(m_in_place){
        // 直接修改输入
        x.reshape(out_shape);
        x.setFormat(DataFormat::AUTO);
        this->m_outputs[0] = x;
    }
    else{
        // 重新生成 Tensor
        if(this->m_outputs[0].numel() != x.numel()){
            this->m_outputs[0] =          
                    Tensor(m_shape, x.type(), DataFormat::AUTO, CPU_BUFFER); 
        }
        this->m_outputs[0].copy(x);
    }
    return 0;
}

int ReshapeOp::runOnGpu(std::vector<Tensor> input){
    EAGLEEYE_LOGE("Dont implement (GPU)");
    return -1;
}
}    
}