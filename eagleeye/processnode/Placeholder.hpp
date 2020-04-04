namespace eagleeye{
template<class T>
Placeholder<T>::Placeholder(bool queue_mode){
	this->setNumberOfOutputSignals(1);
	this->setOutputPort(new T,OUTPUT_PORT_PLACEHOLDER);

    this->m_queue_mode = queue_mode;
    if(this->m_queue_mode){
        this->getOutputPort(OUTPUT_PORT_PLACEHOLDER)->transformCategoryToQ();
    }
}   

template<class T>
Placeholder<T>::~Placeholder(){

}

template<class T>
void Placeholder<T>::setPlaceholderType(SignalType type){
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
        }
        return is_ok;
    }
    else{
        return true;
    }
}

template<class T>
void Placeholder<T>::preexit(){

}

template<class T>
void Placeholder<T>::postexit(){
    if(this->m_queue_mode){
        T* output_img_signal = (T*)(this->getOutputPort(OUTPUT_PORT_PLACEHOLDER));
        // dont care data is what
        output_img_signal->setData(typename T::DataType());
    }
}

template<class T>
void Placeholder<T>::reset(){
    T* output_img_signal = (T*)(this->getOutputPort(OUTPUT_PORT_PLACEHOLDER));
    output_img_signal->makeempty();
}
}