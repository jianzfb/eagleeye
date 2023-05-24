#include "eagleeye/framework/pipeline/TensorSignal.h"
namespace eagleeye
{
TensorSignal::TensorSignal(){this->m_release_count = 1;}
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
    return this->m_data.empty();
}

typename TensorSignal::DataType TensorSignal::getData(){
	return this->m_data;
}

void TensorSignal::setData(TensorSignal::DataType data){
	this->m_data = data;
	modified();
}

void TensorSignal::copy(AnySignal* sig){
	if(sig->getSignalCategory() == SIGNAL_CATEGORY_TENSOR){
		TensorSignal* b_sig = (TensorSignal*)sig;
		this->setData(b_sig->getData());
	}
}

void TensorSignal::getSignalContent(void*& data, int* data_size, int& data_dims, int& data_type){
    data = (void*)this->m_data.cpu();
    data_dims = this->m_data.dims().size();
    data_type = this->m_data.type();
    for(int i=0; i<data_dims; ++i){
        data_size[i] = this->m_data.dims()[i];
    }
}

} // namespace eagleeye
