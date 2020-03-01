#include "eagleeye/processnode/SkySegNode.h"
#include "eagleeye/basic/MatrixMath.h"
#include "eagleeye/common/EagleeyeRuntime.h"

namespace eagleeye{
SkySegNode::SkySegNode(EagleeyeRuntimeType runtime)
    :SegNode(EagleeyeRuntime(runtime).prefix("skyseg_%s.dlc"),
             EagleeyeRuntime(runtime).device(),
             "input",
             std::vector<int>{1,768,768,3},
             "output",
             std::vector<int>{1,768,768,2},
             std::vector<std::string>{"br/add_br","br/add_br"}){
    this->m_mean_r = 128.314768;
    this->m_mean_g = 131.470580;
    this->m_mean_b = 132.280889;

    this->m_var_r = 255.0f;
    this->m_var_g = 255.0f;
    this->m_var_b = 255.0f;
}

SkySegNode::~SkySegNode(){

}

void SkySegNode::executeNodeInfo(){
    ImageSignal <Array<unsigned char, 3>> *input_img_sig = dynamic_cast<ImageSignal <Array<unsigned char, 3>> *>(this->m_input_signals[0]);
    Matrix <Array<unsigned char, 3>> frame = input_img_sig->getData();

    // 1.step get sky segmentation map
    Tensor<float> score_tensor = this->runSeg(frame);
    Matrix<float> score_tensor_reshape(768*768, 2, score_tensor.dataptr());
    Matrix<float> softmax_score = msoftmax(score_tensor_reshape);
    float* softmax_score_ptr = softmax_score.dataptr();

    // 2.step proprocess    
    Matrix<float> sky_score(768, 768);
    for(int i=0; i<768; ++i){
        float* sky_score_ptr = sky_score.row(i);
        for(int j=0; j<768; ++j){
            sky_score_ptr[j] = softmax_score_ptr[(i*768+j)*2 + 1];
        }
    }

    // 3.step resize to original size
    // ImageSignal<Array<unsigned char, 3>>* input_img_sig = dynamic_cast<ImageSignal<Array<unsigned char, 3>>*>(this->m_input_signals[0]);
    // Matrix<Array<unsigned char, 3>> frame = input_img_sig->getImage();
    int original_rows = frame.rows();
    int original_cols = frame.cols();
    Matrix<float> original_sky_score = resize(sky_score, original_rows, original_cols, BILINEAR_INTERPOLATION);

    ImageSignal<float>* sky_seg_sig = dynamic_cast<ImageSignal<float>*>(this->m_output_signals[0]);
    sky_seg_sig->setData(original_sky_score);
}
}