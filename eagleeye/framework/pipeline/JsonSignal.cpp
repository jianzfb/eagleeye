#include "eagleeye/framework/pipeline/JsonSignal.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/common/EagleeyeLog.h"


namespace eagleeye
{
JsonSignal::JsonSignal(std::string record_name, bool is_record_in_message_center){
	this->m_sig_category = SIGNAL_CATEGORY_STRING;
	this->m_max_queue_size = 5;
	this->m_record_name = record_name;
	this->m_is_record_in_message_center = is_record_in_message_center;
	this->setSignalType(EAGLEEYE_SIGNAL_TEXT);
	this->m_get_then_auto_remove = true;
	this->m_set_then_auto_remove = true;
}

JsonSignal::~JsonSignal(){
}

void JsonSignal::copyInfo(AnySignal* sig){
	//call the base class
	Superclass::copyInfo(sig);
}

void JsonSignal::copy(AnySignal* sig, bool is_deep){
	if(sig->getSignalCategory() != SIGNAL_CATEGORY_STRING && 
		sig->getSignalCategory() != SIGNAL_CATEGORY_STRING_QUEUE){
		return;
	}

	JsonSignal* from_sig = (JsonSignal*)(sig);
	MetaData from_data_meta;
	DataType from_data = from_sig->getData(from_data_meta, is_deep);
	this->setData(from_data, from_data_meta);

	// ignore record_name and is_record_in_message_center
}

void JsonSignal::printUnit(){
	Superclass::printUnit();
}

void JsonSignal::makeempty(bool auto_empty){
	// ignore auto_empty
	// TODO, 清空json
	this->m_json_obj.Clear();

	//force time update
	modified();
}

bool JsonSignal::isempty(){
	return this->m_json_obj.IsEmpty();
}

typename JsonSignal::DataType JsonSignal::getData(bool deep_copy){
	// refresh data
	if(this->m_link_node != NULL){
		this->m_link_node->refresh();
	}

	if(this->getSignalCategory() == SIGNAL_CATEGORY_STRING){
		// SIGNAL_CATEGORY_STRING
		if(!(m_json_obj.IsEmpty())){
			m_info = m_json_obj.ToString();
		}
		return m_info;
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

typename JsonSignal::DataType JsonSignal::getData(MetaData& mm, bool deep_copy){
	// refresh data
	if(this->m_link_node != NULL){
		this->m_link_node->refresh();
	}

	if(this->getSignalCategory() == SIGNAL_CATEGORY_STRING){
		// SIGNAL_CATEGORY_STRING
		if(!(m_json_obj.IsEmpty())){
			mm = this->m_meta;
			m_info = m_json_obj.ToString();
		}
		return m_info;
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

typename JsonSignal::DataType JsonSignal::getDataWithId(std::string id, MetaData& mm, bool deep_copy){
	// refresh data
	if(this->m_link_node != NULL){
		this->m_link_node->refresh();
	}

	if(this->getSignalCategory() == SIGNAL_CATEGORY_STRING){
		// SIGNAL_CATEGORY_STRING
		EAGLEEYE_LOGE("Dont support get data with ID for SIGNAL_CATEGORY_STRING");
		mm = MetaData();
		return "";
	}

	// SIGNAL_CATEGORY_STRING_QUEUE
	std::unique_lock<std::mutex> locker(this->m_mu);
	while(this->m_queue.size() == 0){
		this->m_cond.wait(locker);
		if(m_disable)
			break;
		if(this->m_queue.size() == 0)
			continue;

		std::pair<MetaData, int> meta_info = this->m_meta_queue.front();
		if(meta_info.first.id != id)
			continue;

		// 至此，已经存在符合要求数据
		break;
	}
	if(m_disable){
		// 由于失活，产生空数据返回
		locker.unlock();
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


void JsonSignal::setData(JsonSignal::DataType data){
	if(this->getSignalCategory() == SIGNAL_CATEGORY_STRING){
		// SIGNAL_CATEGORY_STRING
		this->m_info = data;
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

void JsonSignal::setData(JsonSignal::DataType data, MetaData mm){
	if(this->getSignalCategory() == SIGNAL_CATEGORY_STRING){
		// SIGNAL_CATEGORY_STRING
		this->m_info = data;
		this->m_meta = mm;
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

bool JsonSignal::tryClear(){
	if(m_sig_category != SIGNAL_CATEGORY_STRING_QUEUE){
		EAGLEEYE_LOGE("not string-queue mode, dont exec.");
		return false;
	}
	if(this->m_get_then_auto_remove){
		return false;
	}

	std::unique_lock<std::mutex> locker(this->m_mu);
	if(this->m_queue.size() == 0){
		return false;
	}

	// 确保队列至少有一个元素
	this->m_queue.front().second -= 1;
	if(this->m_queue.front().second == 0){
		this->m_queue.pop();
		this->m_meta_queue.pop();
		return true;
	}
	return false;
}

void JsonSignal::setKV(std::string key, std::string value){
	m_json_obj.ReplaceAdd(key, value);
}

void JsonSignal::setKList(std::string key, std::vector<int> value){
	if(value.size() > 0){
		neb::CJsonObject obj;
		for(int i=0; i<value.size(); ++i){
			obj.Add(value[i]);
		}

		m_json_obj.ReplaceAdd(key, obj);          
		return;  
	}

	m_json_obj.AddEmptySubArray(key);
}

void JsonSignal::setKList(std::string key, std::vector<float> value){
	if(value.size() > 0){
		neb::CJsonObject obj;
		for(int i=0; i<value.size(); ++i){
			obj.Add(value[i]);
		}

		m_json_obj.ReplaceAdd(key, obj);
		return;
	}

	m_json_obj.AddEmptySubArray(key);
}

void JsonSignal::setKList(std::string key, std::vector<double> value){
	if(value.size() > 0){
		neb::CJsonObject obj;
		for(int i=0; i<value.size(); ++i){
			obj.Add(value[i]);
		}

		m_json_obj.ReplaceAdd(key, obj);
		return;
	}

	m_json_obj.AddEmptySubArray(key);
}    

void JsonSignal::setKList(std::string key, std::vector<std::string> value){
	if(value.size() > 0){
		neb::CJsonObject obj;
		for(int i=0; i<value.size(); ++i){
			obj.Add(value[i]);
		}

		m_json_obj.ReplaceAdd(key, obj);
		return;
	}

	m_json_obj.AddEmptySubArray(key);
}

void JsonSignal::setKT(std::string key, std::vector<float> value, EagleeyeType type, std::vector<int> dims){
	neb::CJsonObject obj;
	neb::CJsonObject data_obj;
	for(int i=0; i<value.size(); ++i){
		data_obj.Add(value[i]);
	}
	obj.Add("data", data_obj);
	obj.Add("type", int(type));
	neb::CJsonObject dims_obj;
	for(int i=0; i<dims.size(); ++i){
		dims_obj.Add(dims[i]);
	}
	obj.Add("dims", dims_obj);
	m_json_obj.ReplaceAdd(key, obj);
}

void JsonSignal::flush(){
	if(m_record_name == ""){
		return;
	}
	if(!m_is_record_in_message_center){
		return;
	}

	std::shared_ptr<Message> message(
		new Message(EagleeyeTime::getCurrentTime()), [](Message* m){delete m;}
	);
	message->copy(m_json_obj.ToString());
	MessageCenter::getInstance()->insert(m_record_name, message);
}

void JsonSignal::setData(void* data, MetaData meta){
	std::string str = *((std::string*)data);
	this->setData(str);
}

void JsonSignal::getSignalContent(void*& data, size_t*& data_size, int& data_dims, int& data_type){
	this->m_tmp_cache = this->getData(false);
	data = &this->m_tmp_cache;
	this->m_data_size[0] = this->m_tmp_cache.size();
	data_size = this->m_data_size;
	data_dims = 1;
	data_type = int(EAGLEEYE_STRING);
}
} // namespace eagleeye
