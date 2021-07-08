#include "eagleeye/processnode/ConstNode.h"
namespace eagleeye
{
ConstNode::ConstNode(int input_port_num, std::vector<AnySignal*> output_const_sigs){
    this->setNumberOfInputSignals(input_port_num);

    // no input ports
    this->setNumberOfOutputSignals(output_const_sigs.size());
    for(int i=0; i<output_const_sigs.size(); ++i){
        this->setOutputPort(output_const_sigs[i], i);
    }
}   
ConstNode::~ConstNode(){

} 

void ConstNode::executeNodeInfo(){
    // do nothing
}
} // namespace eagleeye
