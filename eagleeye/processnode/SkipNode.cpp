#include "eagleeye/processnode/SkipNode.h"
#include "eagleeye/framework/pipeline/AnyMonitor.h"
#include "eagleeye/common/EagleeyeLog.h"

namespace eagleeye
{
SkipNode::SkipNode()
    :AnyNode("skipnode"){
    EAGLEEYE_MONITOR_VAR(bool, setSkip, getSkip, "skip","","");
}    

SkipNode::~SkipNode(){

}

void SkipNode::executeNodeInfo(){
    if(!this->m_skip){
        // copy input to output directly
        int input_sigs_num = this->getNumberOfInputSignals();
        for(int sig_i=0; sig_i<input_sigs_num; ++sig_i){
            this->getOutputPort(sig_i)->copy(this->getInputPort(sig_i));
        }
    }
}

bool SkipNode::selfcheck(){
    return true;
}

void SkipNode::setSkip(bool skip){
    this->m_skip = skip;
    // set node state
    this->setIsSatisfiedCondition(!skip);       
    modified();
}

void SkipNode::getSkip(bool& skip){
    skip = this->m_skip;
}

void SkipNode::addInputPort(AnySignal* sig){
    Superclass::addInputPort(sig);

    // add new output signal 
    this->setNumberOfOutputSignals(this->getNumberOfOutputSignals() + 1);
    this->setOutputPort(sig->make(), this->getNumberOfOutputSignals()-1);
}

void SkipNode::setInputPort(AnySignal* sig,int index){
    if(index >= this->getNumberOfInputSignals()){
        this->setNumberOfInputSignals(index+1);
    }
    Superclass::setInputPort(sig, index);

    // set output sig
    if(index >= this->getNumberOfOutputSignals()){
        this->setNumberOfOutputSignals(index + 1);
    }
    if(this->getOutputPort(index) != NULL){
        delete this->getOutputPort(index);
    }
    this->setOutputPort(sig->make(), index);
}
} // namespace eagleeye
