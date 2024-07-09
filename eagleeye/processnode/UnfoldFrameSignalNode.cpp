#include "eagleeye/processnode/UnfoldFrameSignalNode.h"
namespace eagleeye{
UnfoldFrameSignalNode::UnfoldFrameSignalNode(){
    this->setNumberOfInputSignals(1);
    this->setNumberOfOutputSignals(2);
}
UnfoldFrameSignalNode::~UnfoldFrameSignalNode(){

}

void UnfoldFrameSignalNode::executeNodeInfo(){
    this->getOutputPort(0)->copy(this->getInputPort(0));
    MetaData meta = this->getOutputPort(0)->meta();
    ImageSignal<double>* timestamp_sig = (ImageSignal<double>*)this->getOutputPort(1);
    Matrix<double> timestamp_mat(1,1);

    timestamp_mat.at(0,0) = meta.timestamp;
    timestamp_sig->setData(timestamp_mat);
}

void UnfoldFrameSignalNode::setInputPort(AnySignal* sig,int index){
    // 忽略端口设置，仅接受0端口
    Superclass::setInputPort(sig, 0);

    // 设置输出端口
    this->setOutputPort(sig->make(), 0);
    this->setOutputPort(new ImageSignal<double>(), 1);
}
}