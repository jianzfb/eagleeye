#include "eagleeye/processnode/IsUpdateNode.h"
#include "eagleeye/framework/pipeline/BooleanSignal.h"

namespace eagleeye
{
IsUpdateNode::IsUpdateNode(){
    this->setNumberOfInputSignals(1);
    this->setNumberOfOutputSignals(1);
    this->setOutputPort(new BooleanSignal(), 0);
}   

IsUpdateNode::~IsUpdateNode(){

}

void IsUpdateNode::executeNodeInfo(){
    BooleanSignal* output_sig = (BooleanSignal*)(this->getOutputPort(0));

    if(this->getInputPort(0)->isHasBeenUpdate()){
        output_sig->setData(true);
    }
    else{
        output_sig->setData(false);
    }
}
} // namespace eagleeye
