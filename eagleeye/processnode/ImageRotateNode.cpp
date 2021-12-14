#include "eagleeye/processnode/ImageRotateNode.h"
namespace eagleeye
{
ImageRotateNode::ImageRotateNode(float default_rot_deg)
    :m_default_rot_deg(default_rot_deg){
    // 设置输出端口（拥有1个输出端口）
    this->setNumberOfOutputSignals(1);
}   

ImageRotateNode::~ImageRotateNode(){

}

void ImageRotateNode::executeNodeInfo(){
    // 
}

void ImageRotateNode::addInputPort(AnySignal* sig){
    int signal_num = this->getNumberOfInputSignals();
    this->setInputPort(sig, signal_num);
}

void ImageRotateNode::setInputPort(AnySignal* sig,int index){
    if(index >= 2){
        EAGLEEYE_LOGE("Only support input signal 2");
        return;
    }
    if(this->getNumberOfInputSignals() < index+1){
        this->setNumberOfInputSignals(index+1);
    }

    Superclass::setInputPort(sig, index);
    if(index == 0){
        this->setOutputPort(sig->make(), index);
        this->getOutputPort(0)->setSignalType(sig->getSignalType());
    }
}
} // namespace eagleeye
