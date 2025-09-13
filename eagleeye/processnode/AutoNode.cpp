#include "eagleeye/processnode/AutoNode.h"
#include "eagleeye/common/EagleeyeTime.h"
#include <functional>
#include <thread>
#include <chrono>

namespace eagleeye
{
AutoNode::AutoNode(std::function<AnyNode*()> generator, int queue_size, bool get_then_auto_remove,  bool set_then_auto_remove, bool copy_input){
    // auto_node 必须非异步
    m_auto_node = generator();
    // 设置输出端口
    int signal_num = m_auto_node->getNumberOfOutputSignals();
    this->setNumberOfOutputSignals(signal_num);
    for(int signal_i=0; signal_i<signal_num; ++signal_i){
        AnySignal* output_signal = m_auto_node->getOutputPort(signal_i)->make();
        // 必须为队列信号
        output_signal->transformCategoryToQ(queue_size, get_then_auto_remove, set_then_auto_remove);
        output_signal->disableDataTimestamp();  // 禁用数据时间戳
        this->setOutputPort(output_signal, signal_i);
    }

    EAGLEEYE_MONITOR_VAR(std::string, setFolder, getFolder, "folder", "", "");

    this->m_thread_status = true;
    this->m_is_ini = false;
    this->m_callback = nullptr;
    this->m_persistent_flag = false;
    this->m_copy_input = copy_input;
    this->m_get_then_auto_remove = get_then_auto_remove;
    this->m_set_then_auto_remove = set_then_auto_remove;
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
    // TODO，忽略run_in_no_copy_input调用（可能存在风险，在aisports 50x8里 使用的是run_in_copy_input，需要进一步查看是否有影响）
    run_in_copy_input();
}

void AutoNode::run_in_copy_input(){
    // 记录输入信号时间戳，用于防止重复数据计算
    if(m_last_timestamp.size() == 0){
        m_last_timestamp = std::vector<double>(this->getNumberOfInputSignals(), 0.0);
    }

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

        int signal_num = this->getNumberOfInputSignals();
        bool is_duplicate_frame = false;
        int no_timestamp_signal_num = 0;
        std::vector<double> input_data_timestamp(this->getNumberOfInputSignals(), 0.0);

        // 1.step get input
        for(int signal_i = 0; signal_i<signal_num; ++signal_i){
            if(!this->m_thread_status){
                // 发现退出标记，退出线程运行
                break;
            }

            // block call
            signal_list[signal_i]->copy(this->getInputPort(signal_i), false);
            if(!this->m_thread_status){
                // 发现退出标记，退出线程运行
                break;
            }
            if(!this->getInputPort(signal_i)->isEnable()){
                // 上游数据已经无效，主动设置退出标记
                this->m_thread_status = false;
                break;
            }

            // set input
            MetaData data_meta = signal_list[signal_i]->meta();
            if(data_meta.timestamp > 0.0 && m_last_timestamp[signal_i] == data_meta.timestamp){
                is_duplicate_frame = true;
            }
            if(int(data_meta.timestamp) == 0){
                no_timestamp_signal_num += 1;
            }
            input_data_timestamp[signal_i] = data_meta.timestamp;
        }

        if(!this->m_thread_status){
            // 发现退出标记，退出线程运行
            break;
        }

        // 如果所有输入信号都是无时间戳信号，则忽略重复帧去除
        if(is_duplicate_frame && no_timestamp_signal_num != signal_num){
            // 重复帧，休息10ms
            std::this_thread::sleep_for(std::chrono::microseconds(10000));
            continue;
        }
        if(!this->m_thread_status){
            // 发现退出标记，退出线程运行
            break;
        }

        // 产生有效帧，驱动管线执行
        this->m_last_timestamp = input_data_timestamp;

        // 至此，已经获得一帧新数据
        if(!this->m_thread_status){
            // 发现退出标记，退出线程运行
            break;
        }

        // 2.step run node
        long before_time = EagleeyeTime::getCurrentTime();
        bool running_ischange = m_auto_node->start();

        // 尝试清理输入信号队列信息
        // 信号队列🈶三种清理队列数据机制
        // 1. 推入队列时，检查是否超出队列最大值，如果超出吐出尾部数据
        // 2. 读取队列时，是否直接将吐出队列数据
        // 3. 外部进行tryClear()，如果满足出度数，则吐出数据
        for(int signal_i = 0; signal_i<signal_num; ++signal_i){
            bool is_clear = this->getInputPort(signal_i)->tryClear();
        }

        if(!running_ischange){
            continue;
        }

        // 3.step get output
        signal_num = m_auto_node->getNumberOfOutputSignals();
        bool is_auto_stop = false;

        for(int signal_i = 0; signal_i<signal_num; ++signal_i){
            // 时间戳填充
            if(m_last_timestamp.size() > 0){
                m_auto_node->getOutputPort(signal_i)->meta().timestamp = m_last_timestamp[0];
            }

            this->getOutputPort(signal_i)->copy(m_auto_node->getOutputPort(signal_i), true);
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

    // for(int signal_i = 0; signal_i<this->getNumberOfInputSignals(); ++signal_i){
    //     m_auto_node->clearInputPort(signal_i);
    // }
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
    // if(m_auto_node){
    //     this->m_auto_node->exit();
    // }
}

void AutoNode::preexit(){
    if(this->m_persistent_flag){
        return;
    }

    // prepare exit
    this->m_thread_status = false;

    // 触发自身，跳出数据等待，并结束线程
    // 可以保证，下游节点先退出，游节点再退出
    int signal_num = this->getNumberOfInputSignals();
    for(int signal_i = 0; signal_i<signal_num; ++signal_i){
        this->getInputPort(signal_i)->disable();
        this->getInputPort(signal_i)->wake();
    }

    if(this->m_is_ini){
        if(m_auto_thread.joinable()){
            m_auto_thread.join();
        }
    }
}

void AutoNode::postexit(){
    if(this->m_persistent_flag){
        return;
    }

    // int signal_num = m_auto_node->getNumberOfOutputSignals();
    // for(int signal_i = 0; signal_i<signal_num; ++signal_i){
    //     // dont care data is what
    //     this->getOutputPort(signal_i)->copy(m_auto_node->getOutputPort(signal_i));        
    // }
    // 触发自身，跳出数据等待，并结束线程
    // int signal_num = m_auto_node->getNumberOfInputSignals();
    // for(int signal_i = 0; signal_i<signal_num; ++signal_i){
    //     this->getInputPort(signal_i)->disable();
    //     this->getInputPort(signal_i)->wake();
    // }

    // if(this->m_is_ini){
    //     if(m_auto_thread.joinable()){
    //         m_auto_thread.join();
    //     }
    // }
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

void AutoNode::setCallback(std::string name, std::function<void(AnyNode*, std::vector<AnySignal*>)> callback){
    if(name == ""){
        this->m_callback = callback;
        return;
    }
    
    std::string next_name = "";
    if (name.find("/") == std::string::npos) {
        next_name = name;
    }
    else{
        std::string separator = "/";
        std::vector<std::string> name_tree = split(name, separator);
        next_name = "";
        for(int i=1; i<name_tree.size(); ++i){
            if(i != name_tree.size() - 1){
                next_name += name_tree[i]+"/";
            }
            else{
                next_name += name_tree[i];
            }
        }
    }
    this->m_auto_node->setCallback(next_name, callback);
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