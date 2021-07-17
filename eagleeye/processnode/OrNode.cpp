#include "eagleeye/processnode/OrNode.h"
#include "eagleeye/common/EagleeyeLog.h"
#include <iostream>

namespace eagleeye
{
OrNode::OrNode(){
    // 连接两个上游节点，只要其中一个节点有值，将其传入下游
}   

OrNode::~OrNode(){

}

void OrNode::executeNodeInfo(){
    int signal_num = this->getNumberOfInputSignals();
    int select_signal_i = -1;
    for(int sig_i=0; sig_i<signal_num; ++sig_i){
        if(this->getInputPort(sig_i)->isHasBeenUpdate()){
            select_signal_i = sig_i;
            break;
        }
    }

    if(select_signal_i == -1){
        EAGLEEYE_LOGE("no valid output, select default - 0");
        select_signal_i = 0;
    }    

    EAGLEEYE_LOGD("passing sig %d output", select_signal_i);
    this->getOutputPort(0)->copy(this->getInputPort(select_signal_i));
    this->getOutputPort(0)->setSignalType(this->getInputPort(select_signal_i)->getSignalType());
}

void OrNode::addInputPort(AnySignal* sig){
    int signal_num = this->getNumberOfInputSignals();
    this->setInputPort(sig, signal_num);
}

void OrNode::setInputPort(AnySignal* sig,int index){
    if(this->getNumberOfInputSignals() < index + 1){
        this->setNumberOfInputSignals(index + 1);
    }
    Superclass::setInputPort(sig, index);
    if(this->getNumberOfOutputSignals() == 0){
        this->setOutputPort(sig->make(), 0);
    }
}

bool OrNode::isNeedProcessed(){
	// check all input signals
	bool is_need_processed = false;
	std::vector<AnySignal*>::iterator iter,iend(m_input_signals.end());
	for (iter = m_input_signals.begin();iter != iend; ++iter){
		if((*iter)->isPreparedOK()){
			is_need_processed = true;
			break;
		}
	}
	
	std::vector<AnySignal*>::iterator o_iter,o_iend(m_output_signals.end());
	bool is_one_ok = true;
	for (o_iter = m_output_signals.begin();o_iter != o_iend; ++o_iter){
		is_one_ok = is_one_ok | (*o_iter)->isPreparedOK();
	}
	is_need_processed = is_need_processed & is_one_ok;

	return is_need_processed;
}
} // namespace eagleeye
