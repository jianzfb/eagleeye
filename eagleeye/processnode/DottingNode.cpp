#include "eagleeye/processnode/DottingNode.h"
#include "eagleeye/processnode/DottingNode.h"
#include "eagleeye/common/EagleeyeLog.h"


namespace eagleeye
{
DottingNode::DottingNode(std::string dotting_str)
    :m_dotting_str(dotting_str){

}   

DottingNode::~DottingNode(){

}

void DottingNode::executeNodeInfo(){
    int signal_num = this->getNumberOfInputSignals();
    for(int sig_i=0; sig_i<signal_num; ++sig_i){
        this->getOutputPort(sig_i)->copy(this->getInputPort(sig_i));
    }

    EAGLEEYE_LOGD("%s", this->m_dotting_str.c_str());
}

void DottingNode::setDottingStr(std::string dotting_str){
    this->m_dotting_str = dotting_str;
    modified();
}

void DottingNode::addInputPort(AnySignal* sig){
    int signal_num = this->getNumberOfInputSignals();
    this->setInputPort(sig, signal_num);
}

void DottingNode::setInputPort(AnySignal* sig,int index){
    if(this->getNumberOfInputSignals() < index + 1){
        this->setNumberOfInputSignals(index + 1);
    }
    Superclass::setInputPort(sig, index);
    

    if(this->getNumberOfOutputSignals() < index + 1){
        this->setNumberOfOutputSignals(index + 1);
    }
    this->setOutputPort(sig->make(), index);
}
} // namespace eagleeye
