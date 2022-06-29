#include "eagleeye/engine/nano/op/select_by_thres_op.h"
#include <vector>

namespace eagleeye{
namespace dataflow{
SelectByThresOp::SelectByThresOp(std::vector<float> thres, int64_t index, bool attach_index, bool exclusive)
    :m_thres(thres),
     m_index(index),
     m_attach_index(attach_index),
     m_exclusive(exclusive){

}

SelectByThresOp::SelectByThresOp(const SelectByThresOp& op)
    :m_thres(op.m_thres),
     m_index(op.m_index),
     m_attach_index(op.m_attach_index),
     m_exclusive(op.m_exclusive){

}

SelectByThresOp::~SelectByThresOp(){

}

int SelectByThresOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}
int SelectByThresOp::runOnCpu(const std::vector<Tensor>& input){
    const Tensor x = input[0];
    if(x.type() != EAGLEEYE_FLOAT){
        EAGLEEYE_LOGE("x type only support float.");
        return -1;
    }

    Dim x_dims = x.dims();
    if(x_dims.size() != 3){
        EAGLEEYE_LOGE("x size only support 3 (B,N,C).");
        return -1;
    }
    if(m_index < 0 || m_index >= x_dims[x_dims.size()-1]){
        EAGLEEYE_LOGE("m_index not accepted.");
        return -1;
    }

    const float* x_data = x.cpu<float>();
    int outer_size = x_dims.count(0,2); // BN
    int inner_size = x_dims[2];         // C

    if(this->m_thres.size() == 1){
        this->m_thres.reserve(x_dims[0]);
        for(int i=1; i<x_dims[0]; ++i){
            this->m_thres[i] = this->m_thres[0];
        }
    }

    std::vector<std::vector<int>> record;
    record.resize(x_dims[0]);
    // 发现满足阈值条件索引
    for(int i=0; i<outer_size; ++i){
        int b_i = i/x_dims[1];
        int e_i = i%x_dims[1];
        if(x_data[i*x_dims[2]+m_index] >= m_thres[b_i]){
            record[b_i].push_back(e_i);
        }
    }

    // 创建输出tensor
    int out_b = 
        std::accumulate(record.begin(), 
                        record.end(), 
                        0, 
                        [](size_t num, std::vector<int>& b){return num+b.size();});   
    if(out_b == 0){
        return -1;
    }

    int out_c = 0;
    if((m_exclusive & m_attach_index) || (!m_exclusive & !m_attach_index)){
        // 不变
        out_c = x_dims[2];
    }
    else if((!m_exclusive & m_attach_index)){
        // 增加
        out_c = x_dims[2] + 1;
    }   
    else{
        // 减少
        out_c = x_dims[2] - 1;
    }

    this->m_outputs[0] =             
            Tensor(std::vector<int64_t>{out_b, out_c},
                    x.type(),
                    DataFormat::AUTO,
                    CPU_BUFFER); 
    float* out_data = this->m_outputs[0].cpu<float>();
    Dim out_dim = this->m_outputs[0].dims();

    this->m_outputs[1] = 
            Tensor(std::vector<int64_t>{x_dims[0]+1},
            EAGLEEYE_INT,
            DataFormat::AUTO,
            CPU_BUFFER);
    int32_t* out_index_offset = this->m_outputs[1].cpu<int32_t>();

    // 赋值索引偏移
    memset(out_index_offset, 0, sizeof(int32_t)*(x_dims[0]+1));
    for(int b_i=0; b_i<x_dims[0]; ++b_i){
        out_index_offset[b_i+1] = out_index_offset[b_i]+record[b_i].size();
    }

    // 赋值数据
    int b_offset = 0;
    int c_offset = m_attach_index?1:0;
    int x_b_offset = 0;
    int x_stride = x_dims.count(1,3);
    for(int b_i=0; b_i<x_dims[0]; ++b_i){
        for(int e_i=0; e_i<record[b_i].size(); ++e_i){
            if(m_attach_index){
                out_data[b_offset+e_i*out_dim[1]] = b_i;
            }

            if(m_exclusive){
                int count = 0;
                for(int i=0; i<x_dims[2]; ++i){
                    if(i != m_index){
                        *(out_data+b_offset+e_i*out_dim[1] + count++) = *(x_data+x_b_offset+record[b_i][e_i]*x_dims[2] + i);
                    }
                }
            }
            else{
                memcpy(out_data+b_offset+e_i*out_dim[1]+c_offset, x_data+x_b_offset+record[b_i][e_i]*x_dims[2], sizeof(float)*x_dims[2]);
            }
        }

        b_offset += record[b_i].size()*out_dim[1];
        x_b_offset += x_stride;
    }
    return 0;
}
int SelectByThresOp::runOnGpu(const std::vector<Tensor>& input){
    return 0;
}

} // namespace dataflow
} // namespace eagleeye
