#include "eagleeye/processnode/ProxyNode.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/EagleeyeLog.h"

namespace eagleeye{
ProxyNode::ProxyNode(std::function<AnyNode*()> generator){
    m_func_node = generator();

    int signal_num = m_func_node->getNumberOfOutputSignals();
    this->setNumberOfOutputSignals(signal_num);
    for(int signal_i=0; signal_i<signal_num; ++signal_i){
        AnySignal* output_signal = m_func_node->getOutputPort(signal_i)->make();
        this->setOutputPort(output_signal, signal_i);
    }
}

ProxyNode::~ProxyNode(){
    // 删除内部节点
    if(m_func_node != NULL){
        delete m_func_node;
        m_func_node = NULL;
    }

    // 删除缓存信号
    for(int signal_i=0; signal_i < m_cache_signals.size(); ++signal_i){
        delete m_cache_signals[signal_i];
    }
}

void ProxyNode::setUnitName(const char* unit_name){
    this->m_unit_name=std::string("proxy-") + unit_name;
    this->m_func_node->setUnitName(unit_name);
}

void ProxyNode::executeNodeInfo(){
    if(m_cache_signals.size() == 0){
        for(int signal_i = 0; signal_i<this->getNumberOfInputSignals(); ++signal_i){
            AnySignal* signal_cp = this->getInputPort(signal_i)->make();
            m_func_node->setInputPort(signal_cp, signal_i);        
            m_cache_signals.push_back(signal_cp);
        }
    }

    // fill input
    int signal_num = this->getNumberOfInputSignals();
    for(int signal_i = 0; signal_i<signal_num; ++signal_i){
        m_cache_signals[signal_i]->copy(this->getInputPort(signal_i));        
    }
    // run node
    this->m_func_node->start();
    // get output
    signal_num = m_func_node->getNumberOfOutputSignals();
    for(int signal_i = 0; signal_i<signal_num; ++signal_i){
        this->getOutputPort(signal_i)->copy(m_func_node->getOutputPort(signal_i));
    }
}

void ProxyNode::setNumberOfInputSignals(unsigned int inputnum){
    AnyNode::setNumberOfInputSignals(inputnum);
    this->m_func_node->setNumberOfInputSignals(inputnum);
}
}