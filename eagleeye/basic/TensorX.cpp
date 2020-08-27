#include "eagleeye/basic/TensorX.h"
#include "eagleeye/common/EagleeyeOpenCL.h"
namespace eagleeye{
TensorX::TensorX(std::vector<int64_t> shape, 
            EagleeyeType data_type, 
            DataFormat data_format,
            MemoryType memory_type,
            std::vector<int64_t> image_shape,
            Aligned aligned)
            :Blob(shape, data_type, memory_type, image_shape, aligned){
    assert(data_format == DataFormat::NHWC || data_format == DataFormat::OIHW);
    assert(data_type == EAGLEEYE_FLOAT || data_type == EAGLEEYE_UCHAR);
    this->m_format = data_format;
    this->m_shape = shape;
    this->m_range.clear();
	for(int i=0; i<this->m_shape.size(); ++i){
		this->m_range.push_back(Range(0, this->m_shape[i]));
	}
}   
TensorX::TensorX()
    :Blob(std::vector<int64_t>{}, EAGLEEYE_FLOAT, CPU_BUFFER,std::vector<int64_t>{}, Aligned(64)){

}

TensorX::~TensorX(){

} 

int64_t TensorX::size() const{
    if(this->m_shape.size() == 0){
		return 0;
	}

	int64_t input_size = 1;
	for(int i=0; i<this->ndim(); ++i){
		input_size *= (this->m_range[i].e - this->m_range[i].s);
	}
	return input_size;
}
int64_t TensorX::ndim() const{
    return this->m_shape.size();
}
int64_t TensorX::dim(int index) const{
    return this->m_shape[index];
}

DataFormat TensorX::format() const{
    return m_format;
}

std::vector<int64_t> TensorX::shape() const{
    return this->m_shape;
}

void TensorX::resizeImage(std::vector<int64_t> image_shape){
    assert(this->m_memory_type == GPU_IMAGE);
#ifdef EAGLEEYE_OPENCL_OPTIMIZATION    
    this->m_gpu_data = std::shared_ptr<OpenCLObject>(
            new OpenCLImage(EAGLEEYE_CL_MEM_READ_WRITE, "t", image_shape[0], image_shape[1], 4, this->m_data_type),
            [](OpenCLObject* arr) { delete arr; });

    this->m_image_shape = image_shape;
#endif
}
}