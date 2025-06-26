#include "eagleeye/basic/Tensor.h"
#include "eagleeye/common/EagleeyeOpenCL.h"
namespace eagleeye{
Tensor::Tensor(
    const std::vector<int64_t> shape, 
    EagleeyeType data_type, 
    DataFormat data_format,
    MemoryType memory_type,
    Aligned aligned,
    std::vector<int64_t> image_shape)
        :Blob(shape, data_type, memory_type, image_shape, aligned){
    assert(data_type == EAGLEEYE_DOUBLE || data_type == EAGLEEYE_FLOAT || data_type == EAGLEEYE_CHAR || data_type == EAGLEEYE_BOOL ||data_type == EAGLEEYE_UCHAR || data_type == EAGLEEYE_INT);
    
    this->m_format = data_format;
}  

Tensor::Tensor(
    const std::vector<int64_t> shape, 
    EagleeyeType data_type, 
    DataFormat data_format, 
    void* data, bool copy, bool manage)
        :Blob(shape, data_type, CPU_BUFFER, std::vector<int64_t>(), Aligned(64), data, copy, manage){
    assert(data_type == EAGLEEYE_DOUBLE || data_type == EAGLEEYE_FLOAT || data_type == EAGLEEYE_CHAR || data_type == EAGLEEYE_BOOL || data_type == EAGLEEYE_UCHAR || data_type == EAGLEEYE_INT);
    
    this->m_format = data_format;
}

Tensor::Tensor()
    :Blob(std::vector<int64_t>{}, EAGLEEYE_UCHAR, CPU_BUFFER, std::vector<int64_t>{}){

}

Tensor::~Tensor(){

} 

DataFormat Tensor::format() const{
    return m_format;
}

Tensor Tensor::slice(int64_t begin, int64_t end) const{
    if(begin < 0 || end <= begin){
        return Tensor();
    }

    Dim dst_dims = this->dims();
    dst_dims[0] = end - begin;
    Tensor dst = *this;
    dst.m_dims = dst_dims;
    dst.m_offset += begin * dst_dims.count(1, dst_dims.size()) * this->m_elem_size;

    return dst;
}

Tensor Tensor::squeeze(int64_t axis){
    if(axis == -1){
        axis = this->m_dims.size() - 1;
    }
    if(axis < 0 || axis >= this->m_dims.size()){
        return *this;
    }

    if(this->m_dims[axis] != 1){
        return *this;
    }

    std::vector<int64_t> shape(this->m_dims.size()-1);
    int j = 0;
    for(int i=0; i<this->m_dims.size(); ++i){
        if(i != axis){
            shape[j++] = this->m_dims[i];
        }
    }
    Tensor tmp = *this;
    tmp.m_dims.ConstructFrom(shape);
    return tmp;
}
Tensor Tensor::unsqueeze(int64_t axis){
    if(axis == -1){
        axis = this->m_dims.size();
    }
    if(axis < 0 || axis > this->m_dims.size()){
        return *this;
    }

    std::vector<int64_t> shape(this->m_dims.size()+1);
    int j = 0;
    for(int i=0; i<this->m_dims.size()+1; ++i){
        if(i != axis){
            shape[i] = this->m_dims[j++];
        }
        else{
            shape[i] = 1;
        }
    }

    Tensor tmp = *this;
    tmp.m_dims.ConstructFrom(shape);
    return tmp;
}


Tensor Tensor::clone(){
    if(m_memory_type == GPU_IMAGE){
        EAGLEEYE_LOGE("Dont support clone on GPU_IMAGE Tensor");
        return Tensor();
    }
    
    // TODO, clone 仅对非slice模式有效，需要修改
    Tensor t(this->m_dims.data(), m_data_type, m_format, m_memory_type, m_aligned, m_image_shape);
    if(m_memory_type == CPU_BUFFER){
        // CPU_BUFFER
        memcpy(t.cpu(true), this->cpu(true), this->m_elem_size*this->m_dims.production());      
    }
    else{
        // GPU_BUFFER
#ifdef EAGLEEYE_OPENCL_OPTIMIZATION    
        OpenCLMem* ocl_mem = (OpenCLMem*)(this->gpu());
        OpenCLMem* t_ocl_mem = (OpenCLMem*)(t.gpu());

        t_ocl_mem->copyToDeviceFromDevice(ocl_mem->getObject(), true, this->m_offset, 0, this->m_elem_size*this->m_dims.production());
#endif        
    }

    return t;  
}

void Tensor::copy(const Tensor& t){
    if(t.m_memory_type == GPU_IMAGE || this->m_memory_type == GPU_IMAGE){
        EAGLEEYE_LOGE("Dont support copy on GPU_IMAGE Tensor");
        return;
    }

    int elem_num = this->numel();
    if(elem_num != t.numel() || this->type() != t.type()){
        EAGLEEYE_LOGE("Tensor not consistent");
        return;
    }
    if(this->m_memory_type == CPU_BUFFER){
        memcpy(this->cpu(true), t.cpu(true), this->m_elem_size*elem_num);
    }
    else{
#ifdef EAGLEEYE_OPENCL_OPTIMIZATION   
        OpenCLMem* ocl_mem = (OpenCLMem*)(this->gpu());
        if(t.m_memory_type == CPU_BUFFER){
            ocl_mem->copyToDevice(t.cpu(true), true, 0, this->m_offset, this->m_elem_size*elem_num);
        }
        else{
            OpenCLMem* t_ocl_mem = (OpenCLMem*)(t.gpu());
            ocl_mem->copyToDeviceFromDevice(t_ocl_mem->getObject(), true, t.m_offset, this->m_offset, this->m_elem_size*elem_num);
        }
#endif
    }
}
}