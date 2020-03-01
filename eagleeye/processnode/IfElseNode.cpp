#include "eagleeye/processnode/IfElseNode.h"

namespace eagleeye{
IfElseNode::IfElseNode(AnyNode* x, AnyNode* y)
    :AnyNode("ifelse"){
    // set output port
    this->setNumberOfOutputSignals(1);
    this->setOutputPort(x->getOutputPort(0)->make(), 0);

    // set input port number
    // 0 - conditon signal
    // 1 - data signal
    this->setNumberOfInputSignals(2);

    this->m_x = x;
    this->m_y = y;
}    

IfElseNode::~IfElseNode(){
    if(this->m_x){
        delete this->m_x;
    }
    if(this->m_y){
        delete this->m_y;
    }
}

void IfElseNode::executeNodeInfo(){
    // get input / output signal
    // 1.step 条件信号
    BooleanSignal* condition_sig = (BooleanSignal*)(this->getInputPort(0));
    AnySignal* input_sig = this->getInputPort(1);
    if(condition_sig->getData()){
        // 执行X
        this->m_x->setInputPort(input_sig);
        this->m_x->start();

        this->getOutputPort(0)->copy(this->m_x->getOutputPort(0));
    }
    else{
        // 执行Y
        this->m_y->setInputPort(input_sig);
        this->m_y->start();

        this->getOutputPort(0)->copy(this->m_y->getOutputPort(0));
    }
}

bool IfElseNode::selfcheck(){
    return true;
}
}