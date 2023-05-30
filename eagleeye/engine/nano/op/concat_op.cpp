#include "eagleeye/engine/nano/op/concat_op.h"
namespace eagleeye{
namespace dataflow{
ConcatOp::ConcatOp(int axis){
    m_aixs = axis;
}

ConcatOp::ConcatOp(const ConcatOp& op)
  :m_aixs(op.m_aixs){
}

ConcatOp::~ConcatOp(){

}

int ConcatOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

template <typename T>
void concat_func(const std::vector<Tensor>& input, const int axis, Tensor output) {
  size_t num = input.size();
  auto dim_0 = input[0].dims();

  int64_t concat_input_size = 1;
  int64_t num_cancats = 1;
  for (int i = axis + 1; i < dim_0.size(); i++) {
    concat_input_size *= dim_0[i];
  }
  for (int i = 0; i < axis; i++) {
    num_cancats *= dim_0[i];
  }

//   auto* dst_ptr = output->mutable_data<T>();
  T* dst_ptr = (T*)output.cpu();
  const int out_concat_axis = output.dims()[axis];
  int64_t offset_concat_axis = 0;
  int64_t out_sum = out_concat_axis * concat_input_size;
  for (int n = 0; n < num; n++) {
    auto dims = input[n].dims();
    T* src_ptr = (T*)input[n].cpu();;
    int64_t in_concat_axis = dims[axis];
    auto* dout_ptr = dst_ptr + offset_concat_axis * concat_input_size;
    int64_t in_sum = in_concat_axis * concat_input_size;
    for (int i = 0; i < num_cancats; i++) {
      memcpy(dout_ptr, src_ptr, sizeof(T) * in_sum);
      dout_ptr += out_sum;
      src_ptr += in_sum;
    }
    offset_concat_axis += in_concat_axis;
  }
}

int ConcatOp::runOnCpu(const std::vector<Tensor>& input){
    int num = input.size();
    if(num != 2){
      EAGLEEYE_LOGE("ConcatOp only support 2 input.");
      return -1;
    }
    if(input[0].type() != EAGLEEYE_FLOAT && input[0].type() != EAGLEEYE_UCHAR && input[0].type() != EAGLEEYE_INT){
      EAGLEEYE_LOGE("ConcatOp dont support type %d", ((int)(input[0].type())));
      return -1;
    }
    if(input[0].dims().size() == 0 || input[0].dims()[0] == 0){
      return -1;
    }

    Dim dim0 = input[0].dims();
    Dim dim1 = input[1].dims();
    for(int i=0; i<dim0.size(); ++i){
      if(i != m_aixs){
        if(dim0[i] != dim1[i]){
          EAGLEEYE_LOGE("dim0 and dim1 not same in axis %d", i);
          return -1;
        }
      }
    }

    int aixs = m_aixs;
    if(aixs == -1){
        aixs = dim0.size() - 1;
    }
    int concat_after_n = 0;
    for(int i=0; i<num; ++i){
        concat_after_n += input[i].dims()[aixs];
    }
    std::vector<int64_t> needed_shape = dim0.data();
    needed_shape[aixs] = concat_after_n;
    Dim needed_out_dim(needed_shape);
    Dim out_dim = this->m_outputs[0].dims();
    if(out_dim.production() != needed_out_dim.production()){
        this->m_outputs[0] =             
            Tensor(needed_out_dim.data(),
                    input[0].type(),
                    input[0].format(),
                    CPU_BUFFER);              
    }

    if(input[0].type() == EAGLEEYE_FLOAT){
        concat_func<float>(input, aixs, this->m_outputs[0]);
    }
    else if(input[0].type() == EAGLEEYE_UCHAR){
        concat_func<unsigned char>(input, aixs, this->m_outputs[0]);
    }
    else if(input[0].type() == EAGLEEYE_INT){
        concat_func<int>(input, aixs, this->m_outputs[0]);
    }
    return 0;
}

int ConcatOp::runOnGpu(const std::vector<Tensor>& input){
    return -1;
}
}   
}