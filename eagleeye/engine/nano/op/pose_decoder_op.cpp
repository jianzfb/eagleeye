#include "eagleeye/engine/nano/op/pose_decoder_op.h"
namespace eagleeye{
namespace dataflow{
PoseDecoderOp::PoseDecoderOp(float score_thre)
    :m_score_thre(score_thre){

}
PoseDecoderOp::PoseDecoderOp(const PoseDecoderOp& op)
    :m_score_thre(op.m_score_thre){

}
PoseDecoderOp::~PoseDecoderOp(){

}
int PoseDecoderOp::init(std::map<std::string, std::vector<float>> params){
    return 0;
}
int PoseDecoderOp::runOnCpu(const std::vector<Tensor>& input){
    const Tensor score = input[0];
    const Tensor offset_scale = input[1];
    if(score.type() != EAGLEEYE_FLOAT || offset_scale.type() != EAGLEEYE_FLOAT){
        EAGLEEYE_LOGE("score and offset_scale must be float.");
        return -1;
    }

    Dim score_dims = score.dims();
    if(score_dims.size() != 4){
        EAGLEEYE_LOGE("score dims size must be 4 (NCHW).");
        return -1;
    }

    const float* score_data = score.cpu<float>();
    const float* score_scale_data = offset_scale.cpu<float>();

    float offset_x = score_scale_data[0];
    float offset_y = score_scale_data[1];
    float scale_x = score_scale_data[2];
    float scale_y = score_scale_data[3];

    int batch_size = score_dims[0];
    int num_joints =score_dims[1];

    Dim out_dim = this->m_outputs[0].dims();
    if(out_dim.size() == 0 || out_dim.production() != batch_size*num_joints*2){
        this->m_outputs[0] =             
            Tensor(std::vector<int64_t>{batch_size, num_joints, 2},
                    EAGLEEYE_FLOAT,
                    DataFormat::AUTO,
                    CPU_BUFFER); 
    }

    this->decoderByHeatmap(score, m_score_thre, this->m_outputs[0], offset_x, offset_y, scale_x, scale_y);

    return 0;
}
int PoseDecoderOp::runOnGpu(const std::vector<Tensor>& input){
    return 0;
}

void PoseDecoderOp::decoderByHeatmap(const Tensor& heatmap, float score_thre, Tensor& joints_pres, float x_offset, float y_offset, float x_scale, float y_scale){
    Dim dims = heatmap.dims();
    int64_t batch_size = dims[0];
    int64_t num_joints = dims[1];
    int64_t height = dims[2];
    int64_t width = dims[3];

    float* out_data = joints_pres.cpu<float>();
    std::vector<std::pair<float, int32_t>> vec;
    vec.resize(height*width);

    const float* heatmap_data = heatmap.cpu<float>();
    int64_t offset = dims.count(2,4);
    for(int b_i=0; b_i<batch_size; ++b_i){
        // finding max score
        for(int joint_i=0; joint_i<num_joints; ++joint_i){
            // prepare
            for (int32_t i = 0; i < offset; i++) {
                vec[i] = std::make_pair(heatmap_data[i], i);
            }

            // sort
            std::partial_sort(vec.begin(),
                                vec.begin() + 1,
                                vec.end(),
                                std::greater<std::pair<float, int32_t>>());

            // thre 
            if(vec[0].first > score_thre){
                int32_t px = vec[0].second % width;
                int32_t py = vec[0].second / width;
                out_data[0] = (float)(px);
                out_data[1] = (float)(py);

                if(px > 1 && px < width-1 && py > 1 && py < height-1){
                    // diff = np.array([hm[py][px+1] - hm[py][px-1],
                    //                  hm[py+1][px]-hm[py-1][px]]);
                    float diff_x = heatmap_data[py*width+(px+1)] - heatmap_data[py*width+(px-1)];
                    float diff_y = heatmap_data[(py+1)*width+px] - heatmap_data[(py-1)*width+px];

                    out_data[0] += (diff_x > 0 ? 0.25 : -0.25);
                    out_data[1] += (diff_y > 0 ? 0.25 : -0.25);
                }

                out_data[0] = out_data[0]*x_scale+x_offset;
                out_data[1] = out_data[1]*y_scale+y_offset;
            }

            heatmap_data += offset;
            out_data += 2;
        }
    }
}
} // namespace dataflow
} // namespace eagleeye