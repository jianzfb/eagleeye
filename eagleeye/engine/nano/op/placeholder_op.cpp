#include "eagleeye/engine/nano/op/placeholder_op.h"
#include "eagleeye/runtime/gpu/opencl_runtime.h"
#include "eagleeye/common/EagleeyeOpenCL.h"
#include "eagleeye/engine/nano/util/opencl_util.h"
#include "eagleeye/basic/Math.h"

namespace eagleeye{
namespace dataflow{
PlaceholderOp::PlaceholderOp(){
    this->m_memory_type = CPU_BUFFER;       
    this->m_data_format = DataFormat::AUTO;
    this->m_data_type = EAGLEEYE_FLOAT;
    this->m_zero_copy = true;
}
PlaceholderOp::PlaceholderOp(int64_t b, 
                                int64_t h, 
                                int64_t w, 
                                int64_t c, 
                                DataFormat format, 
                                EagleeyeType type, 
                                MemoryType memory_type)
    :m_memory_type(memory_type),
    m_data_format(format),
    m_data_type(type){
    this->m_zero_copy = true;
}

PlaceholderOp::PlaceholderOp(const PlaceholderOp& op)
    :m_memory_type(op.m_memory_type),
    m_data_format(op.m_data_format),
    m_data_type(op.m_data_type),
    m_zero_copy(op.m_zero_copy){
}

PlaceholderOp::~PlaceholderOp(){
}

int PlaceholderOp::init(std::map<std::string, std::vector<float>> params){
    // only support CPU_BUFFER
    if(params.size() == 0){
        return 0;
    }

    if(params.find("memory_type") != params.end()){
        this->m_memory_type = (MemoryType)((int)(params["memory_type"][0]));
        this->m_memory_type = CPU_BUFFER;
    }

    this->m_data_format = DataFormat::AUTO;
    if(params.find("data_format") != params.end()){
        this->m_data_format = (DataFormat)((int)(params["data_format"][0]));
    }
    if(params.find("data_type") != params.end()){
        this->m_data_type = (EagleeyeType)((int)(params["data_type"][0]));
    }

    if(params.find("zero_copy") != params.end()){
        this->m_zero_copy = bool((int)(params["zero_copy"][0]));
    }
    // CPU_BUFFER, GPU_BUFFER, GPU_IMAGE
    return 0;
}

int PlaceholderOp::runOnCpu(const std::vector<Tensor>& input){
    // do nothing
    return 0;
}

int PlaceholderOp::runOnGpu(const std::vector<Tensor>& input){
    // do nothing
    return 0;
}

int PlaceholderOp::update(void* data, std::vector<int64_t> shape, EagleeyeType type, int index){
    // RGB, RGBA -> 转换到base type
    if(type == EAGLEEYE_RGB || type == EAGLEEYE_RGBA || type == EAGLEEYE_UCHAR3 || type == EAGLEEYE_UCHAR4 || type == EAGLEEYE_UINT8_3 || type == EAGLEEYE_UINT8_4){
        type = EAGLEEYE_UCHAR;
    }
    if(this->m_zero_copy){
        this->m_outputs[0] = Tensor(
            shape,
            type,
            DataFormat::AUTO,
            data
        );
    }
    else{
        if(this->m_outputs[0].dims().production() != Dim(shape).production()){
            this->m_outputs[0] = Tensor(
                shape,
                type,
                DataFormat::AUTO,
                CPU_BUFFER
            );
        }

        this->m_outputs[0].update(data);
    }

    return 0;
}
} // namespace dataflow
} // namespace eagleeye
