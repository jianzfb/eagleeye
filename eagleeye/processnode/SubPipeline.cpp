#include "eagleeye/processnode/SubPipeline.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/framework/pipeline/AnyPipeline.h"
#include "eagleeye/common/EagleeyeLog.h"
namespace eagleeye
{
SubPipeline::SubPipeline(){
    this->m_source_node = NULL;
    this->m_source_ignore_port = -1;
    this->m_sink_node = NULL;
    this->m_sink_ignore_port = -1;

    this->m_source_port_map = NULL;
    this->m_sink_port_map = NULL;
}

SubPipeline::~SubPipeline(){
    std::map<std::string, AnyNode*>::iterator iter, iend(this->m_subpipeline.end());
    for(iter=this->m_subpipeline.begin(); iter!=iend; ++iter){
        delete iter->second;
    }

    if(m_source_port_map != NULL){
        delete m_source_port_map;
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
        if(m_special_source_port_map.size() == 0){
            // 基于默认方式绑定输入信号到 内部节点
            for(int i=0; i<this->getNumberOfInputSignals(); ++i){
                AnySignal* signal_cp = this->getInputPort(i)->make();
                this->m_source_node->setInputPort(signal_cp, this->m_source_port_map[i]);
                this->m_placeholders.push_back(signal_cp);
            }
        }
        else{
            // 基于特殊指定方式绑定输入信号的 内部节点
            for(int i=0; i<this->getNumberOfInputSignals(); ++i){
                AnySignal* signal_cp = this->getInputPort(i)->make();
                std::string to_node_name = m_special_source_port_map[i].first;
                int to_node_port = m_special_source_port_map[i].second;
                m_subpipeline[to_node_name]->setInputPort(signal_cp,to_node_port);
                this->m_placeholders.push_back(signal_cp);
            }
        }
    }
    for(int i=0; i<this->getNumberOfInputSignals(); ++i){
        this->m_placeholders[i]->copy(this->getInputPort(i));
    }

    // 2.step run subpipeline
    this->m_sink_node->start();

    // 3.step copy output
    for(int i=0; i<this->getNumberOfOutputSignals(); ++i){
        this->getOutputPort(i)->copy(this->m_sink_node->getOutputPort(this->m_sink_port_map[i]));
    }
}

void SubPipeline::add(AnyNode* node, std::string name, SubPipelineNode nodetype, int dontcare){
    if(this->m_subpipeline.find(name) == this->m_subpipeline.end()){
        m_subpipeline[name] = node;
        node->setUnitName(name.c_str());
    }
    
    if(nodetype == SOURCE_NODE){
        if(this->m_source_node != NULL){
            EAGLEEYE_LOGE("source node has been set");
            return;
        }

        this->m_source_node = node;
        this->m_source_ignore_port = dontcare;
        
        int source_input_signal_num = node->getNumberOfInputSignals();
        if(dontcare < 0){
            // 默认方式，将subpipeline接入的所有信号传递给SOURCE节点
            this->setNumberOfInputSignals(source_input_signal_num);
            m_source_port_map = new int[source_input_signal_num];
            for(int i=0; i<source_input_signal_num; ++i){
                m_source_port_map[i] = i;
            }
        }
        else{
            // 默认方式，将subpipeline接入的所有信号传递给SOURCE节点（忽略dontcare port）
            this->setNumberOfInputSignals(source_input_signal_num - 1);
            m_source_port_map = new int[source_input_signal_num - 1];
            int count = 0;
            for(int i=0; i<source_input_signal_num; ++i){
                if(i != dontcare){
                    m_source_port_map[count] = i;
                    count += 1;
                }
            }
        }
    }
    else if(nodetype == SINK_NODE){
       if(this->m_sink_node != NULL){
            EAGLEEYE_LOGE("sink node has been set");
            return;
        }

        this->m_sink_node = node;
        this->m_sink_ignore_port = dontcare;

        int source_output_signal_num = node->getNumberOfOutputSignals();
        if(dontcare < 0){
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
                if(i != dontcare){
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
        EAGLEEYE_LOGE("%s node not int subpipeline", toname.c_str());
        return;
    }

    if(fromname == "UPPER"){
        // reserved keyword
        m_special_source_port_map[fromport] = std::pair<std::string, int>(toname, toport);
        AnyNode* node_to_ptr = this->m_subpipeline[std::string(toname)];
        if(node_to_ptr->getNumberOfInputSignals() < toport+1){
            node_to_ptr->setNumberOfInputSignals(toport+1);
        }
        return;
    }

    if(m_subpipeline.find(fromname) == m_subpipeline.end()){
        EAGLEEYE_LOGE("%s node not int subpipeline", fromname.c_str());
        return;
    }

    AnyNode* node_to_ptr = this->m_subpipeline[std::string(toname)];
    AnyNode* node_from_ptr = this->m_subpipeline[std::string(fromname)];
    if(node_to_ptr->getNumberOfInputSignals() < toport+1){
        node_to_ptr->setNumberOfInputSignals(toport+1);
    }
    node_to_ptr->setInputPort(node_from_ptr->getOutputPort(fromport), toport);
}

void SubPipeline::reset(){
    this->m_sink_node->reset();

    Superclass::reset();
}

void SubPipeline::init(){
    this->m_sink_node->init();

    Superclass::init();
}

void SubPipeline::exit(){
    this->m_sink_node->exit();

    Superclass::exit();
}

void SubPipeline::getPipelineMonitors(std::map<std::string,std::vector<AnyMonitor*>>& pipeline_monitor_pool){
    // collect all node monitors in subpipeline
    this->m_sink_node->getPipelineMonitors(pipeline_monitor_pool);

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
