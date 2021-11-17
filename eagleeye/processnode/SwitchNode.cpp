#include "eagleeye/processnode/SwitchNode.h"
#include "eagleeye/framework/pipeline/StateSignal.h"
#include "eagleeye/common/EagleeyeLog.h"
namespace eagleeye
{
SwitchNode::SwitchNode(std::vector<AnyNode*> candidates){
    // 设置输入端口
	this->setNumberOfInputSignals(1);

    // 设置输出端口（拥有1个输出端口）
    int x_output_signal_num = 0;
    int x_i;
    for(int i=0; i<candidates.size(); ++i){
        if(x_output_signal_num < candidates[i]->getNumberOfOutputSignals()){
            x_output_signal_num = candidates[i]->getNumberOfOutputSignals();
            x_i = i;
        }
    }
    this->setNumberOfOutputSignals(x_output_signal_num);
    for(int i=0; i<x_output_signal_num; ++i){
        this->setOutputPort(candidates[x_i]->getOutputPort(i)->make(), i);
    }

    this->m_candidates = candidates;
}

SwitchNode::~SwitchNode(){
    for(int i=0; i<this->m_candidates.size(); ++i){
        delete this->m_candidates[i];
    }
    this->m_candidates.clear();
} 

void SwitchNode::executeNodeInfo(){
    if(this->getInputPort(0)->getSignalType() != EAGLEEYE_SIGNAL_STATE){
        EAGLEEYE_LOGD("Dont support input port type.");
        return;
    }

    StateSignal* input_sig = (StateSignal*)this->getInputPort(0);
    int state = input_sig->getData();
    if(state < 0 || state >= this->m_candidates.size()){
        EAGLEEYE_LOGD("Invalid index.");
        return;
    }
    AnyNode* node = this->m_candidates[state];

    int input_sig_num = this->getNumberOfInputSignals();
    if(input_sig_num == 1){
        return;
    }
    std::vector<AnySignal*> placeholders(input_sig_num - 1);
    for(int i=1; i<input_sig_num; ++i){  
        placeholders[i-1] = this->getInputPort(i)->make();
        placeholders[i-1]->copy(this->getInputPort(i));
        placeholders[i-1]->setSignalType(this->getInputPort(i)->getSignalType());
    }

    for(int i=0; i<placeholders.size(); ++i){
        node->setInputPort(placeholders[i], i);
    }

    // run node
    node->start();

    int output_sig_num = this->getNumberOfOutputSignals();
    output_sig_num = output_sig_num < node->getNumberOfOutputSignals() ? output_sig_num : node->getNumberOfOutputSignals();
    for(int i=0; i<output_sig_num; ++i){
        this->getOutputPort(i)->copy(node->getOutputPort(i));
        this->getOutputPort(i)->setSignalType(node->getOutputPort(i)->getSignalType());
    }

    // delete sig;
    for(int i=1; i<input_sig_num; ++i){
        // 清空输出端口设置
        node->clearInputPort(i-1);
        // 删除占位信号
        delete placeholders[i-1];
    }
}

void SwitchNode::init(){
    Superclass::init();
    for(int i=0; i<m_candidates.size(); ++i){
        m_candidates[i]->init();
    }
}

void SwitchNode::reset(){
    for(int i=0; i<m_candidates.size(); ++i){
        m_candidates[i]->reset();
    }
    Superclass::reset();
}

void SwitchNode::exit(){
    for(int i=0; i<m_candidates.size(); ++i){
        m_candidates[i]->exit();
    }
    Superclass::exit();
}

void SwitchNode::getPipelineMonitors(std::map<std::string,std::vector<AnyMonitor*>>& pipeline_monitor_pool){
    // collect all node monitors in subpipeline
    std::map<std::string,std::vector<AnyMonitor*>> temp;
    for(int i=0; i<m_candidates.size(); ++i){
        m_candidates[i]->getPipelineMonitors(temp);
    }
    // 给temp添加更新时的前置回调函数
    std::map<std::string,std::vector<AnyMonitor*>>::iterator monitor_iter, monitor_iend(temp.end());
    for(monitor_iter = temp.begin(); monitor_iter != monitor_iend; ++monitor_iter){
        for(int i=0; i<monitor_iter->second.size(); ++i){
            monitor_iter->second[i]->setPrefixCallback(
                [this](){
                    this->modified();
                }
            );
        }
    }
    pipeline_monitor_pool.insert(temp.begin(), temp.end());
	
    //traverse the whole pipeline
	std::vector<AnySignal*>::iterator signal_iter,signal_iend(m_input_signals.end());
	for (signal_iter = m_input_signals.begin();signal_iter != signal_iend; ++signal_iter){
		if ((*signal_iter))
			(*signal_iter)->getPipelineMonitors(pipeline_monitor_pool);
	}
}


} // namespace eagleeye
