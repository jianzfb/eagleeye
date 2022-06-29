#include "eagleeye/engine/nano/op/anchor_free_decoder_op.h"

namespace eagleeye{
namespace dataflow{
AnchorFreeDecoderOp::AnchorFreeDecoderOp(AnchorFreeDecoderType decoder_type, int max_per, float x_scale, float y_scale, float x_scale_ext, float y_scale_ext)
    :m_decoder_type(decoder_type),
     m_max_per(max_per),
     m_x_scale(x_scale),
     m_y_scale(y_scale),
     m_x_scale_ext(x_scale_ext),
     m_y_scale_ext(y_scale_ext){

}
AnchorFreeDecoderOp::AnchorFreeDecoderOp(const AnchorFreeDecoderOp& op)
    :m_decoder_type(op.m_decoder_type),
     m_max_per(op.m_max_per),
     m_x_scale(op.m_x_scale),
     m_y_scale(op.m_y_scale),
     m_x_scale_ext(op.m_x_scale_ext),
     m_y_scale_ext(op.m_y_scale_ext){

}

AnchorFreeDecoderOp::~AnchorFreeDecoderOp(){

}

int AnchorFreeDecoderOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

int AnchorFreeDecoderOp::runOnCpu(const std::vector<Tensor>& input){
    const Tensor score = input[0];      // B,C,H,W (C是类别数)
    const Tensor position = input[1];   // B,4,H,W

    Dim score_dim = score.dims();
    int64_t batch_num = score_dim[0];
    int64_t cls_num = score_dim[1];
    int64_t h = score_dim[2];
    int64_t w = score_dim[3];

    if(score.type() != EAGLEEYE_FLOAT || position.type() != EAGLEEYE_FLOAT){
        EAGLEEYE_LOGE("score and position must be float.");
        return -1;
    }

    // batch_num, category, m_max_per, 5
    Dim needed_out_dim(std::vector<int64_t>{batch_num,cls_num,m_max_per,5});
    if(this->m_outputs[0].dims().size() == 0){
        this->m_outputs[0] =             
            Tensor(needed_out_dim.data(),
                    EAGLEEYE_FLOAT,
                    DataFormat::AUTO,
                    CPU_BUFFER); 
    }

    switch (m_decoder_type)
    {
    case ANCHORFREE_LTRB:
        this->ltrb_decoder(score, position, this->m_outputs[0]);
        break;
    
    default:
        break;
    }
    return 0;
}
int AnchorFreeDecoderOp::runOnGpu(const std::vector<Tensor>& input){
    return 0;
}

bool _af_decoder_topk_comp_func(std::pair<float, int> a, std::pair<float, int> b) {
  return (a.first > b.first);
}
void AnchorFreeDecoderOp::ltrb_decoder(const Tensor& score, const Tensor& ltrb, Tensor& out){
    Dim score_dim = score.dims();
    int64_t batch_num = score_dim[0];
    int64_t cls_num = score_dim[1];
    int64_t h = score_dim[2];
    int64_t w = score_dim[3];

    const float* score_data = score.cpu<float>();
    const float* ltrb_data = ltrb.cpu<float>(); // batch_num,4,h,w
    float* out_data = out.cpu<float>();         // batch_num, category, m_max_per, 5

    int outer_size = score_dim.count(0, 2);
    int axis_size = h*w;

    // 找到每张图每个类下的 分数最大
    float* top_k_score_val = (float*)malloc(sizeof(float)*outer_size*m_max_per);
    int* top_k_score_index = (int*)malloc(sizeof(int)*outer_size*m_max_per);

    for(int i = 0; i < outer_size; ++i){
        int glb_in_off = i * axis_size;
        std::vector<std::pair<float, int>> vec;
        for (int j = 0; j < axis_size; j++) {
            vec.push_back(std::make_pair(score_data[glb_in_off + j], j));
        }
        std::partial_sort(vec.begin(), vec.begin() + m_max_per, vec.end(), _af_decoder_topk_comp_func);

        int glb_out_off = i * m_max_per;
        for (int j = 0; j < m_max_per; j++) {
            top_k_score_val[glb_out_off + j] = vec[j].first;
            top_k_score_index[glb_out_off + j] = vec[j].second;
        }
    }

    // 计算输出
    for(int i=0; i<outer_size; ++i){
        int b_i = i/cls_num;
        int glb_out_off = i*m_max_per*5;
        int glb_top_k_off = i*m_max_per;
        for(int j=0; j<m_max_per; ++j){
            int index = top_k_score_index[glb_top_k_off+j];
            int x_index = index % w;
            int y_index = index / w;

            out_data[glb_out_off+j*5] = top_k_score_val[glb_top_k_off+j]; // probability
            out_data[glb_out_off+j*5 + 1] = (x_index * m_x_scale - ltrb_data[b_i*4*h*w + index]) * m_x_scale_ext;         // x0
            out_data[glb_out_off+j*5 + 2] = (y_index * m_y_scale - ltrb_data[b_i*4*h*w + h*w + index]) * m_y_scale_ext;   // y0
            out_data[glb_out_off+j*5 + 3] = (x_index * m_x_scale + ltrb_data[b_i*4*h*w + 2*h*w+index]) * m_x_scale_ext;   // x1
            out_data[glb_out_off+j*5 + 4] = (y_index * m_y_scale + ltrb_data[b_i*4*h*w + 3*h*w+index]) * m_y_scale_ext;   // y1
        }
    }

    free(top_k_score_val);
    free(top_k_score_index);
}
} // namespace dataflow
} // namespace eagleeye