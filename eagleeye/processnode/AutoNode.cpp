#include "eagleeye/processnode/AutoNode.h"
#include "eagleeye/common/EagleeyeTime.h"
#include <functional>
#include <thread>

namespace eagleeye
{
AutoNode::AutoNode(std::function<AnyNode*()> generator, int queue_size, bool get_then_auto_remove, bool copy_input){
    m_auto_node = generator();
    // 设置输出端口
    int signal_num = m_auto_node->getNumberOfOutputSignals();
    this->setNumberOfOutputSignals(signal_num);
    for(int signal_i=0; signal_i<signal_num; ++signal_i){
        AnySignal* output_signal = m_auto_node->getOutputPort(signal_i)->make();
        // 必须为队列信号
        output_signal->transformCategoryToQ(queue_size, get_then_auto_remove);
        output_signal->disableDataTimestamp();  // 禁用数据时间戳
        this->setOutputPort(output_signal, signal_i);
    }

    EAGLEEYE_MONITOR_VAR(std::string, setFolder, getFolder, "folder", "", "");

    this->m_thread_status = true;
    this->m_is_ini = false;
    this->m_last_timestamp = 0.0;
    this->m_callback = nullptr;
    this->m_persistent_flag = false;
    this->m_copy_input = copy_input;
    EAGLEEYE_LOGD("auto node run in copy_inpu mode = [%d]", m_copy_input);
}

AutoNode::~AutoNode(){
    // 重复尝试停止线程
    this->m_thread_status = false;
    if(this->m_is_ini){
        if(m_auto_thread.joinable()){
            m_auto_thread.join();
        }
    }
    // 删除内部节点
    if(m_auto_node != NULL){
        delete m_auto_node;
        m_auto_node = NULL;
    }
}

void AutoNode::executeNodeInfo(){
    // do nothing
    int signal_num = this->getNumberOfOutputSignals();
    for(int signal_i=0; signal_i<signal_num; ++signal_i){
        // 手动强制触发，时间戳更新
        this->getOutputPort(signal_i)->modified();
    }
}

void AutoNode::run(){
    if(m_copy_input){
        run_in_copy_input();
    }
    else{
        run_in_no_copy_input();
    }
}

void AutoNode::run_in_copy_input(){
    std::vector<AnySignal*> signal_list;
    for(int signal_i = 0; signal_i<this->getNumberOfInputSignals(); ++signal_i){
        AnySignal* signal_cp = this->getInputPort(signal_i)->make();
        m_auto_node->setInputPort(signal_cp, signal_i);        
        signal_list.push_back(signal_cp);
    }

    while (true){
        if(!this->m_thread_status){
            break;
        }

        bool is_duplicate_frame = false;
        double input_data_timestamp = 0.0;
        // 1.step get input
        int signal_num = this->getNumberOfInputSignals();
        for(int signal_i = 0; signal_i<signal_num; ++signal_i){
            // block call
            signal_list[signal_i]->copy(this->getInputPort(signal_i));

            // set input
            MetaData data_meta = signal_list[signal_i]->meta();
            if(data_meta.timestamp > 0.0 && m_last_timestamp == data_meta.timestamp){
                is_duplicate_frame = true;
            }
            input_data_timestamp = data_meta.timestamp;
        }

        if(is_duplicate_frame){
            // 重复帧，休息1ms
            std::this_thread::sleep_for(std::chrono::microseconds(1000));
            continue;
        }
        this->m_last_timestamp = input_data_timestamp;

        // 2.step run node
        bool running_ischange = m_auto_node->start();
        if(!running_ischange){
            continue;
        }

        // 3.step get output
        signal_num = m_auto_node->getNumberOfOutputSignals();
        bool is_auto_stop = false;
        for(int signal_i = 0; signal_i<signal_num; ++signal_i){
            this->getOutputPort(signal_i)->copy(m_auto_node->getOutputPort(signal_i));
            if(m_auto_node->getOutputPort(signal_i)->meta().is_end_frame){
                is_auto_stop = true;
            }
        }

        // 4.step callback
        if(this->m_callback != nullptr){           
            this->m_callback(this, this->m_output_signals);
        }

        // 5.step 检查自动结束条件
        if(is_auto_stop){
            m_thread_status = false;
            break;
        }
    }

    for(int signal_i = 0; signal_i<this->getNumberOfInputSignals(); ++signal_i){
        m_auto_node->clearInputPort(signal_i);
    }
    for(int signal_i = 0; signal_i<this->getNumberOfInputSignals(); ++signal_i){
        delete signal_list[signal_i];
    }
}

void AutoNode::run_in_no_copy_input(){
    int signal_num = this->getNumberOfInputSignals();
    for(int signal_i  = 0; signal_i < signal_num; ++signal_i){
        m_auto_node->setInputPort(getInputPort(signal_i), signal_i);        
    }
    while (true){
        if(!this->m_thread_status){
            break;
        }

        // 1. start run auto node
        bool running_ischange = m_auto_node->start();
        if(!running_ischange){
            continue;
        }

        // 3.step get output
        signal_num = m_auto_node->getNumberOfOutputSignals();
        bool is_auto_stop = false;
        for(int signal_i = 0; signal_i<signal_num; ++signal_i){
            this->getOutputPort(signal_i)->copy(m_auto_node->getOutputPort(signal_i));
            if(m_auto_node->getOutputPort(signal_i)->meta().is_end_frame){
                is_auto_stop = true;
            }
        }

        // 4.step callback
        if(this->m_callback != nullptr){           
            this->m_callback(this, this->m_output_signals);
        }

        // 5.step 检查自动结束条件
        if(is_auto_stop){
            m_thread_status = false;
            break;
        }
    }
}

void AutoNode::exit(){
    Superclass::exit();
    this->m_auto_node->exit();
}

void AutoNode::preexit(){
    if(this->m_persistent_flag){
        return;
    }

    // prepare exit
    this->m_thread_status = false;
}

void AutoNode::postexit(){
    if(this->m_persistent_flag){
        return;
    }

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

void AutoNode::setUnitName(const char* unit_name){ 
    this->m_unit_name=std::string("auto-") + unit_name;
    this->m_auto_node->setUnitName(unit_name);
}

void AutoNode::setCallback(std::function<void(AnyNode*, std::vector<AnySignal*>)> callback){
    this->m_callback = callback;
}

bool AutoNode::stop(bool block, bool force){
    if(force){
        // 主动退出
        this->exit();
    }

    if(block){
        // 等待直到结束
        if(m_auto_thread.joinable()){
            m_auto_thread.join();
        }
    }

    // 返回线程标记
    return !m_thread_status;
}

void AutoNode::setFolder(const std::string folder){
    m_auto_node->setFolder(folder);
}
void AutoNode::getFolder(std::string& folder){
    // do nothing
    m_auto_node->getFolder(folder);
}
} // namespace eagleeye