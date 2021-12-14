#include "eagleeye/engine/nano/op/slice_op.h"
namespace eagleeye{
namespace dataflow{
SliceOp::SliceOp(std::vector<int> axes, std::vector<int> starts, std::vector<int> ends){
    m_axes = axes;
    m_starts = starts;
    m_ends = ends;
    OP_SUPPORT(CPU);
}   

SliceOp::SliceOp(const SliceOp& op)
  :m_axes(op.m_axes),m_starts(op.m_starts),m_ends(op.m_ends){
  OP_SUPPORT(CPU);
}

SliceOp::~SliceOp(){

}

int SliceOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

template <typename Dtype>
void slice(const Dtype* input,
           std::vector<int64_t> in_dims,
           std::vector<int> axes,
           std::vector<int> starts,
           std::vector<int> ends,
           Dtype* out) {
  auto out_dims = in_dims;
  std::vector<int> real_starts(in_dims.size(), 0);
  std::vector<int> real_ends(in_dims.size(), 0);
  std::vector<int> real_step(in_dims.size(), 0);
  for (int i = 0; i < in_dims.size(); i++) {
    real_ends[i] = in_dims[i];
  }
  for (int i = 0; i < axes.size(); i++) {
    int dim_value = in_dims[axes[i]];
    if (dim_value > 0) {
      int start = starts[i] < 0 ? (starts[i] + dim_value) : starts[i];
      int end = ends[i] < 0 ? (ends[i] + dim_value) : ends[i];
      start = std::max(start, 0);
      end = std::max(end, 0);
      end = std::min(end, dim_value);
      out_dims[axes[i]] = end - start;
      real_starts[axes[i]] = start;
      real_ends[axes[i]] = end;
    }
  }
  const int LEN = in_dims.size();
  int dst_step[LEN];
  for (int i = 0; i < in_dims.size(); ++i) {
    dst_step[i] = 1;
  }
  int src_step[LEN];
  for (int i = 0; i < in_dims.size(); ++i) {
    src_step[i] = 1;
  }
  int out_num = out_dims[in_dims.size() - 1];
  for (int i = in_dims.size() - 2; i >= 0; i--) {
    dst_step[i] = out_dims[i + 1] * dst_step[i + 1];
    src_step[i] = in_dims[i + 1] * src_step[i + 1];
    out_num *= out_dims[i];
  }

  for (int dst_id = 0; dst_id < out_num; dst_id++) {
    int src_id = 0;
    int index_id = dst_id;
    for (int j = 0; j < out_dims.size(); j++) {
      int cur_id = index_id / dst_step[j];
      index_id = index_id % dst_step[j];
      src_id += (cur_id + real_starts[j]) * src_step[j];
    }
    out[dst_id] = input[src_id];
  }
}

int SliceOp::runOnCpu(std::vector<Tensor> input){
    Tensor x = input[0];
    // 合法性判断
    if(x.type() != EAGLEEYE_FLOAT){
        EAGLEEYE_LOGE("x type only support float.");
        return -1;
    }

    Dim x_dim = x.dims(); 
    Dim out_dim = this->m_outputs[0].dims();
    std::vector<int64_t> needed_shape = x_dim.data();
    for(int i=0; i<m_axes.size(); ++i){
        needed_shape[m_axes[i]] = m_ends[i] - m_starts[i];
    }
    Dim needed_out_dim(needed_shape);
    if(out_dim.production() != needed_out_dim.production()){
        this->m_outputs[0] =             
            Tensor(needed_out_dim.data(),
                    x.type(),
                    x.format(),
                    CPU_BUFFER);            
    }

    float* din = (float*)x.cpu();
    float* dout = (float*)this->m_outputs[0].cpu();
    slice<float>(din, x_dim.data(), m_axes, m_starts, m_ends, dout);
    return 0;
}

int SliceOp::runOnGpu(std::vector<Tensor> input){
    return 0;
}
}    
}