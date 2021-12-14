#include "eagleeye/basic/Tensor.h"
#include "eagleeye/common/EagleeyeOpenCL.h"
namespace eagleeye{
Tensor::Tensor(std::vector<int64_t> shape, 
            EagleeyeType data_type, 
            DataFormat data_format,
            MemoryType memory_type,
            std::vector<int64_t> image_shape,
            Aligned aligned)
            :Blob(shape, data_type, memory_type, image_shape, aligned){
    assert(data_type == EAGLEEYE_FLOAT || data_type == EAGLEEYE_CHAR || data_type == EAGLEEYE_UCHAR || data_type == EAGLEEYE_INT);
    this->m_format = data_format;
    this->m_shape = shape;
    this->m_range.clear();
	for(int i=0; i<this->m_shape.size(); ++i){
		this->m_range.push_back(Range(0, this->m_shape[i]));
	}
}  

Tensor::Tensor(std::vector<int64_t> shape, 
            EagleeyeType data_type, 
            DataFormat data_format, 
            void* data)
        :Blob(shape, data_type, CPU_BUFFER, std::vector<int64_t>(), Aligned(64), data){
    assert(data_type == EAGLEEYE_FLOAT || data_type == EAGLEEYE_CHAR || data_type == EAGLEEYE_UCHAR || data_type == EAGLEEYE_INT);
    
    this->m_format = data_format;
    this->m_shape = shape;
    this->m_range.clear();
	for(int i=0; i<this->m_shape.size(); ++i){
		this->m_range.push_back(Range(0, this->m_shape[i]));
	}
}


Tensor::Tensor()
    :Blob(std::vector<int64_t>{0}, EAGLEEYE_UCHAR, CPU_BUFFER, std::vector<int64_t>{}){

}

Tensor::~Tensor(){

} 

DataFormat Tensor::format() const{
    return m_format;
}

void Tensor::setFormat(DataFormat data_format){
    m_format = data_format;
}

Tensor Tensor::clone() const{
    // 
    if(m_runtime.type() != EAGLEEYE_CPU){
        EAGLEEYE_LOGE("Dont support clone on non-cpu device.");
        return Tensor();
    }

    Tensor t(m_shape, m_data_type, m_format, m_memory_type, m_image_shape, m_aligned);
    memcpy(t.cpu(), this->cpu(), m_size);
    return t;
}

void Tensor::copy(const Tensor& t){
    if(m_runtime.type() != t.getRuntimeType() || m_runtime.type() != EAGLEEYE_CPU){
        EAGLEEYE_LOGE("Dont support clone on non-cpu device.");
        return;
    }

    if(this->numel() != t.numel() || this->type() != t.type()){
        EAGLEEYE_LOGE("Tensor not consistent");
        return;
    }

    memcpy(this->cpu(), t.cpu(), this->blobsize());
}
}