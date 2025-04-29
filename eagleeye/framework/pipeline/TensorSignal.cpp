#include "eagleeye/framework/pipeline/TensorSignal.h"
#include <memory>
#include <string.h>

namespace eagleeye{
TensorSignal::TensorSignal(){
	this->m_sig_category = SIGNAL_CATEGORY_TENSOR;
	this->m_max_queue_size = 5;
	this->m_get_then_auto_remove = true;
	this->setSignalType(EAGLEEYE_SIGNAL_TENSOR);
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
		if(this->m_queue.size() > this->m_max_queue_size){
			this->m_queue.pop();
		}

		this->m_queue.push(std::pair<Tensor,int>{data, int(this->getOutDegree())}); 

		modified();
		// notify
		this->m_cond.notify_all();
	}
}

void TensorSignal::setData(void* data, MetaData meta){
	if(meta.allocate_mode == 1){
		// InPlace Mode
		this->setData(
			Tensor(
				meta.dims,
				EagleeyeType(meta.type),
				DataFormat::AUTO,
				data,
				false)
		);
	}
	else{
		// Copy Mode
		this->setData(
			Tensor(
				meta.dims,
				EagleeyeType(meta.type),
				DataFormat::AUTO,
				data,
				true)
		);
	}
}

void TensorSignal::copy(AnySignal* sig){
	if((sig->getSignalCategory() != SIGNAL_CATEGORY_TENSOR) &&
		(sig->getSignalCategory() != SIGNAL_CATEGORY_TENSOR_QUEUE)){
		return;
	}

	TensorSignal* b_sig = (TensorSignal*)sig;
	this->setData(b_sig->getData());
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

void TensorSignal::wake(){
	this->m_cond.notify_all();
}
} // namespace eagleeye
