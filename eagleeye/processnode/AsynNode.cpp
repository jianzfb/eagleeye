#include "eagleeye/processnode/AsynNode.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/EagleeyeLog.h"
namespace eagleeye
{
AsynNode::AsynNode(int thread_num, std::function<AnyNode*()> generator, int input_queue_size, int output_queue_size):
    m_input_queue_size(input_queue_size),
    m_output_queue_size(output_queue_size),
    m_process_count(0),
    m_thread_status(true){
    for(int i=0; i<thread_num; ++i){
        m_run_node.push_back(generator());
        m_run_threads.push_back(std::thread(std::bind(&AsynNode::run,this, i)));
    }

    // 设置输入端口
    setNumberOfInputSignals(1);
    // 设置输出端口
    setNumberOfOutputSignals(1);
    AnySignal* output_signal = m_run_node[0]->getOutputPort(0)->make();
    this->setOutputPort(output_signal, 0);
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
    // 1.step generate input signal clone
    AnySignal* input_signal_cp = this->getInputPort(0)->make();
    input_signal_cp->copy(this->getInputPort(0));

    // 2.step get input and push to queue
	std::unique_lock<std::mutex> input_locker(this->m_input_mu);
    if(this->m_input_cache.size() >= this->m_input_queue_size){
        // pop the oldest element
        this->m_input_cache.pop_front();
    }
    // push the newest frame
    m_input_cache.push_back(std::pair<std::shared_ptr<AnySignal>, AsynMetaData>(std::shared_ptr<AnySignal>(input_signal_cp,[](AnySignal* a){delete a;}), AsynMetaData(this->m_process_count, this->m_round)));
    input_locker.unlock();

	// 3.step notify thread
	this->m_input_cond.notify_one();
    
    // 4.step update process count
    this->m_process_count += 1;
}

void AsynNode::postexit(){
    std::unique_lock<std::mutex> input_locker(this->m_input_mu);
    // clear input queue
    while (this->m_input_cache.size() > 0){
        this->m_input_cache.pop_front();
    }
    
    // push empty 
    for(int i=0; i<this->m_run_threads.size(); ++i){
        this->m_input_cache.push_back(std::pair<std::shared_ptr<AnySignal>, AsynMetaData>(std::shared_ptr<AnySignal>(),AsynMetaData(0,0)));
    }
    this->m_thread_status = false;
    input_locker.unlock();
    this->m_input_cond.notify_all();

    for(int i=0; i<this->m_run_threads.size(); ++i){
        this->m_run_threads[i].join();
    }
}

void AsynNode::refresh(){
    EAGLEEYE_LOGD("asynnode refresh");
    std::unique_lock<std::mutex> output_locker(this->m_output_mu);    
    if(this->m_output_list.size() == 0){
        output_locker.unlock();
        return;
    }

    if(this->m_reset_flag){
        // remove all data in not this round
        this->m_output_list.remove_if([&](std::pair<std::shared_ptr<AnySignal>, AsynMetaData>& a){return a.second.round != m_round;});
        this->m_reset_flag = false;
    }
    
    // get the newest output
    std::pair<std::shared_ptr<AnySignal>, AsynMetaData> result = m_output_list.back();
    // remove the oldest output
    m_output_list.pop_front();
    output_locker.unlock();

    this->m_output_signals[0]->copy(result.first.get());
}

bool LatestPriority(const std::pair<std::shared_ptr<AnySignal>, AsynMetaData>& a,const std::pair<std::shared_ptr<AnySignal>, AsynMetaData>& b) { 
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
        std::pair<std::shared_ptr<AnySignal>, AsynMetaData> input_data = this->m_input_cache.back();
        // pop this
        this->m_input_cache.pop_back();
        input_locker.unlock();

        if(input_data.first.get() == NULL){
            return;
        }

        // 2.step 更新输入数据
        AnySignal* input_signal = input_data.first.get();
        int timestamp = input_data.second.timestamp;        
        int round = input_data.second.round;
        m_run_node[thread_id]->setInputPort(input_signal, 0);

        // 3.step 运行节点
        m_run_node[thread_id]->start();
        AnySignal* output_signal = m_run_node[thread_id]->getOutputPort(0)->make();
        output_signal->copy(m_run_node[thread_id]->getOutputPort(0));

        // 4.step 输出到队列
        std::unique_lock<std::mutex> output_locker(this->m_output_mu);
        // 加入最新结果
        m_output_list.push_back(std::pair<std::shared_ptr<AnySignal>, AsynMetaData>(std::shared_ptr<AnySignal>(output_signal, [](AnySignal* a){delete a;}), AsynMetaData(timestamp,round)));
        // 按时间排序，最新结果置于列尾
        m_output_list.sort(LatestPriority);
        if(m_output_list.size() > this->m_output_queue_size){
            // 除去最旧数据
            m_output_list.pop_front();
        }

        // std::list<std::pair<std::shared_ptr<AnySignal>, AsynMetaData>>::iterator iter, iend(m_output_list.end());
        // for(iter = m_output_list.begin(); iter != iend; ++iter){
        //     std::cout<<iter->second.timestamp<<"\t";
        // }
        // std::cout<<'\n';
        output_locker.unlock();
    }
}

void AsynNode::reset(){
    std::cout<<"RESET ASYNNODE"<<std::endl;
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
} // namespace eagleeye
