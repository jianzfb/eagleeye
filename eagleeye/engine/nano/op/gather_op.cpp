#include "eagleeye/engine/nano/op/gather_op.h"

namespace eagleeye{
namespace dataflow{
GatherOp::GatherOp(const GatherOp& op){
    OP_SUPPORT(CPU);
}

GatherOp::~GatherOp(){

}

int GatherOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

template <typename IndexType, typename DataType>
void GatherFunc(const DataType* p_src, const IndexType* p_index, DataType* p_output, Dim src_dims, Dim index_dims) {
    int index_size = index_dims[0];
    int slice_size = 1;
    for (size_t i = 1; i < src_dims.size(); ++i) {
        slice_size *= src_dims[i];
    }
    for (int i = 0; i < index_size; ++i) {
        IndexType index_ = p_index[i];
        memcpy(p_output + i * slice_size,
            p_src + index_ * slice_size,
            slice_size * sizeof(DataType));
    }
}
int GatherOp::runOnCpu(const std::vector<Tensor>& input){
    const Tensor x = input[0];
    const Tensor index = input[1];
    if(x.dims().size() == 0 || x.dims()[0] == 0){
      return -1;
    }

    Dim out_dim = this->m_outputs[0].dims();
    if(out_dim.size() == 0){
        Dim x_dims = x.dims();
        Dim index_dims = index.dims();
        
        Dim x_dims_c;
        x_dims_c.ConstructFrom(x_dims.data());
        x_dims_c[0] = index_dims[0];
        this->m_outputs[0] = Tensor(x_dims_c.data(),
                    x.type(),
                    DataFormat::AUTO,
                    CPU_BUFFER); 
    }
    switch (x.type()) {
      case EAGLEEYE_FLOAT:
        GatherFunc<int32_t, float>(x.cpu<float>(), index.cpu<int32_t>(), this->m_outputs[0].cpu<float>(), x.dims(), index.dims());
        break;
      case EAGLEEYE_UCHAR:
      case EAGLEEYE_CHAR:
        GatherFunc<int32_t, int8_t>(x.cpu<int8_t>(), index.cpu<int32_t>(), this->m_outputs[0].cpu<int8_t>(), x.dims(), index.dims());
        break;
      case EAGLEEYE_INT:
        GatherFunc<int32_t, int32_t>(x.cpu<int32_t>(), index.cpu<int32_t>(), this->m_outputs[0].cpu<int32_t>(), x.dims(), index.dims());
        break;
      default:
        EAGLEEYE_LOGE("unsupport data type: : %d", static_cast<int>(x.type()));
    }
    return 0;
}

int GatherOp::runOnGpu(const std::vector<Tensor>& input){
    return 0;
}
} // namespace dataflow
} // namespace eagleeye
