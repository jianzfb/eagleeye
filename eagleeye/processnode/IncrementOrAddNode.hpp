namespace eagleeye{
template<class SrcT, class TargetT>
IncrementOrAddNode<SrcT, TargetT>::IncrementOrAddNode(int input_port_num, int output_port_num){
    // 设置初始参数
    this->m_parameter = 5;

    // 设置输出端口（拥有1个输出端口）
    this->setNumberOfOutputSignals(output_port_num);
    // 设置输出端口(端口0)及携带数据类型(TargetT)
    for(int i=0; i<output_port_num; ++i){
        this->setOutputPort(new TargetT, i);
    }

    // 设置输入端口（拥有1个输入端口）
	this->setNumberOfInputSignals(input_port_num);

    this->m_adjust = 0.5;

    //add monitor variable
    EAGLEEYE_MONITOR_VAR(int, setParameter, getParameter, "set parm","1","10");
    EAGLEEYE_MONITOR_VAR(bool, setB, getB, "B","","");
    EAGLEEYE_MONITOR_VAR(float, setAdjust,getAdjust,"adjust","0.0","1.0");
}

template<class SrcT, class TargetT>
IncrementOrAddNode<SrcT, TargetT>::~IncrementOrAddNode(){

}

template<class SrcT, class TargetT>
void IncrementOrAddNode<SrcT, TargetT>::executeNodeInfo(){
    // 1.step 取出输入数据
    Matrix<InputPixelType> input;
    for(int i=0; i<this->getNumberOfInputSignals(); ++i){
        if(i == 0){
            SrcT* input_img_sig = dynamic_cast<SrcT*>(this->m_input_signals[i]);
            input = input_img_sig->getData().clone(); 
        }
        else{
            SrcT* input_img_sig = dynamic_cast<SrcT*>(this->m_input_signals[i]);
            Matrix<InputPixelType> c = input_img_sig->getData();
             input = input + c;
        }
    }

    // 2.step 绑定结果数据到输出端口
    for(int i=0; i<this->getNumberOfOutputSignals(); ++i){
        TargetT* label_image_signal = dynamic_cast<TargetT*>(this->m_output_signals[i]);
        label_image_signal->setData(input.clone());
    }
}

template<class SrcT, class TargetT>
bool IncrementOrAddNode<SrcT, TargetT>::selfcheck(){
    bool is_ok = Superclass::selfcheck();
    return is_ok;
}

template<class SrcT, class TargetT>
void IncrementOrAddNode<SrcT, TargetT>::setParameter(int parameter){
    this->m_parameter = parameter;

    // force time update
    this->modified();
}

template<class SrcT, class TargetT>
void IncrementOrAddNode<SrcT, TargetT>::getParameter(int& parameter){
    parameter = this->m_parameter;
}

template<class SrcT, class TargetT>
void IncrementOrAddNode<SrcT, TargetT>::setB(bool b){
   this->m_b = b;
}

template<class SrcT, class TargetT>
void IncrementOrAddNode<SrcT, TargetT>::getB(bool& b){
   b = this->m_b;
}


template<class SrcT, class TargetT>
void IncrementOrAddNode<SrcT, TargetT>::setAdjust(float adjust){
   this->m_adjust = adjust;
   this->modified();   
}

template<class SrcT, class TargetT>
void IncrementOrAddNode<SrcT, TargetT>::getAdjust(float& adjust){
   adjust = this->m_adjust;
}
}