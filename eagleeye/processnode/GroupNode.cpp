#include "eagleeye/processnode/GroupNode.h"
#include "eagleeye/framework/pipeline/EmptySignal.h"
#include "eagleeye/framework/pipeline/GroupSignal.h"

namespace eagleeye
{
GroupNode::GroupNode(){
    // 1个输出
    this->setNumberOfOutputSignals(1);
    this->setOutputPort(new GroupSignal(), 0);

    // 任意输入信号
}
GroupNode::~GroupNode(){

}

void GroupNode::executeNodeInfo(){
    //  do nothing
}

void GroupNode::addInputPort(AnySignal* sig){
    int signal_num = this->getNumberOfInputSignals();
    this->setInputPort(sig, signal_num);
}

void GroupNode::setInputPort(AnySignal* sig,int index){
    if(this->getNumberOfInputSignals() < index + 1){
        this->setNumberOfInputSignals(index + 1);
    }

    Superclass::setInputPort(sig, index);
    GroupSignal* gs = (GroupSignal*)this->getOutputPort(0);
    std::vector<AnySignal*> signal_list = gs->getData();
    signal_list.push_back(sig);
    gs->setData(signal_list);
}
} // namespace eagleeye
