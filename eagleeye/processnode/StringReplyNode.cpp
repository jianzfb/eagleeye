#include "eagleeye/processnode/StringReplyNode.h"
#include "eagleeye/framework/pipeline/EmptySignal.h"


namespace eagleeye{
StringReplyNode::StringReplyNode(){
    this->setNumberOfOutputSignals(1);
    this->setOutputPort(new EmptySignal(), 0);
}

StringReplyNode::~StringReplyNode(){
}

void StringReplyNode::executeNodeInfo(){
    StringSignal* input_sig = (StringSignal*)this->getInputPort(0);
    if(this->m_callback){
        this->m_callback(input_sig->getData());
    }
}

void StringReplyNode::setCallback(std::function<void(std::string)> callback){
    this->m_callback = callback;
}
}