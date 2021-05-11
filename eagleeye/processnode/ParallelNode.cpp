#include "eagleeye/processnode/ParallelNode.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/common/EagleeyeStr.h"

namespace eagleeye{
ParallelNode::ParallelNode(int thread_num, std::function<AnyNode*()> generator){
    // generate run nodes
    for(int i=0; i<thread_num; ++i){
        m_run_node.push_back(generator());
    }

    // 设置输出端口
    int signal_num = m_run_node[0]->getNumberOfOutputSignals();
    this->setNumberOfOutputSignals(signal_num);
    for(int signal_i=0; signal_i<signal_num; ++signal_i){
        AnySignal* output_signal = m_run_node[0]->getOutputPort(signal_i)->make();
        // 必须为队列信号
        output_signal->transformCategoryToQ();
        this->setOutputPort(output_signal, signal_i);
    }
    m_output_cache.resize(signal_num);

    this->m_thread_status = true;
    this->m_is_ini = false;
    m_waiting_count = 0;

}

ParallelNode::~ParallelNode(){
    std::vector<AnyNode*>::iterator iter, iend(this->m_run_node.end());
    for(iter = this->m_run_node.begin(); iter != iend; ++iter){
        delete *iter;
    }
}

void ParallelNode::executeNodeInfo(){
    // do nothing
}

void ParallelNode::init(){
    Superclass::init();

    if(!this->m_is_ini){
        for(int i=0; i<m_run_node.size(); ++i){
            m_run_threads.push_back(std::thread(std::bind(&ParallelNode::run,this, i)));
        }

        this->m_is_ini = true;
    }
}

void ParallelNode::run(int thread_id){
    std::vector<AnySignal*> signal_list;
    for(int signal_i = 0; signal_i<this->getNumberOfInputSignals(); ++signal_i){
        AnySignal* signal_cp = this->getInputPort(signal_i)->make();
        signal_list.push_back(signal_cp);
    }

    while (true){
        if(!this->m_thread_status){
            break;
        }

        // 1.step get input
        int signal_num = this->getNumberOfInputSignals();
        std::vector<int> timestamp;
        timestamp.resize(signal_num);
        for(int signal_i = 0; signal_i<signal_num; ++signal_i){
            // block call
            signal_list[signal_i]->copy(this->getInputPort(signal_i));
            timestamp[signal_i] = signal_list[signal_i]->meta().timestamp;
            if(!this->m_thread_status){
                break;
            }

            // set input
            this->m_run_node[thread_id]->setInputPort(signal_list[signal_i], signal_i);
        }

        if(!this->m_thread_status){
            break;
        }

        // 2.step run node
        this->m_run_node[thread_id]->start();

        // 3.step get output
        signal_num = m_run_node[thread_id]->getNumberOfOutputSignals();
        std::unique_lock<std::mutex> output_locker(this->m_output_mu);
        for(int signal_i = 0; signal_i<signal_num; ++signal_i){
            AnySignal* output_signal = m_run_node[thread_id]->getOutputPort(signal_i)->make();
            output_signal->copy(m_run_node[thread_id]->getOutputPort(signal_i));
            m_output_cache[signal_i].push(std::pair<std::shared_ptr<AnySignal>, int>(std::shared_ptr<AnySignal>(output_signal, [](AnySignal* a){delete a;}), timestamp[signal_i]));
        }
        output_locker.unlock();
        this->m_output_cond.notify_one();
    }
    
    for(int signal_i = 0; signal_i<this->getNumberOfInputSignals(); ++signal_i){
        delete signal_list[signal_i];
    }
}

void ParallelNode::refresh(){
    while(true){
        if(!m_thread_status){
            int signal_num = this->getNumberOfOutputSignals();
            for(int signal_i=0; signal_i < signal_num; ++signal_i){
                // dont care data
                this->getOutputPort(signal_i)->copy(m_run_node[0]->getOutputPort(signal_i));
            }
            break;
        }

        std::unique_lock<std::mutex> output_locker(this->m_output_mu);
        bool is_finding = false;

        if(m_output_cache[0].size() == 0 || m_waiting_count != m_output_cache[0].top().second){
            this->m_output_cond.wait(output_locker);
        }
        else{
            int signal_num = this->getNumberOfOutputSignals();
            for(int signal_i=0; signal_i < signal_num; ++signal_i){
                std::pair<std::shared_ptr<AnySignal>, int> tmp = m_output_cache[signal_i].top();
                this->getOutputPort(signal_i)->copy(tmp.first.get());
                m_output_cache[signal_i].pop();
            }
            // increment 
            this->m_waiting_count += 1;
            output_locker.unlock();
            break;
        }        
    }
}

void ParallelNode::preexit(){
    this->m_thread_status = false;
}

void ParallelNode::postexit(){
    this->m_output_cond.notify_one();

    int signal_num = this->getNumberOfOutputSignals();
    for(int signal_i = 0; signal_i<signal_num; ++signal_i){
        // dont care data is what
        this->getOutputPort(signal_i)->copy(m_run_node[0]->getOutputPort(signal_i));        
    }

    for(int i=0; i<this->m_run_threads.size(); ++i){
        if(this->m_run_threads[i].joinable()){
            this->m_run_threads[i].join();
        }
    }
}

void ParallelNode::reset(){
    for(int thread_id=0; thread_id<this->m_run_node.size(); thread_id++){
        m_run_node[thread_id]->reset();
    }

    m_waiting_count = 0;
    Superclass::reset();
}

void ParallelNode::getPipelineMonitors(std::map<std::string,std::vector<AnyMonitor*>>& pipeline_monitor_pool){
    for(int ti=0; ti<m_run_node.size(); ++ti){
        std::map<std::string,std::vector<AnyMonitor*>> collect;
        m_run_node[ti]->getPipelineMonitors(collect);

        std::map<std::string,std::vector<AnyMonitor*>>::iterator iter, iend(collect.end());
        for(iter = collect.begin(); iter != iend; ++iter){
            pipeline_monitor_pool[iter->first+"/"+tos<int>(ti)] = iter->second;
        }
    }

	//traverse the whole pipeline
	std::vector<AnySignal*>::iterator signal_iter,signal_iend(m_input_signals.end());
	for (signal_iter = m_input_signals.begin();signal_iter != signal_iend; ++signal_iter)
	{
		if ((*signal_iter))
			(*signal_iter)->getPipelineMonitors(pipeline_monitor_pool);
	}
}

void ParallelNode::loadConfigure(std::map<std::string, std::shared_ptr<char>> nodes_config){
    for(int ti=0; ti<m_run_node.size(); ++ti){
        m_run_node[ti]->loadConfigure(nodes_config);
    }

    Superclass::loadConfigure(nodes_config);
}

void ParallelNode::saveConfigure(std::map<std::string, std::shared_ptr<char>>& nodes_config){
    for(int ti=0; ti<m_run_node.size(); ++ti){
        m_run_node[ti]->saveConfigure(nodes_config);
    }

    Superclass::saveConfigure(nodes_config);
}

void ParallelNode::updateUnitInfo(){
    modified();
    Superclass::updateUnitInfo();
}

void ParallelNode::setUnitName(const char* unit_name){ 
    this->m_unit_name=std::string("parallel-") + unit_name;
    for(int ti=0; ti<m_run_node.size(); ++ti){
        m_run_node[ti]->setUnitName(unit_name);
    }
}
}