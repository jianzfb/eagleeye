eagleeye/framework/pipeline/TensorSignal.cpp#include "eagleeye/framework/pipeline/TensorSignal.h"
#include "eagleeye/common/EagleeyeLog.h"
#include <memory>
#include <string.h>

namespace eagleeye{
TensorSignal::TensorSignal(){
	this->m_sig_category = SIGNAL_CATEGORY_TENSOR;
	this->m_max_queue_size = 5;
	this->m_get_then_auto_remove = true;
	this->setSignalType(EAGLEEYE_SIGNAL_TENSOR);
	m_get_then_auto_remove = true;
	m_set_then_auto_remove = true;

	record_count = -1;
}
TensorSignal::~TensorSignal(){} 

void TensorSignal::copyInfo(AnySignal* sig){
	//call the base class
	Superclass::copyInfo(sig);
}

void TensorSignal::printUnit(){
	Superclass::printUnit();
}

void TensorSignal::makeempty(bool auto_empty){
	// ignore auto_empty
	// only for non-queue
    this->m_data = Tensor();

	//force time update
	modified();
}

bool TensorSignal::isempty(){
	if(this->getSignalCategory() == SIGNAL_CATEGORY_TENSOR){
	    return this->m_data.empty();
	}
	else{
		return false;
	}
}

typename TensorSignal::DataType TensorSignal::getData(){
	if(this->getSignalCategory() == SIGNAL_CATEGORY_TENSOR){
		// 非队列模式
		return m_data;
	}
	else{
		// 队列模式
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
			return Tensor();
		}

		std::pair<Tensor, int> data_info = this->m_queue.front();
		Tensor data = data_info.first;
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

typename TensorSignal::DataType TensorSignal::getData(MetaData& mm){
	if(this->getSignalCategory() == SIGNAL_CATEGORY_TENSOR){
		// 非队列模式
		mm = this->m_meta;
		return m_data;
	}
	else{
		// 队列模式
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
			return Tensor();
		}

		std::pair<Tensor, int> data_info = this->m_queue.front();
		Tensor data = data_info.first;
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

std::vector<std::string> TensorSignal::getString(){
	Tensor tensor = this->getData();
	if(tensor.empty()){
		return std::vector<std::string>();
	}
	if((tensor.type() != EAGLEEYE_UINT8 && tensor.type() != EAGLEEYE_INT8 && tensor.type() != EAGLEEYE_UCHAR && tensor.type() != EAGLEEYE_CHAR) || (tensor.dims().size() != 1 && tensor.dims().size() != 2)){
		return std::vector<std::string>();
	}

	if(tensor.dims().size() == 1){
		char* str_info = tensor.cpu<char>();
		int str_len = tensor.dims().production();
		char* str_info_w_eof = (char*)malloc(str_len+1);
    	memset(str_info_w_eof, '\0', str_len+1);
    	memcpy(str_info_w_eof, str_info, str_len);
		std::string str(str_info_w_eof);
		free(str_info_w_eof);
		return {str};
	}

	std::vector<std::string> out_list;
	int str_num = tensor.dims()[0];
	int str_len = tensor.dims()[1];
	char* str_info_w_eof = (char*)malloc(str_len+1);
	for(int str_i=0; str_i<str_num; ++str_i){
		char* str_info = tensor.cpu<char>() + str_i * str_len;
		memset(str_info_w_eof, '\0', str_len+1);
		memcpy(str_info_w_eof, str_info, str_len);
		std::string str(str_info_w_eof);
		out_list.push_back(str);
	}
	free(str_info_w_eof);

	return out_list;
}

void TensorSignal::setData(TensorSignal::DataType data){
	if(this->getSignalCategory() == SIGNAL_CATEGORY_TENSOR){
		// 非队列模式
		this->m_data = data;
		modified();
	}
	else{
		// 队列模式
		std::unique_lock<std::mutex> locker(this->m_mu);
		if(this->m_queue.size() > this->m_max_queue_size && m_set_then_auto_remove){
			// TODO, 非去除模式，需要阻塞
			this->m_queue.pop();
			this->m_meta_queue.pop();
		}

		this->m_queue.push(std::pair<Tensor,int>{data, int(this->getOutDegree())}); 
		MetaData mm;
		this->m_meta_queue.push(std::pair<MetaData, int>{mm, this->getOutDegree()});
		locker.unlock();
	
		modified();
		// notify
		this->m_cond.notify_all();
	}
}

void TensorSignal::setData(TensorSignal::DataType data, MetaData meta){
	if(this->getSignalCategory() == SIGNAL_CATEGORY_TENSOR){
		// 非队列模式
		this->m_data = data;
		this->m_meta = meta;
		modified();
	}
	else{
		// 队列模式
		std::unique_lock<std::mutex> locker(this->m_mu);
		if(this->m_queue.size() > this->m_max_queue_size && m_set_then_auto_remove){
			// TODO, 非去除模式，需要阻塞
			this->m_queue.pop();
			this->m_meta_queue.pop();
		}

		this->m_queue.push(std::pair<Tensor,int>{data, int(this->getOutDegree())}); 
		this->m_meta_queue.push(std::pair<MetaData, int>{meta, this->getOutDegree()});
		locker.unlock();

		modified();
		// notify
		this->m_cond.notify_all();
	}
}

void TensorSignal::setData(void* data, MetaData meta){
	Tensor t;
	if(meta.allocate_mode == 1){
		// InPlace Mode
		t = Tensor(
			meta.dims,
			EagleeyeType(meta.type),
			DataFormat::AUTO,
			data,
			false);
	}
	else{
		// Copy Mode
		t = Tensor(
			meta.dims,
			EagleeyeType(meta.type),
			DataFormat::AUTO,
			data,
			true);
	}
	this->setData(t, meta);
}

void TensorSignal::copy(AnySignal* sig, bool is_deep){
	if((sig->getSignalCategory() != SIGNAL_CATEGORY_TENSOR) &&
		(sig->getSignalCategory() != SIGNAL_CATEGORY_TENSOR_QUEUE)){
		return;
	}

	TensorSignal* b_sig = (TensorSignal*)sig;
	MetaData from_data_meta;
	Tensor from_data = b_sig->getData(from_data_meta);
	if(is_deep){
		from_data = from_data.clone();
	}
	this->setData(from_data, from_data_meta);
}

void TensorSignal::getSignalContent(void*& data, size_t*& data_size, int& data_dims, int& data_type){
	// 获得数据
	Tensor tensor_data = this->getData();
	m_tmp = tensor_data;

	// 导出
    data = (void*)m_tmp.cpu();
    data_dims = m_tmp.dims().size();
    data_type = m_tmp.type();
	data_size = (size_t*)(&(m_tmp.dims().data()[0]));
}

void TensorSignal::getSignalContent(void*& data, size_t*& data_size, int& data_dims, int& data_type, MetaData& data_meta){
	// 获得数据
	Tensor tensor_data = this->getData(data_meta);
	m_tmp = tensor_data;

	// 导出
    data = (void*)m_tmp.cpu();
    data_dims = m_tmp.dims().size();
    data_type = m_tmp.type();
	data_size = (size_t*)(&(m_tmp.dims().data()[0]));
}

bool TensorSignal::tryClear(){
	if(m_sig_category != SIGNAL_CATEGORY_TENSOR_QUEUE){
		EAGLEEYE_LOGE("not tensor-queue mode, dont exec.");
		return false;
	}
	if(this->m_get_then_auto_remove){
		return false;
	}

	std::unique_lock<std::mutex> locker(this->m_mu);
	this->m_queue.front().second -= 1;

	if(record_count == -1){
		record_count = this->getOutDegree();
	}
	record_count -= 1;
	if(this->m_queue.front().second == 0){
		this->m_queue.pop();
		this->m_meta_queue.pop();
		record_count = this->getOutDegree();
		return true;
	}
	return false;
}

void TensorSignal::wake(){
	this->m_cond.notify_all();
}
} // namespace eagleeye
