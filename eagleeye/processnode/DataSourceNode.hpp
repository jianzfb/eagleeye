namespace eagleeye{
template<class T>
DataSourceNode<T>::DataSourceNode(const char* unit_name, const char* type, const char* source)
        :ImageIONode<T>(unit_name){
	this->setNumberOfOutputSignals(1);
	this->setOutputPort(this->makeOutputSignal(),OUTPUT_PORT_IMAGE_DATA);

    this->getOutputPort(OUTPUT_PORT_IMAGE_DATA)->setSignalType(type);
    this->getOutputPort(OUTPUT_PORT_IMAGE_DATA)->setSignalTarget(source);
    this->m_enable_empty = false;
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
    if(m_enable_empty){
        return true;
    }

    bool is_ok = ImageIONode<T>::selfcheck();
    if(this->getOutputPort(OUTPUT_PORT_IMAGE_DATA)->isempty()){
        is_ok = false;
    }
    return is_ok;
}

template<class T>
void DataSourceNode<T>::enableEmpty(){
    this->m_enable_empty = true;
}
template<class T>
void DataSourceNode<T>::disableEmpty(){
    this->m_enable_empty = false;
}
}