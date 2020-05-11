#include "eagleeye/processnode/AutoNode.h"
#include <functional>
#include <thread>

namespace eagleeye
{
AutoNode::AutoNode(std::function<AnyNode*()> generator){
    m_auto_node = generator();
    // 设置输出端口
    int signal_num = m_auto_node->getNumberOfOutputSignals();
    this->setNumberOfOutputSignals(signal_num);
    for(int signal_i=0; signal_i<signal_num; ++signal_i){
        AnySignal* output_signal = m_auto_node->getOutputPort(signal_i)->make();
        // 必须为队列信号
        output_signal->transformCategoryToQ();
        this->setOutputPort(output_signal, signal_i);
    }

    this->m_thread_status = true;
    this->m_is_ini = false;
}   

AutoNode::~AutoNode(){
    delete m_auto_node;
}

void AutoNode::executeNodeInfo(){
    // do nothing
}

void AutoNode::run(){
    std::vector<AnySignal*> signal_list;
    for(int signal_i = 0; signal_i<this->getNumberOfInputSignals(); ++signal_i){
        AnySignal* signal_cp = this->getInputPort(signal_i)->make();
        signal_list.push_back(signal_cp);
    }

    bool running_ischange = false;
    while (true){
        if(!this->m_thread_status){
            break;
        }

        if(!running_ischange){
            // 1.step get input
            int signal_num = this->getNumberOfInputSignals();
            // std::cout<<"auto node has input signal "<<signal_num<<std::endl;
            for(int signal_i = 0; signal_i<signal_num; ++signal_i){
                // block call
                // std::cout<<"copy string"<<std::endl;
                // std::cout<<"signal list size "<<signal_list.size()<<std::endl;
                signal_list[signal_i]->copy(this->getInputPort(signal_i));
                // std::cout<<"finish copy "<<std::endl;
                if(!this->m_thread_status){
                    break;
                }

                // set input
                m_auto_node->setInputPort(signal_list[signal_i], signal_i);
            }
        }

        if(!this->m_thread_status){
            break;
        }

        // 2.step run node
        running_ischange = m_auto_node->start();
        // std::cout<<"runing runing runing"<<std::endl;
        if(!running_ischange){
            continue;
        }
        // std::cout<<"GGGG"<<std::endl;
        // 3.step get output
        int signal_num = m_auto_node->getNumberOfOutputSignals();
        for(int signal_i = 0; signal_i<signal_num; ++signal_i){
            this->getOutputPort(signal_i)->copy(m_auto_node->getOutputPort(signal_i));
        }
    }

    for(int signal_i = 0; signal_i<this->getNumberOfInputSignals(); ++signal_i){
        delete signal_list[signal_i];
    }
}

void AutoNode::preexit(){
    // prepare exit
    this->m_thread_status = false;
}

void AutoNode::postexit(){
    int signal_num = m_auto_node->getNumberOfOutputSignals();
    for(int signal_i = 0; signal_i<signal_num; ++signal_i){
        // dont care data is what
        this->getOutputPort(signal_i)->copy(m_auto_node->getOutputPort(signal_i));        
    }

    if(this->m_is_ini){
        if(m_auto_thread.joinable()){
            m_auto_thread.join();
        }
    }
}

void AutoNode::reset(){
    Superclass::reset();
    
    m_auto_node->reset();
}

void AutoNode::init(){
    Superclass::init();

    if(!this->m_is_ini){
        m_auto_thread = std::thread(std::bind(&AutoNode::run,this));
        this->m_is_ini = true;
    }   
}

void AutoNode::updateUnitInfo(){
    modified();
    Superclass::updateUnitInfo();
}

void AutoNode::getPipelineMonitors(std::map<std::string,std::vector<AnyMonitor*>>& pipeline_monitor_pool){
    // collect all node monitors in subpipeline
    this->m_auto_node->getPipelineMonitors(pipeline_monitor_pool);

	//traverse the whole pipeline
	std::vector<AnySignal*>::iterator signal_iter,signal_iend(m_input_signals.end());
	for (signal_iter = m_input_signals.begin();signal_iter != signal_iend; ++signal_iter)
	{
		if ((*signal_iter))
			(*signal_iter)->getPipelineMonitors(pipeline_monitor_pool);
	}
}

void AutoNode::loadConfigure(std::map<std::string, std::shared_ptr<char>> nodes_config){
    this->m_auto_node->loadConfigure(nodes_config);
    Superclass::loadConfigure(nodes_config);
}

void AutoNode::saveConfigure(std::map<std::string, std::shared_ptr<char>>& nodes_config){
    this->m_auto_node->saveConfigure(nodes_config);
    Superclass::saveConfigure(nodes_config);
}

} // namespace eagleeye
