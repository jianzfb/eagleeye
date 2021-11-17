#include "eagleeye/processnode/QueueNode.h"
namespace  eagleeye
{
QueueNode::QueueNode(){
}   


QueueNode::~QueueNode(){

} 


void QueueNode::executeNodeInfo(){
    int signal_num = this->getNumberOfInputSignals();
    for(int signal_i=0; signal_i<signal_num; ++signal_i){
        this->getOutputPort(signal_i)->copy(this->getInputPort(signal_i));
    }
}

void QueueNode::addInputPort(AnySignal* sig){
    Superclass::addInputPort(sig);
    int index = this->getNumberOfInputSignals() - 1;

    AnySignal* sig_cp = sig->make();
    sig_cp->transformCategoryToQ();

    if(this->getNumberOfOutputSignals() < index+1){
        this->setNumberOfOutputSignals(index+1);
    }
    Superclass::setOutputPort(sig_cp, index);
}

void QueueNode::setInputPort(AnySignal* sig,int index){
    if(this->getNumberOfInputSignals() < index+1){
        this->setNumberOfInputSignals(index+1);
    }
    Superclass::setInputPort(sig, index);

    AnySignal* sig_cp = sig->make();
    sig_cp->transformCategoryToQ();

    if(this->getNumberOfOutputSignals() < index+1){
        this->setNumberOfOutputSignals(index+1);
    }
    Superclass::setOutputPort(sig_cp, index);

}
} // namespace  eagleeye
