#include "eagleeye/processnode/SubPipeline.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
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
    // 0.step 绑定子管道输入
    if(m_input_node_placeholder == NULL){
        m_input_node_placeholder = this->getInputPort(0)->make();
        for(int i=0; i<this->m_input_nodes.size(); ++i){
            this->m_input_nodes[i]->setInputPort(m_input_node_placeholder, this->m_input_ports[i]);
        }
    }

    // 1.step copy input node signal to subpipeline
    this->m_input_node_placeholder->copy(this->getInputPort(0));
    for(int i=0; i<this->m_input_nodes.size(); ++i){
        this->m_input_nodes[i]->modified();
    }

    // 2.step run subpipeline
    m_output_node->start();
    // 3.step copy output signal to subpipeline
    for(int i=0; i<this->m_output_node->getNumberOfOutputSignals(); ++i){
        this->getOutputPort(i)->copy(m_output_node->getOutputPort(i));
    }
}

void SubPipeline::add(AnyNode* node, std::string name){
    m_subpipeline[name] = node;
    m_in_deg[name] = 0;
    m_out_deg[name] = 0;
    node->setUnitName(name.c_str());
}

void SubPipeline::bind(std::string fromname, int fromport, std::string toname, int toport){
    if(fromname == "SOURCE"){
        // 强制源只能一个0端口
        assert(fromport == 0);
        this->m_input_nodes.push_back(m_subpipeline[toname]);
        this->m_input_ports.push_back(toport);
        // 暂缓绑定subpipeline的输入到对应的节点
        // 放到具体执行中绑定
        return;
    }

    if(toname == "SINK"){
        if(this->m_output_node != NULL){
            assert(this->m_output_node == m_subpipeline[fromname]);
        }

        this->m_output_node = m_subpipeline[fromname];
        this->m_output_node_name = fromname;

        if(this->getNumberOfOutputSignals() == 0){
            int output_signal_num = this->m_output_node->getNumberOfOutputSignals();
            this->setNumberOfOutputSignals(output_signal_num);
        }
        this->setOutputPort(this->m_output_node->getOutputPort(fromport)->make(), toport);
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
