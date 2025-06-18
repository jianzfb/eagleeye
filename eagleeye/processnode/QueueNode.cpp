#include "eagleeye/processnode/QueueNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
namespace  eagleeye
{
QueueNode::QueueNode(int queue_size, bool get_then_auto_remove, bool set_then_auto_remove){
    this->m_queue_size = queue_size;
    this->m_get_then_auto_remove = get_then_auto_remove;
    this->m_set_then_auto_remove = set_then_auto_remove;
}

QueueNode::~QueueNode(){

}

void QueueNode::executeNodeInfo(){
    int signal_num = this->getNumberOfInputSignals();
    for(int signal_i=0; signal_i<signal_num; ++signal_i){
        this->getOutputPort(signal_i)->copy(this->getInputPort(signal_i));
    }
}

void QueueNode::addInputPort(AnySignal* sig){
    Superclass::addInputPort(sig);
    int index = this->getNumberOfInputSignals() - 1;

    AnySignal* sig_cp = sig->make();
    sig_cp->transformCategoryToQ(this->m_queue_size, this->m_get_then_auto_remove, this->m_set_then_auto_remove);

    if(this->getNumberOfOutputSignals() < index+1){
        this->setNumberOfOutputSignals(index+1);
    }
    Superclass::setOutputPort(sig_cp, index);
}

void QueueNode::setInputPort(AnySignal* sig,int index){
    if(this->getNumberOfInputSignals() < index+1){
        this->setNumberOfInputSignals(index+1);
    }
    Superclass::setInputPort(sig, index);

    AnySignal* sig_cp = sig->make();
    sig_cp->transformCategoryToQ(this->m_queue_size, this->m_get_then_auto_remove, this->m_set_then_auto_remove);

    if(this->getNumberOfOutputSignals() < index+1){
        this->setNumberOfOutputSignals(index+1);
    }
    Superclass::setOutputPort(sig_cp, index);
}

void QueueNode::postexit(){
    // 加入一个无效数据，触发后续节点，以防止后续节点退出阻塞
    int signal_num = this->getNumberOfInputSignals();
    for(int signal_i=0; signal_i<signal_num; ++signal_i){
        this->getOutputPort(signal_i)->copy(this->getInputPort(signal_i));
    }
}
} // namespace  eagleeye
