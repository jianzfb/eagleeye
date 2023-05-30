#include "eagleeye/engine/nano/op/shape_op.h"
#include <string>


namespace eagleeye{
namespace dataflow{
ShapeOp::ShapeOp(int64_t start, int64_t stop, EagleeyeType data_type)
    :m_start(start),
     m_stop(stop),
     m_data_type(data_type){
    OP_SUPPORT(CPU);
}

ShapeOp::ShapeOp(const ShapeOp& op)
    :m_start(op.m_start),
     m_stop(op.m_stop),
     m_data_type(op.m_data_type){
    OP_SUPPORT(CPU);
}


ShapeOp::~ShapeOp(){

}

int ShapeOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

int ShapeOp::runOnCpu(const std::vector<Tensor>& input){
    std::vector<int64_t> shape = input[0].dims().data();
    if(m_data_type != EAGLEEYE_INT && m_data_type != EAGLEEYE_FLOAT){
        EAGLEEYE_LOGE("data_type only support int,float.");
        return -1;
    }

    m_start = m_start<0 ? 0 : m_start;
    m_stop = (m_stop<0 || m_stop>shape.size()) ? shape.size(): m_stop;

    Tensor out({(int64_t)(m_stop-m_start)}, m_data_type, DataFormat::AUTO, CPU_BUFFER);

    if(m_data_type == EAGLEEYE_INT){
        int32_t* out_data = out.cpu<int32_t>();
        for(int i=m_start; i<m_stop; ++i){
            out_data[i-m_start] = shape[i];
        }
    }
    else{
        float* out_data = out.cpu<float>();
        for(int i=m_start; i<m_stop; ++i){
            out_data[i-m_start] = (float)(shape[i]);
        }
    }

    this->m_outputs[0] = out;
    return 0;
}

int ShapeOp::runOnGpu(const std::vector<Tensor>& input){
    return this->runOnCpu(input);
}
}
}