#include "eagleeye/framework/pipeline/TensorSignal.h"
namespace eagleeye{
TensorSignal::TensorSignal(){
	this->m_release_count = 1;
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
	if(auto_empty){
		if(this->m_release_count % this->getOutDegree() != 0){
			this->m_release_count += 1;
			return;
		}
	}

    this->m_data = Tensor();

	if(auto_empty){
		this->m_release_count = 1;
	}

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

			if(this->m_queue.size() > 0){
				break;
			}
        }

		Tensor data = this->m_queue.front();
		if(this->m_get_then_auto_remove){
			this->m_queue.pop();
		}
        locker.unlock();
		return data;
	}	
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
		this->m_queue.push(data); 

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

} // namespace eagleeye
