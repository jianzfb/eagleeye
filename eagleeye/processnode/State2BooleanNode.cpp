#include "eagleeye/processnode/State2BooleanNode.h"

namespace eagleeye
{
State2BooleanNode::State2BooleanNode(std::map<int,bool> state_2_bool){
    this->setNumberOfInputSignals(1);
    this->setNumberOfOutputSignals(1);

    this->setOutputPort(new BooleanSignal(), 0);

    this->m_state_2_bool = state_2_bool;
}   

State2BooleanNode::~State2BooleanNode(){

}

void State2BooleanNode::executeNodeInfo(){
    StateSignal* state_sig = (StateSignal*)this->getInputPort(0);
    BooleanSignal* b_sig = (BooleanSignal*)this->getOutputPort(0);

    if(this->m_state_2_bool.find(state_sig->getData()) == this->m_state_2_bool.end()){
        b_sig->setData(false);
    }

    b_sig->setData(this->m_state_2_bool[state_sig->getData()]);
}
} // namespace eagleeye
