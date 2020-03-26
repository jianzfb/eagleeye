#include "eagleeye/framework/pipeline/BooleanSignal.h"
namespace eagleeye
{
BooleanSignal::BooleanSignal(){
    m_boolean = false;
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
    this->m_boolean = false;
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

} // namespace eagleeye
