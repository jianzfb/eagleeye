namespace eagleeye{
namespace dataflow{
template<std::size_t IN>
BlazeDecoderOp<IN>::BlazeDecoderOp(Matrix<float> anchors, int landmark_num){
    this->m_anchors = anchors;
    this->m_landmark_num = landmark_num;
}

template<std::size_t IN>
BlazeDecoderOp<IN>::~BlazeDecoderOp(){

}


template<std::size_t IN>
int BlazeDecoderOp<IN>::init(std::map<std::string, std::vector<float>> params){
    return 0;
}

void _blaze_mat_transpose(const float* src_ptr, int rows, int cols, float* tgt_ptr){

}

template<std::size_t IN>
int BlazeDecoderOp<IN>::runOnCpu(const std::vector<Tensor>& input){
    const int input_num = IN;
    const int half_input_num = input_num / 2;
    // concat location tensors
    Dim dim = input[0].dims();
    // _,anchor_numx(4+14),H,W
    int anchor_num_in_layer = m_anchor_num[0];
    int landmark_num = dim[1]/anchor_num_in_layer - 4;
    int bbox_landmark_size = landmark_num + 4;

    int location_tensor_size = 0;
    for(int i=0; i<half_input_num; ++i){
        location_tensor_size += input[i].numel();
    }
    float* location_data = (float*)(malloc(location_tensor_size * sizeof(float)));

    int score_tensor_size = 0;
    for(int i=half_input_num; i<input_num; ++i){
        score_tensor_size += input[i].numel();
    }
    float* score_data = (float*)(malloc(score_tensor_size * sizeof(float)));

    // fill location 
    float* cur_location_data = location_data;
    for(int i=0; i<half_input_num; ++i){
        const float* data = input[i].cpu<float>();
        Dim data_dim = input[i].dims();
        // anchor,bbox_landmark_size,H,W
        _blaze_mat_transpose(data, m_anchor_num[i]*bbox_landmark_size,data_dim.count(2,4), cur_location_data);
        cur_location_data += input[i].numel();
    }

    // fill socre
    float* cur_score_data = score_data;
    for(int i=half_input_num; i<input_num; ++i){
        const float* data = input[i].cpu<float>();
        Dim data_dim = input[i].dims();
        // anchor,H,W
        _blaze_mat_transpose(data, m_anchor_num[i],data_dim.count(2,4), cur_score_data);
        cur_score_data += input[i].numel();  
    }

    // for(int i=0; i<)


    free(location_data);
    free(score_tensor_size);
    // concat score tensors

    // const Tensor anchor_location = input[0];
    // const Tensor anchor_score = input[1];
    
    // ignore batch 
    // anchor_location: Nx
    // anchor_score: Nx1
    return 0;
}

template<std::size_t IN>
int BlazeDecoderOp<IN>::runOnGpu(const std::vector<Tensor>& input){
    return 0;
}


} // namespace dataflow
} // namespace eagleeye