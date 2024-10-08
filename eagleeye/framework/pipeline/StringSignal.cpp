#include "eagleeye/framework/pipeline/StringSignal.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/common/EagleeyeLog.h"


namespace eagleeye
{
StringSignal::StringSignal(std::string ini_str){
	this->m_ini_str = ini_str;
	this->m_sig_category = SIGNAL_CATEGORY_STRING;

	this->m_release_count = 1;
	this->m_max_queue_size = 5;
}   
StringSignal::~StringSignal(){
} 

void StringSignal::setInit(std::string ini_str){
    this->m_ini_str = ini_str;
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
	this->m_ini_str = from_sig->m_ini_str;
}

void StringSignal::printUnit(){
	Superclass::printUnit();
}

void StringSignal::makeempty(bool auto_empty){
	if(auto_empty){
		if(this->m_release_count % this->getOutDegree() != 0){
			this->m_release_count += 1;
			return;
		}
	}

    this->m_str = this->m_ini_str;

	if(auto_empty){
		this->m_release_count = 1;
	}

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

			if(this->m_queue.size() > 0){
				break;
			}
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
		if(this->m_queue.size() > this->m_max_queue_size){
			this->m_queue.pop();
		}
		this->m_queue.push(data); 
		locker.unlock();

		// notify
		this->m_cond.notify_all();		
	}

    modified();
}

void StringSignal::setData(void* data, MetaData meta){
	EAGLEEYE_LOGD("Set signal string content");
	std::string str = *((std::string*)data);
	this->setData(str);
}

void StringSignal::getSignalContent(void*& data, size_t*& data_size, int& data_dims, int& data_type){
	this->m_tmp_cache = this->getData();
	data = &this->m_tmp_cache;
	this->m_data_size[0] = this->m_tmp_cache.size();
	data_size = m_data_size;
	data_dims = 1;
	data_type = int(EAGLEEYE_STRING);
}
} // namespace eagleeye
