#include "eagleeye/engine/nano/op/gather_nd_op.h"

namespace eagleeye{
namespace dataflow{
GatherNdOp::GatherNdOp(const GatherNdOp& op){
    OP_SUPPORT(CPU);
}

int GatherNdOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

template <typename DataT, typename IndexT = int32_t>
void GatherNd(const Tensor& x, const Tensor& index, Tensor& out) {
  auto index_dims = index.dims();
  auto index_dims_size = index_dims.size();
  auto x_dims = x.dims();
  auto x_dims_size = x_dims.size();

//   const DataT* x_data = x.data<DataT>();
  const DataT* x_data = (DataT*)x.cpu();
//   const IndexT* index_data = index.data<IndexT>();
  const IndexT* index_data = (IndexT*)index.cpu();
//   DataT* out_data = out->template mutable_data<DataT>();
  DataT* out_data = (DataT*)out.cpu();

  int64_t gather_time = 1;
  for (size_t i = 0; i < index_dims_size - 1; i++) {
    gather_time *= index_dims[i];
  }

  int64_t end_size = index_dims[index_dims_size - 1];
  int64_t gather_size = 1;
  for (size_t i = end_size; i < x_dims_size; i++) {
    gather_size *= x_dims[i];
  }
  const size_t gather_bytes = gather_size * sizeof(DataT);

  for (int64_t i = 0; i < gather_time; i++) {
    int64_t x_index = 0;
    int64_t step = 1;
    for (int64_t j = end_size - 1; j >= 0; j--) {
      x_index += (index_data[i * end_size + j] * step);
      step *= x_dims[j];
    }
    memcpy(out_data, x_data + x_index * gather_size, gather_bytes);
    out_data += gather_size;
  }
  return;
}

int GatherNdOp::runOnCpu(const std::vector<Tensor>& input){
    const Tensor x = input[0];
    const Tensor index = input[1];
    if(x.dims().size() == 0 || x.dims()[0] == 0){
      return -1;
    }

    Dim out_dim = this->m_outputs[0].dims();
    if(out_dim.size() == 0){
        Dim x_dims = x.dims();
        Dim index_dims = index.dims();
        Dim needed_out_dim =  x_dims.Slice(index_dims[1], x_dims.size());
        this->m_outputs[0] = \
                Tensor(needed_out_dim.data(),
                        x.type(),
                        DataFormat::AUTO,
                        CPU_BUFFER);         
    }

    switch (x.type())
    {
    case EAGLEEYE_FLOAT:
        GatherNd<float>(x, index, this->m_outputs[0]);  
        break;
    case EAGLEEYE_UCHAR:
    case EAGLEEYE_CHAR:
        GatherNd<char>(x, index, this->m_outputs[0]);  
        break;
    case EAGLEEYE_INT:
        GatherNd<int32_t>(x, index, this->m_outputs[0]);  
        break;
    default:
        EAGLEEYE_LOGE("unsupport data type: : %d", static_cast<int>(x.type()));
        break;
    }
    return 0;
}

int GatherNdOp::runOnGpu(const std::vector<Tensor>& input){
    return 0;
}
} // namespace dataflow
} // namespace eagleeye
