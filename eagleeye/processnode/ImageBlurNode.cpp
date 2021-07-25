#include "eagleeye/processnode/ImageBlurNode.h"
namespace eagleeye
{
ImageBlurNode::ImageBlurNode(){
    // 设置输出端口（拥有1个输出端口）
    this->setNumberOfOutputSignals(1);
	this->setOutputPort(new ImageSignal<Array<unsigned char, 3>>(), 0);

    // 设置输入端口数
    // port 0: RGB/BGR
    this->setNumberOfInputSignals(1);

}   
ImageBlurNode::~ImageBlurNode(){

} 

void ImageBlurNode::executeNodeInfo(){
    if(this->getInputPort(0)->getSignalType() != EAGLEEYE_SIGNAL_RGB_IMAGE && 
        this->getInputPort(0)->getSignalType() != EAGLEEYE_SIGNAL_BGR_IMAGE){
        return;
    }
    ImageSignal<Array<unsigned char,3>>* input_sig = (ImageSignal<Array<unsigned char,3>>*)this->getInputPort(0);
    Matrix<Array<unsigned char,3>> input = input_sig->getData();
    if(!input.isContinuous()){
        input = input.clone();
    }
    int rows = input.rows();
    int cols = input.cols();
    ImageSignal<Array<unsigned char,3>>* output_sig = (ImageSignal<Array<unsigned char,3>>*)this->getInputPort(0);
    Matrix<Array<unsigned char,3>> output = output_sig->getData();
    if(output.rows() != rows || output.cols() != cols){
        output = input.clone();
    }

    // 
    

    output_sig->setData(output);
    this->getOutputPort(0)->setSignalType(this->getInputPort(0)->getSignalType());
}
} // namespace eagleeye
