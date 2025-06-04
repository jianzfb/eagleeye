#include "eagleeye/processnode/AsynNode.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/EagleeyeLog.h"
#include<stdio.h>
#include<stdlib.h>
 #include <cstdlib>
#include <unistd.h> // sleep 的头文件

namespace eagleeye
{
AsynNode::AsynNode(int thread_num, std::function<AnyNode*()> generator, int input_queue_size, int output_queue_size):
    m_input_queue_size(input_queue_size),
    m_output_queue_size(output_queue_size),
    m_process_count(0),
    m_is_asyn_mode(true),
    m_thread_status(true){
    // 开启线程        
    for(int i=0; i<thread_num; ++i){
        m_run_node.push_back(generator());
        m_run_threads.push_back(std::thread(std::bind(&AsynNode::run,this, i)));
    }
    if(thread_num <= 0){
        // 非多线程模式
        m_is_asyn_mode = false;
        m_run_node.push_back(generator());
    }

    // 设置输出端口
    int signal_num = m_run_node[0]->getNumberOfOutputSignals();
    this->setNumberOfOutputSignals(signal_num);
    for(int signal_i=0; signal_i<signal_num; ++signal_i){
        AnySignal* output_signal = m_run_node[0]->getOutputPort(signal_i)->make();
        this->setOutputPort(output_signal, signal_i);
    }

    this->m_round = 1;
    this->m_reset_flag = true;
}

AsynNode::~AsynNode(){
    std::vector<AnyNode*>::iterator iter, iend(this->m_run_node.end());
    for(iter = this->m_run_node.begin(); iter != iend; ++iter){
        delete *iter;
    }
}

void AsynNode::executeNodeInfo(){
    if(!m_is_asyn_mode){
        // 同步调用
        for(int i=0; i<this->getNumberOfInputSignals(); ++i){
            m_run_node[0]->setInputPort(this->getInputPort(i), i);
        }
        m_run_node[0]->start();
        for(int i=0; i<this->getNumberOfOutputSignals(); ++i){
            this->getOutputPort(i)->copy(m_run_node[0]->getOutputPort(i));
        }
        return;
    }

    // 1.step generate input signal clone
    std::vector<std::shared_ptr<AnySignal>> input_signals_cp;
    for(int i=0; i<this->getNumberOfInputSignals(); ++i){
        AnySignal* signal_cp = this->getInputPort(i)->make();
        signal_cp->copy(this->getInputPort(i));
        input_signals_cp.push_back(std::shared_ptr<AnySignal>(signal_cp, [](AnySignal* a){delete a;}));
    }

    // 2.step get input and push to queue
	std::unique_lock<std::mutex> input_locker(this->m_input_mu);
    if(this->m_input_cache.size() >= this->m_input_queue_size){
        // pop the oldest element
        this->m_input_cache.pop_front();
    }
    // push the newest frame
    m_input_cache.push_back(std::pair<std::vector<std::shared_ptr<AnySignal>>, AsynMetaData>(input_signals_cp, AsynMetaData(this->m_process_count, this->m_round)));
    input_locker.unlock();

	// 3.step notify thread
	this->m_input_cond.notify_one();
    
    // 4.step update process count
    this->m_process_count += 1;
}

void AsynNode::preexit(){
    if(!m_is_asyn_mode){
        return;
    }
    this->m_thread_status = false;
}

void AsynNode::postexit(){
    if(!m_is_asyn_mode){
        return;
    }
    std::unique_lock<std::mutex> input_locker(this->m_input_mu);
    // clear input queue
    while (this->m_input_cache.size() > 0){
        this->m_input_cache.pop_front();
    }

    // push empty 
    for(int i=0; i<this->m_run_threads.size(); ++i){
        this->m_input_cache.push_back(std::pair<std::vector<std::shared_ptr<AnySignal>>, AsynMetaData>(std::vector<std::shared_ptr<AnySignal>>(),AsynMetaData(0,0)));
    }
    input_locker.unlock();
    this->m_input_cond.notify_one();

    for(int i=0; i<this->m_run_threads.size(); ++i){
        if(this->m_run_threads[i].joinable()){
            this->m_run_threads[i].join();
        }
    }
}

void AsynNode::refresh(){
    if(!m_is_asyn_mode){
        return;
    }
    std::unique_lock<std::mutex> output_locker(this->m_output_mu);    
    if(this->m_reset_flag){
        // remove all data in not this round
        this->m_output_list.remove_if([&](std::pair<std::vector<std::shared_ptr<AnySignal>>, AsynMetaData>& a){return a.second.round != m_round;});
        
        if(this->m_output_list.size() > 0){
            this->m_reset_flag = false;
        }
    }
    if(this->m_output_list.size() == 0){
        output_locker.unlock();
        return;
    }

    // get the newest output
    std::pair<std::vector<std::shared_ptr<AnySignal>>, AsynMetaData> result = m_output_list.back();
    // remove the oldest output
    m_output_list.pop_front();
    output_locker.unlock();

    for(int i=0; i<this->getNumberOfOutputSignals(); ++i){
        this->getOutputPort(i)->copy(result.first[i].get());
    }
}

bool LatestPriority(const std::pair<std::vector<std::shared_ptr<AnySignal>>, AsynMetaData>& a,const std::pair<std::vector<std::shared_ptr<AnySignal>>, AsynMetaData>& b) { 
    return a.second.timestamp < b.second.timestamp; 
}

void AsynNode::run(int thread_id){
    while (this->m_thread_status){
        // 1.step get input
        std::unique_lock<std::mutex> input_locker(this->m_input_mu);
        while(this->m_input_cache.size() == 0 && this->m_thread_status){
            this->m_input_cond.wait(input_locker);
        }

        if(!this->m_thread_status){
			input_locker.unlock();
			break;
		}

        // get the newest frame
        std::pair<std::vector<std::shared_ptr<AnySignal>>, AsynMetaData> input_data = this->m_input_cache.back();
        // pop this
        this->m_input_cache.pop_back();
        input_locker.unlock();

        if(input_data.first.size() == 0){
            break;
        }

        // 2.step 更新输入数据
        int timestamp = input_data.second.timestamp;        
        int round = input_data.second.round;
        
        if(m_run_node[thread_id]->getNumberOfInputSignals() != input_data.first.size()){
            m_run_node[thread_id]->setNumberOfInputSignals(input_data.first.size());
        }
        for(int i=0; i<input_data.first.size(); ++i){
            AnySignal* sig = input_data.first[i].get();
            m_run_node[thread_id]->setInputPort(sig, i);
        }

        // 3.step 运行节点
        m_run_node[thread_id]->start();

        std::vector<std::shared_ptr<AnySignal>> ll;
        for(int i=0; i<m_run_node[thread_id]->getNumberOfOutputSignals(); ++i){
            AnySignal* output_signal = m_run_node[thread_id]->getOutputPort(i)->make();
            output_signal->copy(m_run_node[thread_id]->getOutputPort(i));
            ll.push_back(std::shared_ptr<AnySignal>(output_signal, [](AnySignal* a){delete a;}));
        }

        // 4.step 输出到队列
        std::unique_lock<std::mutex> output_locker(this->m_output_mu);    
        m_output_list.push_back(std::pair<std::vector<std::shared_ptr<AnySignal>>, AsynMetaData>(ll, AsynMetaData(timestamp, round)));
        // 按时间排序，最新结果置于列尾
        m_output_list.sort(LatestPriority);
        if(m_output_list.size() > this->m_output_queue_size){
            // 除去最旧数据
            m_output_list.pop_front();
        }

        // if(this->m_reset_flag){
        //     // remove all data in not this round
        //     this->m_output_list.remove_if([&](std::pair<std::vector<std::shared_ptr<AnySignal>>, AsynMetaData>& a){return a.second.round != m_round;});
        //     if(this->m_output_list.size() > 0){
        //         this->m_reset_flag = false;
        //     }
        // }

        // get the newest output
        std::pair<std::vector<std::shared_ptr<AnySignal>>, AsynMetaData> result = m_output_list.back();
        // remove the oldest output
        m_output_list.pop_front();

        for(int i=0; i<this->getNumberOfOutputSignals(); ++i){
            this->getOutputPort(i)->copy(result.first[i].get());
        }
        output_locker.unlock();
    }
}

void AsynNode::reset(){
    if(!m_is_asyn_mode){
        return;
    }
    // clear input queue
    std::unique_lock<std::mutex> input_locker(this->m_input_mu);
    this->m_input_cache.clear();
    input_locker.unlock();

    // clear output queue
    std::unique_lock<std::mutex> output_locker(this->m_output_mu);
    this->m_output_list.clear();
    output_locker.unlock();

    this->m_process_count = 0;
    this->m_round *= -1;
    this->m_reset_flag = true;
    Superclass::reset();
}

void AsynNode::getPipelineMonitors(std::map<std::string,std::vector<AnyMonitor*>>& pipeline_monitor_pool){
    for(int ti=0; ti<m_run_node.size(); ++ti){
        std::map<std::string,std::vector<AnyMonitor*>> collect;
        m_run_node[ti]->getPipelineMonitors(collect);

        std::map<std::string,std::vector<AnyMonitor*>>::iterator iter, iend(collect.end());
        for(iter = collect.begin(); iter != iend; ++iter){
            pipeline_monitor_pool[iter->first+"/"+tos(ti)] = iter->second;
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

void AsynNode::loadConfigure(std::map<std::string, std::shared_ptr<char>> nodes_config){
    for(int ti=0; ti<m_run_node.size(); ++ti){
        m_run_node[ti]->loadConfigure(nodes_config);
    }

    Superclass::loadConfigure(nodes_config);
}

void AsynNode::saveConfigure(std::map<std::string, std::shared_ptr<char>>& nodes_config){
    for(int ti=0; ti<m_run_node.size(); ++ti){
        m_run_node[ti]->saveConfigure(nodes_config);
    }

    Superclass::saveConfigure(nodes_config);
}

void AsynNode::setUnitName(const char* unit_name){ 
    this->m_unit_name=std::string("asyn-") + unit_name;
    for(int ti=0; ti<m_run_node.size(); ++ti){
        m_run_node[ti]->setUnitName(unit_name);
    }
}

} // namespace eagleeye
