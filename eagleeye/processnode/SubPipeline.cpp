#include "eagleeye/processnode/SubPipeline.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/framework/pipeline/AnyPipeline.h"
#include "eagleeye/common/EagleeyeLog.h"
namespace eagleeye
{
SubPipeline::SubPipeline(){
    this->m_sink_node = NULL;
    this->m_sink_ignore_port = -1;

    this->m_sink_port_map = NULL;
}

SubPipeline::~SubPipeline(){
    std::map<std::string, AnyNode*>::iterator iter, iend(this->m_subpipeline.end());
    for(iter = this->m_subpipeline.begin(); iter != iend; ++iter){
        delete iter->second;
    }

    if(m_sink_port_map != NULL){
        delete m_sink_port_map;
    }

    std::vector<AnySignal*>::iterator sig_iter, sig_iend(this->m_placeholders.end());
    for(sig_iter = this->m_placeholders.begin(); sig_iter != sig_iend; ++sig_iter){
        delete *sig_iter;
    }
}

void SubPipeline::executeNodeInfo(){
    // 1.step copy input
    if(this->m_placeholders.size() == 0){
        // 基于特殊指定方式绑定输入信号的 内部节点
        std::map<int, std::vector<std::pair<std::string, int>>>::iterator iter,iend(m_special_source_port_map.end());
        for(iter = m_special_source_port_map.begin(); iter != iend; ++iter){
            int from_port = iter->first;
            AnySignal* signal_cp = this->getInputPort(from_port)->make();

            for(int i = 0; i < iter->second.size(); ++i){
                std::string to_node_name = iter->second[i].first;
                int to_node_port = iter->second[i].second;
                this->m_subpipeline[to_node_name]->setInputPort(signal_cp, to_node_port);
            }

            this->m_placeholders.push_back(signal_cp);
        }
    }

    std::map<int, std::vector<std::pair<std::string, int>>>::iterator iter,iend(m_special_source_port_map.end());
    for(iter = m_special_source_port_map.begin(); iter != iend; ++iter){
        int from_port = iter->first;
        AnySignal* input_sig = this->getInputPort(from_port);

        for(int i = 0; i < iter->second.size(); ++i){
            std::string to_node_name = iter->second[i].first;
            int to_node_port = iter->second[i].second;

            this->m_subpipeline[to_node_name]->getInputPort(to_node_port)->copy(input_sig);
            this->m_subpipeline[to_node_name]->getInputPort(to_node_port)->setSignalType(input_sig->getSignalType());
        }
    }

    // 2.step run subpipeline
    this->m_sink_node->start();

    // 3.step copy output
    for(int i = 0; i < this->getNumberOfOutputSignals(); ++i){
        this->getOutputPort(i)->copy(this->m_sink_node->getOutputPort(this->m_sink_port_map[i]));
        this->getOutputPort(i)->setSignalType(this->m_sink_node->getOutputPort(this->m_sink_port_map[i])->getSignalType());
    }
}

void SubPipeline::add(AnyNode* node, std::string name, SubPipelineNode nodetype){
    if(this->m_subpipeline.find(name) == this->m_subpipeline.end()){
        m_subpipeline[name] = node;
        node->setUnitName(name.c_str());
    }

    if(nodetype == SINK_NODE){
       if(this->m_sink_node != NULL){
            EAGLEEYE_LOGE("Sink node has been set.");
            return;
        }

        this->m_sink_node = node;
        this->m_sink_ignore_port = -1;

        int source_output_signal_num = node->getNumberOfOutputSignals();
        if(m_sink_ignore_port < 0){
            this->setNumberOfOutputSignals(source_output_signal_num);
            m_sink_port_map = new int[source_output_signal_num];
            for(int i=0; i<source_output_signal_num; ++i){
                m_sink_port_map[i] = i;
                this->setOutputPort(node->getOutputPort(i)->make(), i);
            }
        }
        else{
            this->setNumberOfOutputSignals(source_output_signal_num-1);
            m_sink_port_map = new int[source_output_signal_num - 1];
            int count = 0;
            for(int i=0; i<source_output_signal_num; ++i){
                if(i != m_sink_ignore_port){
                    m_sink_port_map[count] = i;
                    this->setOutputPort(node->getOutputPort(i)->make(), count);
                    count += 1;
                }
            }
        }
    }
}

void SubPipeline::bind(std::string fromname, int fromport, std::string toname, int toport){
    if(m_subpipeline.find(toname) == m_subpipeline.end()){
        EAGLEEYE_LOGE("Node %s not int subpipeline.", toname.c_str());
        return;
    }

    if(fromname == "UPPER" || fromname == "SOURCE"){
        if(m_special_source_port_map.find(fromport) == m_special_source_port_map.end()){
            m_special_source_port_map[fromport] = std::vector<std::pair<std::string, int>>();
        }

        m_special_source_port_map[fromport].push_back(std::pair<std::string, int>(toname, toport));
        AnyNode* node_to_ptr = this->m_subpipeline[std::string(toname)];
        if(node_to_ptr->getNumberOfInputSignals() < toport + 1){
            node_to_ptr->setNumberOfInputSignals(toport + 1);
        }

        if(this->getNumberOfInputSignals() < fromport + 1){
            this->setNumberOfInputSignals(fromport + 1);
        }
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
}

void SubPipeline::reset(){
    this->m_sink_node->reset();

    Superclass::reset();
}

void SubPipeline::init(){
    Superclass::init();

    // 初始化子管线
    this->m_sink_node->init();
}

void SubPipeline::exit(){
    this->m_sink_node->exit();

    Superclass::exit();
}

void SubPipeline::getPipelineMonitors(std::map<std::string,std::vector<AnyMonitor*>>& pipeline_monitor_pool){
    std::map<std::string,std::vector<AnyMonitor*>> temp;
    this->m_sink_node->getPipelineMonitors(temp);
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
	for (signal_iter = m_input_signals.begin();signal_iter != signal_iend; ++signal_iter)
	{
		if ((*signal_iter))
			(*signal_iter)->getPipelineMonitors(pipeline_monitor_pool);
	}
}

void SubPipeline::loadConfigure(std::map<std::string, std::shared_ptr<char>> nodes_config){
    this->m_sink_node->loadConfigure(nodes_config);
    Superclass::loadConfigure(nodes_config);
}

void SubPipeline::saveConfigure(std::map<std::string, std::shared_ptr<char>>& nodes_config){
    this->m_sink_node->saveConfigure(nodes_config);
    Superclass::saveConfigure(nodes_config);
}
} // namespace eagleeye
