#include "eagleeye/engine/nano/op/topk_op.h"
#include "eagleeye/basic/Dim.h"

namespace eagleeye{
namespace dataflow{
TopKOp::TopKOp(int axis, int k){
    m_axis = axis;
    m_k = k;
    OP_SUPPORT(CPU);
}
TopKOp::TopKOp(const TopKOp& op)
    :m_axis(op.m_axis), m_k(op.m_k){
    OP_SUPPORT(CPU);
}

TopKOp::~TopKOp(){

}

int TopKOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

bool _topk_comp_func(std::pair<float, int> a, std::pair<float, int> b) {
  return (a.first > b.first);
}

int TopKOp::runOnCpu(const std::vector<Tensor>& input){
    const Tensor x = input[0];
    // 合法性判断
    if(x.type() != EAGLEEYE_FLOAT){
        EAGLEEYE_LOGE("x type only support float.");
        return -1;
    }

    Dim x_dims = x.dims();
    float* x_data = (float*)x.cpu();
    int dim_size = x_dims.size();
    // int k = param.K;
    if (m_axis < 0) {
        m_axis += dim_size;
    }

    int outer_size = x_dims.count(0, m_axis);
    int axis_size = x_dims[m_axis];
    int inner_size = x_dims.count(m_axis + 1, dim_size);
    int sum_size = axis_size * inner_size;
    int out_sum_size = m_k * inner_size;

    Dim out_dim = this->m_outputs[0].dims();
    if(out_dim.size() == 0){
        std::vector<int64_t> out_shape(dim_size);
        for(int i=0; i<dim_size; ++i){
            if(i != m_axis){
                out_shape[i] = x_dims[i];
            }
            else{
                out_shape[i] = m_k;
            }
        }

        Dim needed_out_dim(out_shape);
        this->m_outputs[0] =             
            Tensor(needed_out_dim.data(),
                    x.type(),
                    DataFormat::AUTO,
                    CPU_BUFFER); 
        
        this->m_outputs[1] =             
            Tensor(needed_out_dim.data(),
                    EAGLEEYE_INT,
                    DataFormat::AUTO,
                    CPU_BUFFER); 
    }

    float* out_val= (float*)this->m_outputs[0].cpu();
    int* out_ind = (int*)this->m_outputs[1].cpu();
    for (int i = 0; i < outer_size; i++) {
        for (int tmp_j = 0; tmp_j < inner_size; tmp_j++) {
            // we need sort outer_size * inner_size times
            // and every times we need sort `axis_size` float

            // we should start from here and pick
            // `axis_size` float strided by inner_size
            int glb_in_off = i * sum_size + tmp_j;
            std::vector<std::pair<float, int>> vec;
            for (int j = 0; j < axis_size; j++) {
                vec.push_back(std::make_pair(x_data[glb_in_off + j * inner_size], j));
            }
            std::partial_sort(vec.begin(), vec.begin() + m_k, vec.end(), _topk_comp_func);

            // we should start from here and put
            // `k` float from here  strided by inner_size
            int glb_out_off = i * out_sum_size + tmp_j;

            for (int j = 0; j < m_k; j++) {
                out_val[glb_out_off + j * inner_size] = vec[j].first;
                out_ind[glb_out_off + j * inner_size] = vec[j].second;
            }
        }
    }    
    return 0;
}

int TopKOp::runOnGpu(const std::vector<Tensor>& input){
    return 0;
}
} // namespace dataflow    
} // namespace eagleeye
