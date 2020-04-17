#include "eagleeye/framework/pipeline/BooleanSignal.h"
namespace eagleeye
{
BooleanSignal::BooleanSignal(bool ini_boolean){
	this->m_ini_boolean = ini_boolean;
	this->m_boolean = ini_boolean;
}   
BooleanSignal::~BooleanSignal(){

} 

void BooleanSignal::copyInfo(AnySignal* sig)
{
	//call the base class
	Superclass::copyInfo(sig);
}

void BooleanSignal::printUnit()
{
	Superclass::printUnit();
}

void BooleanSignal::makeempty(bool auto_empty)
{
    this->m_boolean = this->m_ini_boolean;
	//force time update
	modified();
}

bool BooleanSignal::isempty()
{
    return false;
}

typename BooleanSignal::DataType BooleanSignal::getData(){
	return this->m_boolean;
}

void BooleanSignal::setData(BooleanSignal::DataType data){
	this->m_boolean = data;
	modified();
}

void BooleanSignal::copy(AnySignal* sig){
	if(sig->getSignalCategory() == SIGNAL_CATEGORY_CONTROL){
		BooleanSignal* b_sig = (BooleanSignal*)sig;
		this->setData(b_sig->getData());
	}
}
} // namespace eagleeye
