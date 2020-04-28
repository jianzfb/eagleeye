#include "eagleeye/processnode/CopyNode.h"

namespace eagleeye
{
CopyNode::CopyNode(bool inplace, bool is_clear_input){
    m_inplace = inplace;
    m_is_clear_input = is_clear_input;
}   

CopyNode::~CopyNode(){

}

void CopyNode::executeNodeInfo(){
    int signal_num = this->getNumberOfInputSignals();
    for(int sig_i=0; sig_i<signal_num; ++sig_i){
        this->getOutputPort(sig_i)->copy(this->getInputPort(sig_i));
    }

    if(this->m_is_clear_input){
        for(int sig_i=0; sig_i<signal_num; ++sig_i){
            this->getInputPort(sig_i)->makeempty(false);
        }
    }
}

void CopyNode::addInputPort(AnySignal* sig){
    int signal_num = this->getNumberOfInputSignals();
    this->setInputPort(sig, signal_num);
}

void CopyNode::setInputPort(AnySignal* sig,int index){
    if(this->getNumberOfInputSignals() < index + 1){
        this->setNumberOfInputSignals(index + 1);
    }

    if(this->getNumberOfOutputSignals() < index + 1){
        this->setNumberOfOutputSignals(index + 1);
    }

    Superclass::setInputPort(sig, index);
    this->setOutputPort(sig->make(), index);
}
} // namespace eagleeye
