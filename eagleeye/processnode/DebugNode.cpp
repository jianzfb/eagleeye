#include "eagleeye/processnode/DebugNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"

namespace eagleeye
{
DebugNode::DebugNode(int input_port_num, int output_port_num){
    this->m_reset_times = 0;
    this->m_init_times = 0;
    this->m_execute_times = 0;

    this->setNumberOfInputSignals(input_port_num);
    this->setNumberOfOutputSignals(output_port_num);
}

DebugNode::~DebugNode(){
}

void DebugNode::setPortCategoryType(int port_index, SignalCategory port_category, EagleeyeType tt){
    switch (port_category)
    {
    case SIGNAL_CATEGORY_IMAGE:
        if(tt == EAGLEEYE_UCHAR){
            this->setOutputPort(new ImageSignal<unsigned char>(), port_index);
        }
        else if(tt == EAGLEEYE_INT){
            this->setOutputPort(new ImageSignal<int>(), port_index);
        }
        else if(tt == EAGLEEYE_FLOAT){
            this->setOutputPort(new ImageSignal<float>(), port_index);
        }
        else if(tt == EAGLEEYE_RGB){
            this->setOutputPort(new ImageSignal<Array<unsigned char, 3>>(), port_index);
        }
        break;
    case SIGNAL_CATEGORY_STRING:
        this->setOutputPort(new StringSignal(), port_index);
        break;
    case SIGNAL_CATEGORY_CONTROL:
        this->setOutputPort(new BooleanSignal(), port_index);
        break;
    case SIGNAL_CATEGORY_STATE:
        this->setOutputPort(new StateSignal(), port_index);
        break;
    default:
        break;
    }
}

void DebugNode::executeNodeInfo(){
    EAGLEEYE_LOGD("execute %d times (init %d, reset %d)", this->m_execute_times, this->m_init_times, this->m_reset_times);
    for(int signal_i = 0; signal_i<this->getNumberOfOutputSignals(); ++signal_i){
        // EAGLEEYE_LOGD("output port %d category %s");
        EAGLEEYE_LOGD("generate random data");
    }
}

void DebugNode::reset(){
    Superclass::reset();
    this->m_reset_times += 1;
}

void DebugNode::init(){
    Superclass::init();
    this->m_init_times += 1;
    this->m_execute_times = 0; 
}
} // namespace eagleeye
