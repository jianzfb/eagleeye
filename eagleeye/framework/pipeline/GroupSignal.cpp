#include "eagleeye/framework/pipeline/GroupSignal.h"

namespace eagleeye
{
GroupSignal::GroupSignal(){
    this->setSignalType(EAGLEEYE_SIGNAL_GROUP);
}   
GroupSignal::~GroupSignal(){

} 

void GroupSignal::copyInfo(AnySignal* sig){
    if(sig == NULL){
        return;
    }

    Superclass::copyInfo(sig);
}

void GroupSignal::copy(AnySignal* sig, bool is_deep){
    if((SIGNAL_CATEGORY_DEFAULT != (sig->getSignalCategory() & SIGNAL_CATEGORY_DEFAULT)) || (this->getSignalType() != sig->getSignalType())){
		return;
	}
	GroupSignal* from_sig = (GroupSignal*)(sig);	

	MetaData from_data_meta;
	DataType from_data = from_sig->getData(from_data_meta);
	this->setData(from_data, from_data_meta);
}

void GroupSignal::printUnit(){
    Superclass::printUnit();
}

void GroupSignal::makeempty(bool auto_empty){
    this->m_data.clear();
}

bool GroupSignal::isempty(){
    if(this->getSignalCategory() == SIGNAL_CATEGORY_DEFAULT){
        if(this->m_data.size() == 0){
            return true;
        }
    }

    return false;
}

typename GroupSignal::DataType GroupSignal::getData(){
    return this->m_data;
}

typename GroupSignal::DataType GroupSignal::getData(MetaData& mm){
    mm = this->m_meta;
    return this->m_data;
}

void GroupSignal::setData(typename GroupSignal::DataType data){
    this->m_data = data;
}

void GroupSignal::setMeta(MetaData meta){
    this->m_meta = meta;
}

void GroupSignal::setData(typename GroupSignal::DataType data, MetaData mm){
    this->m_data = data;
    this->m_meta = mm;
}

} // namespace eagleeye
