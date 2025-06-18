#include "eagleeye/framework/pipeline/StringSignal.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/common/EagleeyeLog.h"


namespace eagleeye
{
StringSignal::StringSignal(std::string ini_str){
	this->m_ini_str = ini_str;
	this->m_sig_category = SIGNAL_CATEGORY_STRING;

	this->m_max_queue_size = 5;
	this->setSignalType(EAGLEEYE_SIGNAL_TEXT);

	m_get_then_auto_remove = true;
	m_set_then_auto_remove = true;

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

void StringSignal::copy(AnySignal* sig, bool is_deep){
	if(SIGNAL_CATEGORY_STRING != (sig->getSignalCategory() & SIGNAL_CATEGORY_STRING)){
		return;
	}

	StringSignal* from_sig = (StringSignal*)(sig);
	MetaData from_data_meta;
	DataType data = from_sig->getData(from_data_meta);
	this->setData(data, from_data_meta);
}

void StringSignal::printUnit(){
	Superclass::printUnit();
}

void StringSignal::makeempty(bool auto_empty){
    this->m_str = this->m_ini_str;

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
			if(this->m_queue.size() > 0 || m_disable){
				break;
			}
        }
		if(m_disable){
			// 由于失活，产生空数据返回
			locker.unlock();
			return "";
		}

		std::pair<std::string, int> data_info = this->m_queue.front();
		std::string data = data_info.first;
		this->m_queue.front().second -= 1;
		if(this->m_queue.front().second == 0){
			this->m_queue.pop();
			this->m_meta_queue.pop();
		}

        locker.unlock();
		return data;
	}
}

typename StringSignal::DataType StringSignal::getData(MetaData& mm){
	// refresh data
	if(this->m_link_node != NULL){
		this->m_link_node->refresh();
	}

	if(this->getSignalCategory() == SIGNAL_CATEGORY_STRING){
		// SIGNAL_CATEGORY_STRING
		mm = this->m_meta;
		return m_str;
	}
	else{
		// SIGNAL_CATEGORY_STRING_QUEUE
		std::unique_lock<std::mutex> locker(this->m_mu);
		while(this->m_queue.size() == 0){
            this->m_cond.wait(locker);
			if(this->m_queue.size() > 0 || m_disable){
				break;
			}
        }
		if(m_disable){
			// 由于失活，产生空数据返回
			locker.unlock();
			mm = MetaData();
			mm.disable = true;
			return "";
		}

		std::pair<std::string, int> data_info = this->m_queue.front();
		std::string data = data_info.first;
		std::pair<MetaData, int> meta_info = this->m_meta_queue.front();
		mm = meta_info.first;
		
		if(this->m_get_then_auto_remove){
			this->m_queue.front().second -= 1;
			if(this->m_queue.front().second == 0){
				this->m_queue.pop();
				this->m_meta_queue.pop();
			}
		}

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
		if(this->m_queue.size() > this->m_max_queue_size && m_set_then_auto_remove){
			// TODO, 非去除模式，需要阻塞
			this->m_queue.pop();
			this->m_meta_queue.pop();
		}
		this->m_queue.push(std::pair<std::string, int>{data, int(this->getOutDegree())}); 
		MetaData mm;
		this->m_meta_queue.push(std::pair<MetaData, int>{mm, int(this->getOutDegree())});
		locker.unlock();

		// notify
		this->m_cond.notify_all();		
	}

    modified();
}

void StringSignal::setData(StringSignal::DataType data, MetaData mm){
	if(this->getSignalCategory() == SIGNAL_CATEGORY_STRING){
		// SIGNAL_CATEGORY_STRING
		this->m_str = data;
	}
	else{
		// SIGNAL_CATEGORY_STRING_QUEUE
		std::unique_lock<std::mutex> locker(this->m_mu);
		if(this->m_queue.size() > this->m_max_queue_size && m_set_then_auto_remove){
			// TODO, 非去除模式，需要阻塞
			this->m_queue.pop();
			this->m_meta_queue.pop();
		}
		this->m_queue.push(std::pair<std::string, int>{data, int(this->getOutDegree())}); 
		this->m_meta_queue.push(std::pair<MetaData, int>{mm, int(this->getOutDegree())});
		locker.unlock();

		// notify
		this->m_cond.notify_all();		
	}

    modified();
}

void StringSignal::setData(void* data, MetaData meta){
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


void StringSignal::wake(){
	this->m_cond.notify_all();
}

bool StringSignal::tryClear(){
	if(m_sig_category != SIGNAL_CATEGORY_STRING_QUEUE){
		EAGLEEYE_LOGE("not string-queue mode, dont exec.");
		return false;
	}
	if(this->m_get_then_auto_remove){
		return false;
	}

	std::unique_lock<std::mutex> locker(this->m_mu);
	this->m_queue.front().second -= 1;
	if(this->m_queue.front().second == 0){
		this->m_queue.pop();
		this->m_meta_queue.pop();
		return true;
	}
	return false;
}

/* ***************************************************************************************************** */
ListStringSignal::ListStringSignal(){
	this->m_sig_category = SIGNAL_CATEGORY_LIST_STRING;
	this->m_max_queue_size = 5;

	m_get_then_auto_remove = true;
	m_set_then_auto_remove = true;
}   
ListStringSignal::~ListStringSignal(){
} 

void ListStringSignal::copyInfo(AnySignal* sig){
	//call the base class
	Superclass::copyInfo(sig);
}

void ListStringSignal::copy(AnySignal* sig, bool is_copy){
	if(this->getSignalCategory() != sig->getSignalCategory()){
		return;
	}

	ListStringSignal* from_sig = (ListStringSignal*)(sig);
	MetaData from_data_meta;
	DataType from_data = from_sig->getData(from_data_meta);
	this->setData(from_data, from_data_meta);
}

void ListStringSignal::printUnit(){
	Superclass::printUnit();
}

void ListStringSignal::makeempty(bool auto_empty){
	this->m_list.clear();

	//force time update
	modified();
}

bool ListStringSignal::isempty(){
    if(this->m_list.size() == 0){
		return true;
	}
	return false;
}

typename ListStringSignal::DataType ListStringSignal::getData(){
	// refresh data
	if(this->m_link_node != NULL){
		this->m_link_node->refresh();
	}

	if(this->getSignalCategory() == SIGNAL_CATEGORY_LIST_STRING){
		return m_list;
	}
	else{
		std::unique_lock<std::mutex> locker(this->m_mu);
		while(this->m_queue.size() == 0){
            this->m_cond.wait(locker);
			if(this->m_queue.size() > 0 || m_disable){
				break;
			}
        }
		if(m_disable){
			// 由于失活，产生空数据返回
			locker.unlock();
			return std::vector<std::string>();
		}

		std::pair<std::vector<std::string>, int> data_info = this->m_queue.front();
		std::vector<std::string> data = data_info.first;
		this->m_queue.front().second -= 1;
		if(this->m_queue.front().second == 0){
			this->m_queue.pop();
			this->m_meta_queue.pop();
		}

        locker.unlock();
		return data;
	}
}

typename ListStringSignal::DataType ListStringSignal::getData(MetaData& mm){
	// refresh data
	if(this->m_link_node != NULL){
		this->m_link_node->refresh();
	}

	if(this->getSignalCategory() == SIGNAL_CATEGORY_LIST_STRING){
		mm = m_meta;
		return m_list;
	}
	else{
		std::unique_lock<std::mutex> locker(this->m_mu);
		while(this->m_queue.size() == 0){
            this->m_cond.wait(locker);
			if(this->m_queue.size() > 0 || m_disable){
				break;
			}
        }
		if(m_disable){
			// 由于失活，产生空数据返回
			locker.unlock();
			return std::vector<std::string>();
		}

		std::pair<std::vector<std::string>, int> data_info = this->m_queue.front();
		std::vector<std::string> data = data_info.first;
		std::pair<MetaData, int> meta_info = this->m_meta_queue.front();
		mm = meta_info.first;

		if(this->m_get_then_auto_remove){
			this->m_queue.front().second -= 1;
			if(this->m_queue.front().second == 0){
				this->m_queue.pop();
				this->m_meta_queue.pop();
			}
		}
        locker.unlock();
		return data;
	}
}

void ListStringSignal::setData(ListStringSignal::DataType data){
	if(this->getSignalCategory() == SIGNAL_CATEGORY_LIST_STRING){
		this->m_list = data;
	}
	else{
		std::unique_lock<std::mutex> locker(this->m_mu);
		if(this->m_queue.size() > this->m_max_queue_size && m_set_then_auto_remove){
			// TODO, 非去除模式，需要阻塞
			this->m_queue.pop();
			this->m_meta_queue.pop();
		}
		this->m_queue.push(std::pair<std::vector<std::string>, int>{data, int(this->getOutDegree())}); 
		MetaData mm;
		this->m_meta_queue.push(std::pair<MetaData, int>{mm, this->getOutDegree()});
		locker.unlock();

		// notify
		this->m_cond.notify_all();		
	}

    modified();
}

void ListStringSignal::setData(ListStringSignal::DataType data, MetaData mm){
	if(this->getSignalCategory() == SIGNAL_CATEGORY_LIST_STRING){
		this->m_list = data;
		this->m_meta = mm;
	}
	else{
		std::unique_lock<std::mutex> locker(this->m_mu);
		if(this->m_queue.size() > this->m_max_queue_size && m_set_then_auto_remove){
			// TODO, 非去除模式，需要阻塞
			this->m_queue.pop();
			this->m_meta_queue.pop();
		}
		this->m_queue.push(std::pair<std::vector<std::string>, int>{data, int(this->getOutDegree())}); 
		this->m_meta_queue.push(std::pair<MetaData, int>{mm, this->getOutDegree()});
		locker.unlock();

		// notify
		this->m_cond.notify_all();		
	}

    modified();
}

void ListStringSignal::setData(void* data, MetaData meta){
	std::vector<std::string> str_list = *((std::vector<std::string>*)data);
	this->setData(str_list, meta);
}

void ListStringSignal::wake(){
	this->m_cond.notify_all();
}

bool ListStringSignal::tryClear(){
	if(m_sig_category != SIGNAL_CATEGORY_LIST_STRING_QUEUE){
		EAGLEEYE_LOGE("not liststring-queue mode, dont exec.");
		return false;
	}
	if(this->m_get_then_auto_remove){
		return false;
	}

	std::unique_lock<std::mutex> locker(this->m_mu);
	this->m_queue.front().second -= 1;
	if(this->m_queue.front().second == 0){
		this->m_queue.pop();
		this->m_meta_queue.pop();
		return true;
	}
	return false;
}
} // namespace eagleeye
