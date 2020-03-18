#include "eagleeye/processnode/SubPipeline.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/framework/pipeline/AnyPipeline.h"
namespace eagleeye
{
SubPipeline::SubPipeline(){
	this->setNumberOfInputSignals(1);
    this->m_input_node_placeholder=NULL;
    this->m_output_node = NULL;
}

SubPipeline::~SubPipeline(){
    std::map<std::string, AnyNode*>::iterator iter, iend(this->m_subpipeline.end());
    for(iter=this->m_subpipeline.begin(); iter!=iend; ++iter){
        delete iter->second;
    }
    if(m_input_node_placeholder){
        delete m_input_node_placeholder;
    }    
}

void SubPipeline::executeNodeInfo(){
    // 1.step 绑定子管道输入
    if(m_input_node_placeholder == NULL){
        m_input_node_placeholder = this->getInputPort(0)->make();
        for(int i=0; i<this->m_input_nodes.size(); ++i){
            this->m_input_nodes[i]->setInputPort(m_input_node_placeholder, this->m_input_ports[i]);
        }
    }

    // 2.step copy input node signal to subpipeline
    this->m_input_node_placeholder->copy(this->getInputPort(0));
    for(int i=0; i<this->m_input_nodes.size(); ++i){
        this->m_input_nodes[i]->modified();
    }

    // 3.step run subpipeline
    m_output_node->start();
    // 4.step copy output signal to subpipeline
    for(int i=0; i<this->m_output_node->getNumberOfOutputSignals(); ++i){
        this->getOutputPort(i)->copy(m_output_node->getOutputPort(i));
    }
}

void SubPipeline::add(AnyNode* node, std::string name, PipelineNodeType nodetype){
    m_subpipeline[name] = node;
    m_in_deg[name] = 0;
    m_out_deg[name] = 0;
    node->setUnitName(name.c_str());

    switch (nodetype)
    {
    case SOURCE_NODE:
        this->m_input_nodes.push_back(node);
        this->m_input_ports.push_back(0);
        break;
    case SINK_NODE:
        this->m_output_node = node;
        this->m_output_node_name = name;   
        if(this->getNumberOfOutputSignals() == 0){
            int output_signal_num = this->m_output_node->getNumberOfOutputSignals();
            this->setNumberOfOutputSignals(output_signal_num);
        }
        this->setOutputPort(this->m_output_node->getOutputPort(0)->make(), 0);
        break;
    default:
        break;
    }
}

void SubPipeline::bind(std::string fromname, int fromport, std::string toname, int toport){
    if(m_subpipeline.find(fromname) == m_subpipeline.end()){
        EAGLEEYE_LOGE("%s node not int subpipeline", fromname.c_str());
        return;
    }
    if(m_subpipeline.find(toname) == m_subpipeline.end()){
        EAGLEEYE_LOGE("%s node not int subpipeline", toname.c_str());
        return;
    }

    m_subpipeline[toname]->setInputPort(m_subpipeline[fromname]->getOutputPort(fromport), toport);
    m_in_deg[toname] += 1;
    m_out_deg[fromname] += 1;
}

void SubPipeline::reset(){
    std::map<std::string, AnyNode*>::iterator iter, iend(this->m_subpipeline.end());
    for(iter=this->m_subpipeline.begin(); iter!=iend; ++iter){
        iter->second->reset();
    }

    Superclass::reset();
}

void SubPipeline::getPipelineMonitors(std::map<std::string,std::vector<AnyMonitor*>>& pipeline_monitor_pool){
    // collect all node monitors in subpipeline
    this->m_output_node->getPipelineMonitors(pipeline_monitor_pool);

	//traverse the whole pipeline
	std::vector<AnySignal*>::iterator signal_iter,signal_iend(m_input_signals.end());
	for (signal_iter = m_input_signals.begin();signal_iter != signal_iend; ++signal_iter)
	{
		if ((*signal_iter))
			(*signal_iter)->getPipelineMonitors(pipeline_monitor_pool);
	}
}
} // namespace eagleeye
