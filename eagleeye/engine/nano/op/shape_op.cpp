#include "eagleeye/engine/nano/op/shape_op.h"
#include <string>


namespace eagleeye{
namespace dataflow{
ShapeOp::ShapeOp(std::vector<int64_t> axes)
    :m_axes(axes){
    OP_SUPPORT(CPU);
}

ShapeOp::ShapeOp(const ShapeOp& op)
    :m_axes(op.m_axes){
    OP_SUPPORT(CPU);
}


ShapeOp::~ShapeOp(){

}

int ShapeOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

int ShapeOp::runOnCpu(std::vector<Tensor> input){
    std::vector<int64_t> shape = input[0].shape();
    Tensor t({(int64_t)(shape.size())}, EAGLEEYE_INT, DataFormat::AUTO, CPU_BUFFER);
    int32_t* data = (int32_t*)t.cpu();
    for(int i=0; i<m_axes.size(); ++i){
        data[i] = shape[m_axes[i]];
    }

    this->m_outputs[0] = t;
    return 0;
}

int ShapeOp::runOnGpu(std::vector<Tensor> input){
    return this->runOnCpu();
}
}
}