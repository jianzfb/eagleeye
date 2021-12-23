#include "eagleeye/engine/nano/op/placeholder_op.h"
#include "eagleeye/runtime/gpu/opencl_runtime.h"
#include "eagleeye/common/EagleeyeOpenCL.h"
#include "eagleeye/basic/Math.h"

namespace eagleeye{
namespace dataflow{
PlaceholderOp::PlaceholderOp(){
    this->m_b = 0;
    this->m_h = 0;
    this->m_w = 0;
    this->m_c = 0;
    this->m_memory_type = CPU_BUFFER;       
    this->m_data_format = DataFormat::NCHW;
    this->m_data_type = EAGLEEYE_FLOAT;
}
PlaceholderOp::PlaceholderOp(int64_t b, 
                                int64_t h, 
                                int64_t w, 
                                int64_t c, 
                                DataFormat format, 
                                EagleeyeType type, 
                                MemoryType memory_type)
    :m_b(b),m_h(h),m_w(w),m_c(c),
    m_memory_type(memory_type),
    m_data_format(format),
    m_data_type(type){
}

PlaceholderOp::PlaceholderOp(const PlaceholderOp& op)
    :m_b(op.m_b),
    m_h(op.m_h),
    m_w(op.m_w),
    m_c(op.m_c),
    m_memory_type(op.m_memory_type),
    m_data_format(op.m_data_format),
    m_data_type(op.m_data_type){
}

PlaceholderOp::~PlaceholderOp(){

}

int PlaceholderOp::init(std::map<std::string, std::vector<float>> params){
    // only support CPU_BUFFER
    this->m_memory_type = (MemoryType)((int)(params["memory_type"][0]));
    this->m_memory_type = CPU_BUFFER;  

    this->m_data_format = (DataFormat)((int)(params["data_format"][0]));
    this->m_data_type = (EagleeyeType)((int)(params["data_type"][0]));
    if(this->m_data_format != DataFormat::NCHW && this->m_data_format != DataFormat::NHWC && this->m_data_format != DataFormat::NC){
        EAGLEEYE_LOGE("Dont support tensor format");
        return -1;
    }
    if(this->m_data_format == DataFormat::NCHW){
        this->m_b = (int)(params["shape"][0]);
        this->m_c = (int)(params["shape"][1]);
        this->m_h = (int)(params["shape"][2]);
        this->m_w = (int)(params["shape"][3]);
    }
    else if(this->m_data_format == DataFormat::NHWC){
        this->m_b = (int)(params["shape"][0]);
        this->m_h = (int)(params["shape"][1]);
        this->m_w = (int)(params["shape"][2]);
        this->m_c = (int)(params["shape"][3]);
    }
    else{
        this->m_b = (int)(params["shape"][0]);
        this->m_c = (int)(params["shape"][1]);
    }

    // CPU_BUFFER, GPU_BUFFER, GPU_IMAGE
    return 0;
}

int PlaceholderOp::runOnCpu(std::vector<Tensor> input){
    // do nothing
    return 0;
}

int PlaceholderOp::runOnGpu(std::vector<Tensor> input){
    // do nothing
    return 0;
}

int PlaceholderOp::update(void* data, std::vector<int64_t> shape, int index){
    // ignore index
    switch (this->m_data_format)
    {
    case DataFormat::NCHW:
        if(this->m_outputs[0].empty() || m_b != shape[0] || m_c != shape[1] || m_h != shape[2] || m_w != shape[3]){
            this->m_b = shape[0];
            this->m_c = shape[1]; 
            this->m_h = shape[2]; 
            this->m_w = shape[3];
            this->m_outputs[0] = Tensor(std::vector<int64_t>{this->m_b, this->m_c, this->m_h, this->m_w}, 
                    this->m_data_type, 
                    this->m_data_format, 
                    this->m_memory_type);
        }

        this->m_outputs[0].update(data);
        break;
    case DataFormat::NHWC:
        if(this->m_outputs[0].empty() || m_b != shape[0] || m_h != shape[1] || m_w != shape[2] || m_c != shape[3]){
            this->m_b = shape[0];
            this->m_h = shape[1]; 
            this->m_w = shape[2];
            this->m_c = shape[3];             
            this->m_outputs[0] = Tensor(std::vector<int64_t>{this->m_b, this->m_h, this->m_w, this->m_c}, 
                    this->m_data_type, 
                    this->m_data_format, 
                    this->m_memory_type);
        }
        this->m_outputs[0].update(data);    
        break;
    case DataFormat::NC:
        if(this->m_outputs[0].empty() || m_b != shape[0] || m_c != shape[1]){
            this->m_b = shape[0];
            this->m_c = shape[1]; 
            this->m_outputs[0] = 
                    Tensor(std::vector<int64_t>{this->m_b, this->m_c}, 
                    this->m_data_type, 
                    this->m_data_format, 
                    this->m_memory_type);     
        }
        this->m_outputs[0].update(data);           
        break;
    default:
        EAGLEEYE_LOGE("Not support");
        break;
    }
    return 0;
}
} // namespace dataflow
} // namespace eagleeye
