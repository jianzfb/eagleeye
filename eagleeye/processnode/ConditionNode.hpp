namespace eagleeye{
template<class SrcT>
ConditionNode<SrcT>::ConditionNode()
    :AnyNode("Condition"){
    // set output port
    this->setNumberOfOutputSignals(1);
    this->setOutputPort(new SrcT, 0);

    // set input port number
    // 3 input signals
    // 0 - conditon signal
    // 1 - sig would pass if true
    // 2 - sig woudl pass if false
    this->setNumberOfInputSignals(3);
}    

template<class SrcT>
ConditionNode<SrcT>::~ConditionNode(){

}

template<class SrcT>
void ConditionNode<SrcT>::executeNodeInfo(){
    // get input / output signal
    // 1.step 条件信号
    BooleanSignal* input_sig_1 = (BooleanSignal*)(this->m_input_signals[0]);
    // 2.step this sig would pass if true
    SrcT* true_pass_sig = (SrcT*)(this->m_input_signals[1]);
    // 3.step this sig would pass if false
    SrcT* false_pass_sig = (SrcT*)(this->m_input_signals[2]);
    // 4.step output sig
    SrcT* output_sig = (SrcT*)(this->m_output_signals[0]);

    if(input_sig_1->getData()){
        output_sig->copy(true_pass_sig);
    }
    else{
        output_sig->copy(false_pass_sig);
    }
}

template<class SrcT>
bool ConditionNode<SrcT>::selfcheck(){
    return true;
}

}