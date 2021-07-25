#include "eagleeye/processnode/AnyPlaceholder.h"
namespace eagleeye
{
AnyPlaceholder::AnyPlaceholder(){
	this->setNumberOfOutputSignals(1);
}   

AnyPlaceholder::~AnyPlaceholder(){

} 

void AnyPlaceholder::executeNodeInfo(){

}

void AnyPlaceholder::setSignalType(SignalType type){
    this->getOutputPort(0)->setSignalType(type);
}

void AnyPlaceholder::setSource(SignalTarget target){
    this->getOutputPort(0)->setSignalTarget(target);
}

void AnyPlaceholder::generate(AnySignal* sig){
    this->setOutputPort(sig->make(), 0);
    this->getOutputPort(0)->setSignalType(sig->getSignalType());
}

bool AnyPlaceholder::selfcheck(){
    bool is_ok = true;
    if(this->getOutputPort(0)->isempty()){
        is_ok = false;
        EAGLEEYE_LOGD("placeholder %s is empty", this->getUnitName());
    }
    return is_ok;
}

void AnyPlaceholder::preexit(){

}

void AnyPlaceholder::postexit(){
    this->getOutputPort(0)->makeempty();
}

void AnyPlaceholder::reset(){
    this->getOutputPort(0)->makeempty();
    // update reset time
	m_reset_timestamp.modified();
	m_reset_time = m_reset_timestamp.getMTime();
}
} // namespace eagleeye
