#include "eagleeye/processnode/IdentityNode.h"

namespace eagleeye
{
IdentityNode::IdentityNode(){
    
}   
IdentityNode::~IdentityNode(){

} 

void IdentityNode::executeNodeInfo(){
    int signal_num = this->getNumberOfInputSignals();
    for(int sig_i=0; sig_i<signal_num; ++sig_i){
        this->getOutputPort(sig_i)->copy(this->getInputPort(sig_i));
    }
}

void IdentityNode::addInputPort(AnySignal* sig){
    int signal_num = this->getNumberOfInputSignals();
    this->setInputPort(sig, signal_num);
}

void IdentityNode::setInputPort(AnySignal* sig,int index){
    if(this->getNumberOfInputSignals() < index + 1){
        this->setNumberOfInputSignals(index + 1);
    }

    if(this->getNumberOfOutputSignals() < index + 1){
        this->setNumberOfOutputSignals(index + 1);
    }

    Superclass::setInputPort(sig, index);
    if(this->getOutputPort(index) == NULL){
        this->setOutputPort(sig->make(), index);
    }
}


} // namespace eagleeye
