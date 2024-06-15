#include "eagleeye/processnode/SubPipeline.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/framework/pipeline/AnyPipeline.h"
#include "eagleeye/common/EagleeyeLog.h"
namespace eagleeye
{
SubPipeline::SubPipeline(){}

SubPipeline::~SubPipeline(){
    std::map<std::string, AnyNode*>::iterator iter, iend(this->m_subpipeline.end());
    for(iter = this->m_subpipeline.begin(); iter != iend; ++iter){
        delete iter->second;
    }

    std::vector<AnySignal*>::iterator sig_iter, sig_iend(this->m_placeholders.end());
    for(sig_iter = this->m_placeholders.begin(); sig_iter != sig_iend; ++sig_iter){
        delete *sig_iter;
    }
}

void SubPipeline::executeNodeInfo(){
    // 1.step copy input
    if(this->m_placeholders.size() == 0){
        int input_port_i = 0;
        for(int i=0; i<m_input_node_name_list.size(); ++i){
            std::string name = m_input_node_name_list[i];
            for(int j=0; j<m_subpipeline[name]->getNumberOfInputSignals(); ++j){
                AnySignal* signal_cp = this->getInputPort(input_port_i)->make();
                this->m_subpipeline[name]->setInputPort(signal_cp, j);
                this->m_placeholders.push_back(signal_cp);
                input_port_i += 1;
            }
        }
    }

    for(int i=0; i<m_placeholders.size(); ++i){
        m_placeholders[i]->copy(this->getInputPort(i));
    }

    // 2.step run subpipeline
    for(int i=0; i<m_output_node_name_list.size(); ++i){
        std::string name = m_output_node_name_list[i];
        m_subpipeline[name]->start();
    }

    // 3.step copy output
    int output_port_i = 0;
    for(int i=0; i<m_output_node_name_list.size(); ++i){
        std::string name = m_output_node_name_list[i];
        for(int j=0; j<m_subpipeline[name]->getNumberOfOutputSignals(); ++j){
            this->getOutputPort(output_port_i)->copy(m_subpipeline[name]->getOutputPort(j));
            this->getOutputPort(output_port_i)->setSignalType(m_subpipeline[name]->getOutputPort(j)->getSignalType());
            output_port_i += 1;
        }
    }
}

void SubPipeline::add(AnyNode* node, std::string name){
    if(this->m_subpipeline.find(name) == this->m_subpipeline.end()){
        m_subpipeline[name] = node;
        node->setUnitName(name.c_str());
        m_name_list.push_back(name);
    }

    m_node_bind_input_num[name] = 0;
    m_node_bind_output_num[name] = 0;
}

void SubPipeline::bind(std::string fromname, int fromport, std::string toname, int toport){
    if(m_subpipeline.find(toname) == m_subpipeline.end()){
        EAGLEEYE_LOGE("Node %s not int subpipeline.", toname.c_str());
        return;
    }
    if(m_subpipeline.find(fromname) == m_subpipeline.end()){
        EAGLEEYE_LOGE("Node %s not int subpipeline.", fromname.c_str());
        return;
    }

    AnyNode* node_to_ptr = this->m_subpipeline[std::string(toname)];
    AnyNode* node_from_ptr = this->m_subpipeline[std::string(fromname)];
    if(node_to_ptr->getNumberOfInputSignals() < toport + 1){
        node_to_ptr->setNumberOfInputSignals(toport + 1);
    }
    node_to_ptr->setInputPort(node_from_ptr->getOutputPort(fromport), toport);

    m_node_bind_output_num[fromname] += 1;
    m_node_bind_input_num[toname] += 1;
}

void SubPipeline::analyze(){
    int total_input_signal_num = 0;
    int total_output_signal_num = 0;
    m_input_node_name_list.clear();
    m_output_node_name_list.clear();
    for(int i=0; i<m_name_list.size(); ++i){
        std::string node_name = m_name_list[i];
        if(m_node_bind_input_num[node_name] == 0){
            // 获得输入节点相关信息
            int input_signal_num = this->m_subpipeline[node_name]->getNumberOfInputSignals();
            total_input_signal_num += input_signal_num;
            m_input_node_name_list.push_back(node_name);

            this->setNumberOfInputSignals(total_input_signal_num);
        }

        if(m_node_bind_output_num[node_name] == 0){
            // 获得输出节点相关信息
            int ouput_signal_num = this->m_subpipeline[node_name]->getNumberOfOutputSignals();
            total_output_signal_num += ouput_signal_num;
            m_output_node_name_list.push_back(node_name);
            
            int subpipeline_output_signal_num = this->getNumberOfOutputSignals();
            this->setNumberOfOutputSignals(total_output_signal_num);
            for(int sig_i=subpipeline_output_signal_num; sig_i<total_output_signal_num; ++sig_i){
                int node_sig_i = sig_i-subpipeline_output_signal_num;
                this->setOutputPort(this->m_subpipeline[node_name]->getOutputPort(node_sig_i)->make(), sig_i);
            }
        }
    }
}

void SubPipeline::reset(){
    for(int i=0; i<m_output_node_name_list.size(); ++i){
        std::string name = m_output_node_name_list[i];
        m_subpipeline[name]->reset();
    }
    Superclass::reset();
}

void SubPipeline::init(){
    Superclass::init();

    // 初始化子管线
    for(int i=0; i<m_output_node_name_list.size(); ++i){
        std::string name = m_output_node_name_list[i];
        m_subpipeline[name]->init();
    }
}

void SubPipeline::exit(){
    for(int i=0; i<m_output_node_name_list.size(); ++i){
        std::string name = m_output_node_name_list[i];
        m_subpipeline[name]->exit();
    }
    Superclass::exit();
}

void SubPipeline::getPipelineMonitors(std::map<std::string,std::vector<AnyMonitor*>>& pipeline_monitor_pool){
    for(int i=0; i<m_output_node_name_list.size(); ++i){
        std::string name = m_output_node_name_list[i];
        m_subpipeline[name]->getPipelineMonitors(pipeline_monitor_pool);
    }

	//traverse the whole pipeline
	std::vector<AnySignal*>::iterator signal_iter,signal_iend(m_input_signals.end());
	for (signal_iter = m_input_signals.begin();signal_iter != signal_iend; ++signal_iter){
		if ((*signal_iter))
			(*signal_iter)->getPipelineMonitors(pipeline_monitor_pool);
	}
}

} // namespace eagleeye
