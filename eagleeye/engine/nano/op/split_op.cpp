#include "eagleeye/engine/nano/op/split_op.h"
namespace eagleeye{
namespace dataflow{
Split2DOp::Split2DOp(int axis)
    :m_axis(axis), m_num(2){

}
Split2DOp::Split2DOp(const Split2DOp& op)
    :m_axis(op.m_axis), m_num(2){

}
Split2DOp::~Split2DOp(){

}

int Split2DOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

template<typename T>
void split_func(Tensor input, int axis, int num, std::vector<Tensor>& output){
    Dim dim_0 = input.dims();
    int64_t size = 1;
    int64_t row_size = 1;
    for (int i = axis + 1; i < dim_0.size(); i++) {
        row_size *= dim_0[i];
    }
    for (int i = 0; i < axis; i++) {
        size *= dim_0[i];
    }
    
    T* ptr = (T*)input.cpu();
    int64_t in_sum = dim_0[axis]/num * row_size;
    int64_t in_sum_2 = dim_0[axis] * row_size;

    for(int n = 0; n < num; ++n){
        T* out_ptr = (T*)(output[n].cpu());
        T* src_ptr = ptr + n * in_sum;

        for(int i = 0; i < size; ++i){
            memcpy(out_ptr, src_ptr, sizeof(T)*in_sum);
            out_ptr += in_sum;
            src_ptr += in_sum_2;
        }
    }
}

int Split2DOp::runOnCpu(const std::vector<Tensor>& input){
    const Tensor x = input[0];
    if(x.type() != EAGLEEYE_FLOAT && x.type() != EAGLEEYE_INT && x.type() != EAGLEEYE_UCHAR && x.type() != EAGLEEYE_CHAR){
        EAGLEEYE_LOGE("x type only support float/int/uchar.");
        return -1;
    }
    Dim dimx = x.dims();
    
    std::vector<int64_t> needed_shape = dimx.data();
    needed_shape[m_axis] /= 2;
    Dim needed_out_dim(needed_shape);
    Dim out_dim = this->m_outputs[0].dims();
    if(out_dim.production() != needed_out_dim.production()){
        for(int i=0; i<2; ++i){
            this->m_outputs[i] =             
                Tensor(needed_out_dim.data(),
                        x.type(),
                        x.format(),
                        CPU_BUFFER);  
        }            
    }

    if(x.type() == EAGLEEYE_FLOAT){
        split_func<float>(x, m_axis, 2, this->m_outputs);
    }
    else if(x.type() == EAGLEEYE_UCHAR){
        split_func<unsigned char>(x, m_axis, 2, this->m_outputs);
    }
    else if(x.type() == EAGLEEYE_CHAR){
        split_func<char>(x, m_axis, 2, this->m_outputs);
    }
    else if(x.type() == EAGLEEYE_INT){
        split_func<int>(x, m_axis, 2, this->m_outputs);
    }
    return 0;
}

int Split2DOp::runOnGpu(const std::vector<Tensor>& input){
    return 0;
}

} // namespace dataflow
    
} // namespace eagleeye
