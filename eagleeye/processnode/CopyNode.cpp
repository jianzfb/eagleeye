#include "eagleeye/processnode/CopyNode.h"

namespace eagleeye
{
CopyNode::CopyNode(bool inplace){
    this->setNumberOfOutputSignals(1);
    this->setNumberOfInputSignals(1);

    m_inplace = inplace;
}   

CopyNode::~CopyNode(){

}

void CopyNode::executeNodeInfo(){
    this->getOutputPort(0)->copy(this->getInputPort(0));
}

void CopyNode::addInputPort(AnySignal* sig){
    this->setInputPort(sig, 0);
}

void CopyNode::setInputPort(AnySignal* sig,int index){
    Superclass::setInputPort(sig, 0);
    this->setOutputPort(sig->make(), 0);
}
} // namespace eagleeye
