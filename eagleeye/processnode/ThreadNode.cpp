#include "eagleeye/processnode/ThreadNode.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/EagleeyeLog.h"
#include<stdio.h>
#include<stdlib.h>
 #include <cstdlib>
#include <unistd.h> // sleep 的头文件

namespace eagleeye
{
ThreadNode::ThreadNode(std::function<AnyNode*()> generator):
    m_thread_status(true){
    // 开启线程        
    m_run_node = generator();
    m_run_thread = std::thread(std::bind(&ThreadNode::run, this));

    // 设置输出端口
    int signal_num = m_run_node->getNumberOfOutputSignals();
    this->setNumberOfOutputSignals(signal_num);
    for(int signal_i=0; signal_i<signal_num; ++signal_i){
        AnySignal* output_signal = m_run_node->getOutputPort(signal_i)->make();
        output_signal->setSignalType(m_run_node->getOutputPort(signal_i)->getSignalType());
        this->setOutputPort(output_signal, signal_i);
    }

    this->m_receive_num = 0;
    this->m_finish_num = 0;
}

ThreadNode::~ThreadNode(){
    delete m_run_node;
}

void ThreadNode::executeNodeInfo(){
    // 1.step generate input signal clone
    std::vector<std::shared_ptr<AnySignal>> input_signals_cp;
    for(int i=0; i<this->getNumberOfInputSignals(); ++i){

        // MetaData dd = this->getInputPort(i)->meta();
        // if(dd.is_end_frame){
        //     EAGLEEYE_LOGD("thread node II end frame");
        // }

        // if(dd.is_start_frame){
        //     EAGLEEYE_LOGD("thread node II start frame");
        // }

        AnySignal* signal_cp = this->getInputPort(i)->make();
        signal_cp->copy(this->getInputPort(i));
        signal_cp->setSignalType(this->getInputPort(i)->getSignalType());

        // dd = signal_cp->meta();
        // if(dd.is_end_frame){
        //     EAGLEEYE_LOGD("thread node WW end frame");
        // }

        // if(dd.is_start_frame){
        //     EAGLEEYE_LOGD("thread node WW start frame");
        // }

        input_signals_cp.push_back(std::shared_ptr<AnySignal>(signal_cp, [](AnySignal* a){delete a;}));
    }

    // 2.step get input and push to queue
	std::unique_lock<std::mutex> input_locker(this->m_input_mu);
    m_input_cache.push_back(input_signals_cp);
    this->m_receive_num += 1;
    EAGLEEYE_LOGD("Increment 1 in ThreadNode, now is %d.", m_receive_num);
    input_locker.unlock();

	// 3.step notify thread
	this->m_input_cond.notify_one();
}

void ThreadNode::preexit(){
    this->m_thread_status = false;
}

void ThreadNode::postexit(){
    std::unique_lock<std::mutex> input_locker(this->m_input_mu);
    // clear input queue
    while (this->m_input_cache.size() > 0){
        this->m_input_cache.pop_front();
    }
    
    // push empty 
    this->m_input_cache.push_back(std::vector<std::shared_ptr<AnySignal>>());
    input_locker.unlock();
    this->m_input_cond.notify_one();

    // 等待线程结束
    if(this->m_run_thread.joinable()){
        this->m_run_thread.join();
    }
}

void ThreadNode::refresh(){
    std::unique_lock<std::mutex> output_locker(this->m_output_mu);    
    if(this->m_output_list.size() == 0){
        output_locker.unlock();
        return;
    }

    // get the front
    std::vector<std::shared_ptr<AnySignal>> result = m_output_list.front();
    // remove 
    m_output_list.pop_front();
    output_locker.unlock();

    for(int i=0; i<this->getNumberOfOutputSignals(); ++i){
        this->getOutputPort(i)->copy(result[i].get());
    }
}

void ThreadNode::run(){
    while (this->m_thread_status){
        // 1.step 获取输入
        std::unique_lock<std::mutex> input_locker(this->m_input_mu);
        while(this->m_input_cache.size() == 0 && this->m_thread_status){
            this->m_input_cond.wait(input_locker);
        }

        if(!this->m_thread_status){
			input_locker.unlock();
			break;
		}

        // get the front
        std::vector<std::shared_ptr<AnySignal>> input_data = this->m_input_cache.front();
        // remove
        this->m_input_cache.pop_front();
        input_locker.unlock();

        if(input_data.size() == 0){
            break;
        }

        // 2.step 更新输入数据
        if(m_run_node->getNumberOfInputSignals() != input_data.size()){
            m_run_node->setNumberOfInputSignals(input_data.size());
        }
        for(int i=0; i<input_data.size(); ++i){
            AnySignal* sig = input_data[i].get();
            m_run_node->setInputPort(sig, i);
        }

        // 3.step 运行节点
        m_run_node->start();

        // 4.step 获取输出
        std::vector<std::shared_ptr<AnySignal>> ll;
        bool has_output = false;
        for(int i=0; i<m_run_node->getNumberOfOutputSignals(); ++i){
            if(!m_run_node->getOutputPort(i)->isempty()){
                has_output = true;
            }

            AnySignal* output_signal = m_run_node->getOutputPort(i)->make();
            output_signal->copy(m_run_node->getOutputPort(i));
            ll.push_back(std::shared_ptr<AnySignal>(output_signal, [](AnySignal* a){delete a;}));
        }

        std::unique_lock<std::mutex> output_locker(this->m_output_mu);
        m_finish_num += 1;
        if(has_output){
            m_output_list.push_back(ll);
        }
        output_locker.unlock();
    }
}

void ThreadNode::wait(){
    Superclass::wait();

    // 阻塞，直到清空所有未完成计算
    while (1){
        EAGLEEYE_LOGD("Waiting receive %d finish %d.", m_receive_num, m_finish_num);
        std::unique_lock<std::mutex> input_locker(this->m_input_mu);
        std::unique_lock<std::mutex> output_locker(this->m_output_mu);
        if(this->m_receive_num == this->m_finish_num){
            input_locker.unlock();
            output_locker.unlock();
            break;
        }

        input_locker.unlock();
        output_locker.unlock();

        sleep(1);
    }
}

void ThreadNode::reset(){
    // clear input queue
    std::unique_lock<std::mutex> input_locker(this->m_input_mu);
    this->m_input_cache.clear();
    input_locker.unlock();

    // clear output queue
    std::unique_lock<std::mutex> output_locker(this->m_output_mu);
    this->m_output_list.clear();
    output_locker.unlock();

    // 
    Superclass::reset();
}

void ThreadNode::getPipelineMonitors(std::map<std::string,std::vector<AnyMonitor*>>& pipeline_monitor_pool){
    std::map<std::string,std::vector<AnyMonitor*>> collect;
    m_run_node->getPipelineMonitors(collect);

    std::map<std::string,std::vector<AnyMonitor*>>::iterator iter, iend(collect.end());
    for(iter = collect.begin(); iter != iend; ++iter){
        pipeline_monitor_pool[iter->first] = iter->second;
    }

	//traverse the whole pipeline
	std::vector<AnySignal*>::iterator signal_iter,signal_iend(m_input_signals.end());
	for (signal_iter = m_input_signals.begin();signal_iter != signal_iend; ++signal_iter)
	{
		if ((*signal_iter))
			(*signal_iter)->getPipelineMonitors(pipeline_monitor_pool);
	}
}

void ThreadNode::loadConfigure(std::map<std::string, std::shared_ptr<char>> nodes_config){
    m_run_node->loadConfigure(nodes_config);
    Superclass::loadConfigure(nodes_config);
}

void ThreadNode::saveConfigure(std::map<std::string, std::shared_ptr<char>>& nodes_config){
    m_run_node->saveConfigure(nodes_config);
    Superclass::saveConfigure(nodes_config);
}

void ThreadNode::setUnitName(const char* unit_name){ 
    this->m_unit_name=std::string("thread-") + unit_name;
    m_run_node->setUnitName(unit_name);
}

} // namespace eagleeye
