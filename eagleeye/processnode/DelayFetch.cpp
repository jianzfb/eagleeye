#include "eagleeye/processnode/DelayFetch.h"
#include "eagleeye/framework/pipeline/AnyMonitor.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeTime.h"


namespace eagleeye
{
DelayFetch::DelayFetch(){
    this->m_delay_count = 1;
    this->m_count = 0;
}    

DelayFetch::~DelayFetch(){

}

void DelayFetch::executeNodeInfo(){
    if(this->m_count >= this->m_delay_count){
        int signal_num = this->getNumberOfInputSignals();
        for(int sig_i=0; sig_i<signal_num; ++sig_i){
            this->getOutputPort(sig_i)->copy(this->getInputPort(sig_i));
        }
    }

    if(this->m_count < m_delay_count){
        this->m_count += 1;
    }
}

void DelayFetch::setDelayTime(int delay_time){
    this->m_delay_count = delay_time;
    this->modified();
}
void DelayFetch::getDelayTime(int& delay_time){
    delay_time = this->m_delay_count;
}

void DelayFetch::addInputPort(AnySignal* sig){
    int signal_num = this->getNumberOfInputSignals();
    this->setInputPort(sig, signal_num);
}

void DelayFetch::setInputPort(AnySignal* sig,int index){
    if(this->getNumberOfInputSignals() < index + 1){
        this->setNumberOfInputSignals(index + 1);
    }

    if(this->getNumberOfOutputSignals() < index + 1){
        this->setNumberOfOutputSignals(index + 1);
    }

    Superclass::setInputPort(sig, index);
    this->setOutputPort(sig->make(), index);
}
} // namespace eagleeye