#include "eagleeye/processnode/CallbackNode.h"
#include "eagleeye/framework/pipeline/EmptySignal.h"


namespace eagleeye{
CallbackNode::CallbackNode(std::function<void(AnyNode*, std::vector<AnySignal*>)> callback){
    if(callback != nullptr){
        this->m_callback = callback;
    }

    this->setNumberOfOutputSignals(1);
    this->setOutputPort(new EmptySignal(), 0);
}

CallbackNode::~CallbackNode(){
}

void CallbackNode::executeNodeInfo(){
    int input_signal_num = this->getNumberOfInputSignals();
    std::vector<AnySignal*> input_signals;
    for(int index=0; index<input_signal_num; ++index){
        input_signals.push_back(this->getInputPort(index));
    }

    this->m_callback(this, input_signals);
}

void CallbackNode::setCallback(std::string name, std::function<void(AnyNode*, std::vector<AnySignal*>)> callback){
    this->m_callback = callback;
}
}