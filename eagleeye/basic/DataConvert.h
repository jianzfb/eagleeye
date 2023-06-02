#include "defines.h"
#include "eagleeye/basic/Tensor.h"

namespace eagleeye{
// CTensor -> Tensor
Tensor convert_cftensor_tensor(CFTensor* t){
    std::vector<int64_t> shape;
    for(int i=0; i<t->dim_size; ++i){
        shape.push_back(t->dims[i]);
    }
    Tensor data(shape, EAGLEEYE_FLOAT, DataFormat::AUTO, CPU_BUFFER);
    memcpy(data.cpu(), t->data, data.blobsize());
    return data;
}

Tensor convert_citensor_tensor(CITensor* t){
    std::vector<int64_t> shape;
    for(int i=0; i<t->dim_size; ++i){
        shape.push_back(t->dims[i]);
    }
    Tensor data(shape, EAGLEEYE_INT, DataFormat::AUTO, CPU_BUFFER);
    memcpy(data.cpu(), t->data, data.blobsize());
    return data;
}

Tensor convert_cuctensor_tensor(CUCTensor* t){
    std::vector<int64_t> shape;
    for(int i=0; i<t->dim_size; ++i){
        shape.push_back(t->dims[i]);
    }
    Tensor data(shape, EAGLEEYE_UCHAR, DataFormat::AUTO, CPU_BUFFER);
    memcpy(data.cpu(), t->data, data.blobsize());
    return data;
}

// Tensor -> CTensor
CFTensor* convert_tensor_cftensor(Tensor t){
    CFTensor* cftensor = new CFTensor();
    cftensor->dim_size = t.dims().size();
    cftensor->dims = new size_t[t.dims().size()];
    cftensor->data = new float[t.dims().production()];
    cftensor->is_assign_inner = true;

    memcpy(cftensor->data, t.cpu(), t.blobsize());
    memcpy(cftensor->dims, &t.dims().data()[0], sizeof(size_t)*t.dims().size());
    return cftensor;
}

CITensor* convert_tensor_citensor(Tensor t){
    CITensor* citensor = new CITensor();
    citensor->dim_size = t.dims().size();
    citensor->dims = new size_t[t.dims().size()];
    citensor->data = new int[t.dims().production()];
    citensor->is_assign_inner = true;

    memcpy(citensor->data, t.cpu(), t.blobsize());
    memcpy(citensor->dims, &t.dims().data()[0], sizeof(size_t)*t.dims().size());
    return citensor;
}

CUCTensor* convert_tensor_cuctensor(Tensor t){
    CUCTensor* cuctensor = new CUCTensor();
    cuctensor->dim_size = t.dims().size();
    cuctensor->dims = new size_t[t.dims().size()];
    cuctensor->data = new unsigned char[t.dims().production()];
    cuctensor->is_assign_inner = true;

    memcpy(cuctensor->data, t.cpu(), t.blobsize());
    memcpy(cuctensor->dims, &t.dims().data()[0], sizeof(size_t)*t.dims().size());
    return cuctensor;
}

// vector -> CTensor
CFTensor* init_cftensor(std::vector<float> data, std::vector<size_t> shape){
    CFTensor* cftensor = new CFTensor();
    cftensor->dim_size = shape.size();
    cftensor->dims = new size_t[shape.size()];
    size_t num = 1;
    for(int i=0; i<shape.size(); ++i){
        num *= shape[i];
        cftensor->dims[i] = shape[i];
    }
    cftensor->data = new float[num];
    memcpy(cftensor->data, &data[0], sizeof(float)*num);
    cftensor->is_assign_inner = true;    
    return cftensor;
}

CITensor* init_citensor(std::vector<int> data, std::vector<size_t> shape){
    CITensor* citensor = new CITensor();
    citensor->dim_size = shape.size();
    citensor->dims = new size_t[shape.size()];
    size_t num = 1;
    for(int i=0; i<shape.size(); ++i){
        num *= shape[i];
        citensor->dims[i] = shape[i];
    }
    citensor->data = new int[num];
    citensor->is_assign_inner = true;

    memcpy(citensor->data, &data[0], sizeof(int)*num);
    return citensor;
}

CUCTensor* init_cuctensor(std::vector<unsigned char> data, std::vector<size_t> shape){
    CUCTensor* cuctensor = new CUCTensor();
    cuctensor->dim_size = shape.size();
    cuctensor->dims = new size_t[shape.size()];
    size_t num = 1;
    for(int i=0; i<shape.size(); ++i){
        num *= shape[i];
        cuctensor->dims[i] = shape[i];
    }
    cuctensor->data = new unsigned char[num];
    cuctensor->is_assign_inner = true;

    memcpy(cuctensor->data, &data[0], sizeof(unsigned char)*num);
    return cuctensor;
}

CFTensor* new_cftensor(){
    CFTensor* t = new CFTensor();
    t->is_assign_inner = false;
    return t;
}

CITensor* new_citensor(){
    CITensor* t = new CITensor();
    t->is_assign_inner = false;
    return t;
}

CUCTensor* new_cuctensor(){
    CUCTensor* t = new CUCTensor();
    t->is_assign_inner = false;
    return t;
}
}