#include "eagleeye/processnode/AdaptorNode.h"
#include <vector>

namespace eagleeye{
AdaptorNode::AdaptorNode(int input_sigs_num, int output_sigs_num, AdaptorFunc func){
    this->setNumberOfOutputSignals(output_sigs_num);
	this->setNumberOfInputSignals(input_sigs_num);
    this->m_adaptor_func = func;
}

AdaptorNode::~AdaptorNode(){

}

void AdaptorNode::executeNodeInfo(){
    std::vector<AnySignal*> input_signals;
    std::vector<AnySignal*> output_signals;

    for(int i=0; i<this->getNumberOfOutputSignals(); ++i){
        output_signals.push_back(this->getOutputPort(i));
    }
    for(int i=0; i<this->getNumberOfInputSignals(); ++i){
        input_signals.push_back(this->getInputPort(i));
    }

    this->m_adaptor_func(input_signals, output_signals);
}
}