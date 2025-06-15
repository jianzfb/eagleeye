#include "eagleeye/processnode/ProxyNode.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/EagleeyeLog.h"
#include <functional>

namespace eagleeye{
ProxyNode::ProxyNode(std::function<AnyNode*()> generator){
    m_func_node = generator();

    int signal_num = m_func_node->getNumberOfOutputSignals();
    this->setNumberOfOutputSignals(signal_num);
    for(int signal_i=0; signal_i<signal_num; ++signal_i){
        AnySignal* output_signal = m_func_node->getOutputPort(signal_i)->make();
        this->setOutputPort(output_signal, signal_i);
    }

    EAGLEEYE_MONITOR_VAR(std::string, setFolder, getFolder, "folder", "", "");
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

void ProxyNode::setCallback(std::string name, std::function<void(AnyNode*, std::vector<AnySignal*>)> callback){
    if(name == ""){
        EAGLEEYE_LOGD("name is empty, not setcallback.");
        return;
    }
    
    std::string next_name = "";
    if (name.find("/") == std::string::npos) {
        next_name = name;
    }
    else{
        std::string separator = "/";
        std::vector<std::string> name_tree = split(name, separator);
        next_name = "";
        for(int i=1; i<name_tree.size(); ++i){
            if(i != name_tree.size() - 1){
                next_name += name_tree[i]+"/";
            }
            else{
                next_name += name_tree[i];
            }
        }
    }
    this->m_func_node->setCallback(next_name, callback);
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

void ProxyNode::setFolder(const std::string folder){
    m_func_node->setFolder(folder);
}
void ProxyNode::getFolder(std::string& folder){
    // do nothing
    m_func_node->getFolder(folder);
}

void ProxyNode::getPipelineMonitors(std::map<std::string,std::vector<AnyMonitor*>>& pipeline_monitor_pool){
    // collect all node monitors in subpipeline
    this->m_func_node->getPipelineMonitors(pipeline_monitor_pool);

	//traverse the whole pipeline
	std::vector<AnySignal*>::iterator signal_iter,signal_iend(m_input_signals.end());
	for (signal_iter = m_input_signals.begin();signal_iter != signal_iend; ++signal_iter)
	{
		if ((*signal_iter))
			(*signal_iter)->getPipelineMonitors(pipeline_monitor_pool);
	}
}

}