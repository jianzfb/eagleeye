#include "eagleeye/processnode/HistogramEqualizationNode.h"

namespace eagleeye
{
HistogramEqualizationNode::HistogramEqualizationNode(){
    // 设置输出端口（拥有1个输出端口）
    this->setNumberOfOutputSignals(1);
    // 设置输出端口(端口0)及携带数据类型(TargetT)
    this->setOutputPort(new ImageSignal<Array<unsigned char,3>>, 0);

    // 设置输入端口（拥有1个输入端口）
	this->setNumberOfInputSignals(1);

}   

HistogramEqualizationNode::~HistogramEqualizationNode(){

}

void HistogramEqualizationNode::executeNodeInfo(){
    // 1.step get image
    ImageSignal<Array<unsigned char, 3>>* input_img_sig = (ImageSignal<Array<unsigned char, 3>>*)(this->getInputPort(0));
    Matrix<Array<unsigned char,3>> image = input_img_sig->getData();

    // 2.step histogram equalization
    float hist[256];
    memset(hist, 0, sizeof(float)*256);
    int rows = image.rows(); int cols = image.cols();
    int total = rows*cols*3;
    for(int i=0; i<rows; ++i){
        Array<unsigned char,3>* ptr = image.row(i);
        for(int j=0; j<cols; ++j){
            hist[ptr[j][0]] += 1.0f;
            hist[ptr[j][1]] += 1.0f;
            hist[ptr[j][2]] += 1.0f;
        }
    }

    int mapto[256];
    for(int i=0; i<256; ++i){
        hist[i] /= float(total);
        if(i > 0){
            hist[i] += hist[i-1];
        }

        mapto[i] = hist[i]*255;
    }
    Matrix<Array<unsigned char,3>> output(rows, cols);
    for(int i=0; i<rows; ++i){
        Array<unsigned char,3>* in_ptr = image.row(i);
        Array<unsigned char,3>* out_ptr = output.row(i);
        for(int j=0; j<cols; ++j){
            out_ptr[j][0] = mapto[in_ptr[j][0]];
            out_ptr[j][1] = mapto[in_ptr[j][1]];
            out_ptr[j][2] = mapto[in_ptr[j][2]];
        }
    }

    // 3.step output
    ImageSignal<Array<unsigned char, 3>>* output_img_sig = (ImageSignal<Array<unsigned char, 3>>*)(this->getOutputPort(0));
    output_img_sig->setData(output);
}
} // namespace eagleeye
