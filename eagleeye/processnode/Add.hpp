namespace eagleeye{
template<class SrcT, class TargetT>
Add<SrcT, TargetT>::Add(){
    // 设置初始参数
    this->m_parameter = 5;

    // 设置输出端口（拥有1个输出端口）
    this->setNumberOfOutputSignals(1);
    // 设置输出端口(端口0)及携带数据类型(TargetT)
    this->setOutputPort(new TargetT, 0);

    // 设置输入端口（拥有1个输入端口）
	this->setNumberOfInputSignals(2);
    this->m_adjust = 0.5;

    //add monitor variable
    EAGLEEYE_MONITOR_VAR(int, setParameter, getParameter, "set parm","1","10");
    EAGLEEYE_MONITOR_VAR(bool, setB, getB, "B","","");
    EAGLEEYE_MONITOR_VAR(float, setAdjust,getAdjust,"adjust","0.0","1.0");
}

template<class SrcT, class TargetT>
Add<SrcT, TargetT>::~Add(){

}

template<class SrcT, class TargetT>
void Add<SrcT, TargetT>::executeNodeInfo(){
    // 1.step 取出输入数据
    SrcT* a_sig = dynamic_cast<SrcT*>(this->m_input_signals[0]);
    Matrix<InputPixelType> a = a_sig->getData();

    SrcT* b_sig = dynamic_cast<SrcT*>(this->m_input_signals[1]);
    Matrix<InputPixelType> b = b_sig->getData();

    // 2.step 绑定结果数据到输出端口
    Matrix<InputPixelType> c = a+b;
    TargetT* c_sig = dynamic_cast<TargetT*>(this->m_output_signals[0]);
    c_sig->setData(c);
}

template<class SrcT, class TargetT>
bool Add<SrcT, TargetT>::selfcheck(){
    bool is_ok = Superclass::selfcheck();
    return is_ok;
}

template<class SrcT, class TargetT>
void Add<SrcT, TargetT>::setParameter(int parameter){
    this->m_parameter = parameter;

    // force time update
    this->modified();
}

template<class SrcT, class TargetT>
void Add<SrcT, TargetT>::getParameter(int& parameter){
    parameter = this->m_parameter;
}

template<class SrcT, class TargetT>
void Add<SrcT, TargetT>::setB(bool b){
   this->m_b = b;

    // force time update
    this->modified();
}

template<class SrcT, class TargetT>
void Add<SrcT, TargetT>::getB(bool& b){
   b = this->m_b;
}


template<class SrcT, class TargetT>
void Add<SrcT, TargetT>::setAdjust(float adjust){
   this->m_adjust = adjust;
   this->modified();   
}

template<class SrcT, class TargetT>
void Add<SrcT, TargetT>::getAdjust(float& adjust){
   adjust = this->m_adjust;
}
}