#ifndef _EAGLEEYE_ANTGO_DATACONVERT_H_
#define _EAGLEEYE_ANTGO_DATACONVERT_H_
#include "defines.h"
#include "eagleeye/basic/Tensor.h"

using namespace antgo;
namespace eagleeye{
// CTensor -> Tensor
inline Tensor convert_cftensor_tensor(CFTensor* t){
    std::vector<int64_t> shape;
    for(int i=0; i<t->dim_size; ++i){
        shape.push_back(t->dims[i]);
    }
    Tensor data(shape, EAGLEEYE_FLOAT, DataFormat::AUTO, CPU_BUFFER);
    memcpy(data.cpu(), t->data, data.blobsize());
    return data;
}

inline Tensor convert_citensor_tensor(CITensor* t){
    std::vector<int64_t> shape;
    for(int i=0; i<t->dim_size; ++i){
        shape.push_back(t->dims[i]);
    }
    Tensor data(shape, EAGLEEYE_INT, DataFormat::AUTO, CPU_BUFFER);
    memcpy(data.cpu(), t->data, data.blobsize());
    return data;
}

inline Tensor convert_cuctensor_tensor(CUCTensor* t){
    std::vector<int64_t> shape;
    for(int i=0; i<t->dim_size; ++i){
        shape.push_back(t->dims[i]);
    }
    Tensor data(shape, EAGLEEYE_UCHAR, DataFormat::AUTO, CPU_BUFFER);
    memcpy(data.cpu(), t->data, data.blobsize());
    return data;
}

// Tensor -> CTensor
inline CFTensor* convert_tensor_cftensor(Tensor t){
    CFTensor* cftensor = new CFTensor();
    cftensor->dim_size = t.dims().size();
    cftensor->dims = new int64_t[t.dims().size()];
    cftensor->data = new float[t.dims().production()];
    cftensor->is_assign_inner = true;

    memcpy(cftensor->data, t.cpu(), t.blobsize());
    memcpy(cftensor->dims, &t.dims().data()[0], sizeof(int64_t)*t.dims().size());
    return cftensor;
}

inline CITensor* convert_tensor_citensor(Tensor t){
    CITensor* citensor = new CITensor();
    citensor->dim_size = t.dims().size();
    citensor->dims = new int64_t[t.dims().size()];
    citensor->data = new int[t.dims().production()];
    citensor->is_assign_inner = true;

    memcpy(citensor->data, t.cpu(), t.blobsize());
    memcpy(citensor->dims, &t.dims().data()[0], sizeof(int64_t)*t.dims().size());
    return citensor;
}

inline CBTensor* convert_tensor_cbtensor(Tensor t){
    CBTensor* cbtensor = new CBTensor();
    cbtensor->dim_size = t.dims().size();
    cbtensor->dims = new int64_t[t.dims().size()];
    cbtensor->data = new bool[t.dims().production()];
    cbtensor->is_assign_inner = true;

    memcpy(cbtensor->data, t.cpu(), t.blobsize());
    memcpy(cbtensor->dims, &t.dims().data()[0], sizeof(int64_t)*t.dims().size());
    return cbtensor;
}

inline CUCTensor* convert_tensor_cuctensor(Tensor t){
    CUCTensor* cuctensor = new CUCTensor();
    cuctensor->dim_size = t.dims().size();
    cuctensor->dims = new int64_t[t.dims().size()];
    cuctensor->data = new unsigned char[t.dims().production()];
    cuctensor->is_assign_inner = true;

    memcpy(cuctensor->data, t.cpu(), t.blobsize());
    memcpy(cuctensor->dims, &t.dims().data()[0], sizeof(int64_t)*t.dims().size());
    return cuctensor;
}

// vector -> CTensor
inline CFTensor* init_cftensor(std::vector<float> data, std::vector<int64_t> shape){
    CFTensor* cftensor = new CFTensor();
    cftensor->dim_size = shape.size();
    cftensor->dims = new int64_t[shape.size()];
    int64_t num = 1;
    for(int i=0; i<shape.size(); ++i){
        num *= shape[i];
        cftensor->dims[i] = shape[i];
    }
    cftensor->data = new float[num];
    memcpy(cftensor->data, data.data(), sizeof(float)*num);
    cftensor->is_assign_inner = true;    
    return cftensor;
}

inline CITensor* init_citensor(std::vector<int> data, std::vector<int64_t> shape){
    CITensor* citensor = new CITensor();
    citensor->dim_size = shape.size();
    citensor->dims = new int64_t[shape.size()];
    int64_t num = 1;
    for(int i=0; i<shape.size(); ++i){
        num *= shape[i];
        citensor->dims[i] = shape[i];
    }
    citensor->data = new int[num];
    citensor->is_assign_inner = true;

    memcpy(citensor->data, data.data(), sizeof(int)*num);
    return citensor;
}

inline CBTensor* init_cbtensor(std::vector<bool> data, std::vector<int64_t> shape){
    CBTensor* cbtensor = new CBTensor();
    cbtensor->dim_size = shape.size();
    cbtensor->dims = new int64_t[shape.size()];
    int64_t num = 1;
    for(int i=0; i<shape.size(); ++i){
        num *= shape[i];
        cbtensor->dims[i] = shape[i];
    }
    cbtensor->data = new bool[num];
    cbtensor->is_assign_inner = true;

    std::copy(data.begin(), data.end(), cbtensor->data);
    return cbtensor;
}

inline CUCTensor* init_cuctensor(std::vector<unsigned char> data, std::vector<int64_t> shape){
    CUCTensor* cuctensor = new CUCTensor();
    cuctensor->dim_size = shape.size();
    cuctensor->dims = new int64_t[shape.size()];
    int64_t num = 1;
    for(int i=0; i<shape.size(); ++i){
        num *= shape[i];
        cuctensor->dims[i] = shape[i];
    }
    cuctensor->data = new unsigned char[num];
    cuctensor->is_assign_inner = true;

    memcpy(cuctensor->data, data.data(), sizeof(unsigned char)*num);
    return cuctensor;
}

inline CFTensor* new_cftensor(){
    CFTensor* t = new CFTensor();
    t->is_assign_inner = false;
    return t;
}

inline CITensor* new_citensor(){
    CITensor* t = new CITensor();
    t->is_assign_inner = false;
    return t;
}

inline CUCTensor* new_cuctensor(){
    CUCTensor* t = new CUCTensor();
    t->is_assign_inner = false;
    return t;
}

inline CUSTensor* new_custensor(){
    CUSTensor* t = new CUSTensor();
    t->is_assign_inner = false;
    return t;
}

inline CBTensor* new_cbtensor(){
    CBTensor* t = new CBTensor();
    t->is_assign_inner = false;
    return t;
}
}

#endif