#include "eagleeye/framework/pipeline/StringSignal.h"
namespace eagleeye
{
StringSignal::StringSignal(){
}   
StringSignal::~StringSignal(){
} 

void StringSignal::copyInfo(AnySignal* sig)
{
	//call the base class
	Superclass::copyInfo(sig);
}

void StringSignal::printUnit()
{
	Superclass::printUnit();
}

void StringSignal::makeempty(bool auto_empty)
{
    this->m_str = std::string();
	//force time update
	modified();
}

bool StringSignal::isempty()
{
    return this->m_str.empty();
}

typename StringSignal::DataType StringSignal::getData(){
	return this->m_str;
}

void StringSignal::setData(StringSignal::DataType data){
	this->m_str = data;
    modified();
}

} // namespace eagleeye
