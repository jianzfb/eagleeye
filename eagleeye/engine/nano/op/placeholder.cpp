#include "eagleeye/engine/nano/op/placeholder.h"
#include "eagleeye/runtime/gpu/opencl_runtime.h"
#include "eagleeye/common/EagleeyeOpenCL.h"
#include "eagleeye/basic/Math.h"

namespace eagleeye{
namespace dataflow{
Placeholder::Placeholder(std::string name){
    this->m_b = 0;
    this->m_h = 0;
    this->m_w = 0;
    this->m_c = 0;
    this->m_memory_type = CPU_BUFFER;
    this->m_data_format = DataFormat::NHWC;
    this->m_data_type = EAGLEEYE_FLOAT;
    this->m_name = name;
}

Placeholder::~Placeholder(){

}

int Placeholder::init(std::map<std::string, std::vector<float>> params){
    EAGLEEYE_LOGD("Init Placeholder (%s)", this->m_name.c_str());
    std::map<std::string, std::vector<float>> conv_param = params;
    this->m_b = (int)(conv_param["shape"][0]);
    this->m_h = (int)(conv_param["shape"][1]);
    this->m_w = (int)(conv_param["shape"][2]);
    this->m_c = (int)(conv_param["shape"][3]);

    this->m_memory_type = (MemoryType)((int)(conv_param["memory_type"][0]));
    this->m_data_format = (DataFormat)((int)(conv_param["data_format"][0]));
    this->m_data_type = (EagleeyeType)((int)(conv_param["data_type"][0]));

    // CPU_BUFFER, GPU_BUFFER, GPU_IMAGE
    EAGLEEYE_LOGD("params: %d %d %d %d (memory_type %d, data_format %d, data_type %d)",
            this->m_b,this->m_h, this->m_w, this->m_c, 
            int(this->m_memory_type),
            int(this->m_data_format),
            int(this->m_data_type));

    this->m_output_tensors.resize(1);
    return 0;
}

int Placeholder::runOnCpu(std::vector<Tensor>& output, std::vector<Tensor> input){
    output = this->m_output_tensors;
    return 0;
}

int Placeholder::runOnGpu(std::vector<Tensor>& output, std::vector<Tensor> input){
    output = this->m_output_tensors;
    return 0;
}

int Placeholder::update(void* data, int index){
    // data in host
    if(this->m_output_tensors[0].size() == 0){
        this->m_output_tensors[0] = 
                Tensor(std::vector<int64_t>{this->m_b, this->m_h, this->m_w, this->m_c}, 
                this->m_data_type, 
                this->m_data_format, 
                this->m_memory_type);
    }
    
    this->m_output_tensors[0].update(data);
    return 0;
}
} // namespace dataflow
} // namespace eagleeye
