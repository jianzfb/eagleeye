namespace eagleeye{
template<class T>
DataSourceNode<T>::DataSourceNode(bool queue_mode)
        :ImageIONode<T>("DATASOURCE"){
	this->setNumberOfOutputSignals(1);
	this->setOutputPort(this->makeOutputSignal(),OUTPUT_PORT_IMAGE_DATA);

    this->m_queue_mode = queue_mode;
    if(this->m_queue_mode){
        this->getOutputPort(OUTPUT_PORT_IMAGE_DATA)->transformCategoryToQ();
    }
}   

template<class T>
DataSourceNode<T>::~DataSourceNode(){

}

template<class T>
void DataSourceNode<T>::setSourceType(SignalType type){
    this->getOutputPort(OUTPUT_PORT_IMAGE_DATA)->setSignalType(type);
}

template<class T>
void DataSourceNode<T>::setSourceTarget(SignalTarget target){
    this->getOutputPort(OUTPUT_PORT_IMAGE_DATA)->setSignalTarget(target);
}

template<class T>
void DataSourceNode<T>::setSourceData(Matrix<OutputPixelType> data){
    T* output_img_signal = dynamic_cast<T*>(this->getOutputPort(OUTPUT_PORT_IMAGE_DATA));
    output_img_signal->setData(data);
    //force time to update
	this->modified();
}

template<class T>
void DataSourceNode<T>::executeNodeInfo(){
    // do nothing
}

template<class T>
bool DataSourceNode<T>::selfcheck(){
    if(!this->m_queue_mode){
        bool is_ok = ImageIONode<T>::selfcheck();
        if(this->getOutputPort(OUTPUT_PORT_IMAGE_DATA)->isempty()){
            is_ok = false;
        }
        return is_ok;
    }
    else{
        return true;
    }

}

template<class T>
void DataSourceNode<T>::preexit(){

}

template<class T>
void DataSourceNode<T>::postexit(){
    if(this->m_queue_mode){
        T* output_img_signal = (T*)(this->getOutputPort(OUTPUT_PORT_IMAGE_DATA));
        // dont care data is what
        output_img_signal->setData(Matrix<OutputPixelType>());
    }
}
}