#include "eagleeye/framework/pipeline/StringSignal.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
namespace eagleeye
{
StringSignal::StringSignal(){
	this->m_sig_category = SIGNAL_CATEGORY_STRING;
}   
StringSignal::~StringSignal(){
} 

void StringSignal::copyInfo(AnySignal* sig){
	//call the base class
	Superclass::copyInfo(sig);
}

void StringSignal::copy(AnySignal* sig){
	if(SIGNAL_CATEGORY_STRING != (sig->getSignalCategory() & SIGNAL_CATEGORY_STRING)){
		return;
	}

	StringSignal* from_sig = (StringSignal*)(sig);
	this->setData(from_sig->getData());
}

void StringSignal::printUnit(){
	Superclass::printUnit();
}

void StringSignal::makeempty(bool auto_empty){
    this->m_str = std::string();
	//force time update
	modified();
}

bool StringSignal::isempty(){
    return this->m_str.empty();
}

typename StringSignal::DataType StringSignal::getData(){
	// refresh data
	if(this->m_link_node != NULL){
		this->m_link_node->refresh();
	}

	if(this->getSignalCategory() == SIGNAL_CATEGORY_STRING){
		// SIGNAL_CATEGORY_STRING
		return m_str;
	}
	else{
		// SIGNAL_CATEGORY_STRING_QUEUE
		std::unique_lock<std::mutex> locker(this->m_mu);
		while(this->m_queue.size() == 0){
            this->m_cond.wait(locker);

			if(this->m_queue.size() > 0 || this->m_signal_exit){
				break;
			}
        }
		if(this->m_signal_exit){
			return std::string();
		}

		std::string data = this->m_queue.front();
        this->m_queue.pop();
        locker.unlock();
		return data;
	}
}

void StringSignal::setData(StringSignal::DataType data){
	if(this->getSignalCategory() == SIGNAL_CATEGORY_STRING){
		// SIGNAL_CATEGORY_STRING
		this->m_str = data;
	}
	else{
		// SIGNAL_CATEGORY_STRING_QUEUE
		std::unique_lock<std::mutex> locker(this->m_mu);
		this->m_queue.push(data); 
		locker.unlock();

		// notify
		this->m_cond.notify_all();		
	}

    modified();
}

void StringSignal::setSignalContent(void* data, const int* data_size, const int data_dims){
	std::string str = *((std::string*)data);
	this->setData(str);
}

void StringSignal::getSignalContent(void*& data, int* data_size, int& data_dims, int& data_type){
	this->m_tmp = this->getData();
	data = &this->m_tmp;
	data_size[0] = 1;
	data_size[1] = this->m_tmp.size();
	data_size[2] = 1;
	data_dims = 3;
	data_type = int(EAGLEEYE_STRING);
}
} // namespace eagleeye
