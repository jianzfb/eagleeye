#include "eagleeye/processnode/ForNode.h"
namespace eagleeye{
ForNode::ForNode(std::function<AnyNode*()> generator){
    m_auto_node = generator();
    int signal_num = m_auto_node->getNumberOfOutputSignals();
    this->setNumberOfOutputSignals(signal_num);
    for(int signal_i=0; signal_i<signal_num; ++signal_i){
        this->setOutputPort(new GroupSignal(), signal_i);
    }

    EAGLEEYE_MONITOR_VAR(std::string, setFolder, getFolder, "folder","","");
}

ForNode::~ForNode(){
    if(m_auto_node != NULL){
        delete m_auto_node;
    }
}

void ForNode::executeNodeInfo(){
    std::vector<std::vector<AnySignal*>> input_signals;
    input_signals.resize(this->getNumberOfInputSignals());
    for(int i=0; i<input_signals.size(); ++i){
        GroupSignal* i_g_signal = (GroupSignal*)(this->getInputPort(i));
        input_signals[i] = i_g_signal->getData();
    }

    if(input_signals.size() == 0){
        return;
    }
    if(input_signals[0].size() == 0){
        return;
    }

    int input_num = input_signals.size();
    int loop_num = input_signals[0].size();
    for(int loop_i=0; loop_i<loop_num; ++loop_i){
        // input
        for(int input_i=0; input_i<input_num; ++input_i){
            m_auto_node->setInputPort(input_signals[input_i][loop_i], input_i);
        }

        // run
        m_auto_node->start();

        // output
        for(int output_i=0; output_i<this->getNumberOfOutputSignals(); ++output_i){
            GroupSignal* o_g_signal = (GroupSignal*)(this->getOutputPort(output_i));
            std::vector<AnySignal*> temp = o_g_signal->getData();
            temp.push_back(m_auto_node->getOutputPort(output_i));
            o_g_signal->setData(temp);
        }
    }
}

void ForNode::setUnitName(const char* unit_name){ 
    this->m_unit_name=std::string("for-") + unit_name;
    this->m_auto_node->setUnitName(unit_name);
}

void ForNode::setFolder(const std::string folder){
    m_auto_node->setFolder(folder);
}
void ForNode::getFolder(std::string& folder){
    // do nothing
    m_auto_node->getFolder(folder);
}
}