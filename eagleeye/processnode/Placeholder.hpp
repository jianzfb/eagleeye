namespace eagleeye{
template<class T>
Placeholder<T>::Placeholder(bool queue_mode, int queue_size){
	this->setNumberOfOutputSignals(1);
	this->setOutputPort(new T,OUTPUT_PORT_PLACEHOLDER);

    this->m_queue_mode = queue_mode;
    if(this->m_queue_mode){
        this->getOutputPort(OUTPUT_PORT_PLACEHOLDER)->transformCategoryToQ(queue_size);
    }
}

template<class T>
Placeholder<T>::~Placeholder(){

}

template<class T>
void Placeholder<T>::setPlaceholderSignalType(SignalType type){
    this->getOutputPort(OUTPUT_PORT_PLACEHOLDER)->setSignalType(type);
}

template<class T>
void Placeholder<T>::setPlaceholderSource(SignalTarget target){
    this->getOutputPort(OUTPUT_PORT_PLACEHOLDER)->setSignalTarget(target);
}

template<class T>
void Placeholder<T>::executeNodeInfo(){
    // do nothing
}

template<class T>
bool Placeholder<T>::selfcheck(){
    if(!this->m_queue_mode){
        bool is_ok = true;
        if(this->getOutputPort(OUTPUT_PORT_PLACEHOLDER)->isempty()){
            is_ok = false;
            EAGLEEYE_LOGD("placeholder %s is empty", this->getUnitName());
        }
        return is_ok;
    }
    else{
        return true;
    }
}
}