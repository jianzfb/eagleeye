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
    this->m_lock = std::shared_ptr<spinlock>(new spinlock(), [](spinlock* d) { delete d; });
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

    while (true){
        // 1.step get input
        int signal_num = this->getNumberOfInputSignals();
        for(int signal_i = 0; signal_i<signal_num; ++signal_i){
            signal_list[signal_i]->copyInfo(this->getInputPort(signal_i));
            // block call
            signal_list[signal_i]->copy(this->getInputPort(signal_i));

            if(!this->m_thread_status){
                break;
            }

            // set input
            m_auto_node->setInputPort(signal_list[signal_i], signal_i);
        }

        if(!this->m_thread_status){
            break;
        }

        // 2.step run node
        this->m_lock->lock();
        m_auto_node->start();
        this->m_lock->unlock();

        // 3.step get output
        signal_num = m_auto_node->getNumberOfOutputSignals();
        for(int signal_i = 0; signal_i<signal_num; ++signal_i){
            this->getOutputPort(signal_i)->copyInfo(m_auto_node->getOutputPort(signal_i));
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
        m_auto_thread.join();
    }
}

void AutoNode::reset(){
    Superclass::reset();
    
    this->m_lock->lock();
    m_auto_node->reset();
    this->m_lock->unlock();
}

void AutoNode::init(){
    Superclass::init();

    if(!this->m_is_ini){
        m_auto_thread = std::thread(std::bind(&AutoNode::run,this));
        this->m_is_ini = true;
    }   
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
} // namespace eagleeye
