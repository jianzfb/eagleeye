#include "eagleeye/processnode/LogicalNode.h"

namespace eagleeye
{
LogicalNode::LogicalNode(LogicalType logical_type){
    this->setNumberOfInputSignals(2);
    this->setNumberOfOutputSignals(1);
    this->setOutputPort(new BooleanSignal(), 0);

    this->m_logical_type = logical_type;
}

LogicalNode::~LogicalNode(){

}

void LogicalNode::executeNodeInfo(){
    BooleanSignal* bs_0 = (BooleanSignal*)this->getInputPort(0);
    BooleanSignal* bs_1 = (BooleanSignal*)this->getInputPort(1);

    BooleanSignal* output_bs = (BooleanSignal*)this->getOutputPort(0);

    switch (this->m_logical_type)
    {
    case LOGICAL_AND:
        output_bs->setData(bs_0->getData() && bs_1->getData());
        break;
    case LOGICAL_OR:
        output_bs->setData(bs_0->getData() || bs_1->getData());
        break;
    default:
        output_bs->setData(bs_0->getData() && bs_1->getData());
        break;
    }
}
} // namespace eagleeye
