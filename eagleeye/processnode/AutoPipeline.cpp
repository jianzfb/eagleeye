#include "eagleeye/processnode/AutoPipeline.h"
#include "eagleeye/common/EagleeyeLog.h"
#include <functional>
#include <thread>
#include <chrono>
#include "opencv2/opencv.hpp"


namespace eagleeye{
AutoPipeline::AutoPipeline(std::function<AnyPipeline*()> pipeline_generator, std::vector<std::pair<std::string, int>> pipeline_node, int queue_size, bool get_then_auto_remove, bool set_then_auto_remove, bool copy_input){
    m_auto_pipeline = pipeline_generator();

    // 设置输出端口
    if(pipeline_node.size() == 0){
        std::vector<std::string> output_nodes;
        std::vector<std::string> output_types;
        std::vector<std::string> output_categorys;
        std::vector<std::string> output_targets;

        m_auto_pipeline->getPipelineOutputs(output_nodes, output_types, output_categorys, output_targets);
        for(int signal_i=0; signal_i<output_nodes.size(); ++signal_i){
            pipeline_node.push_back(std::make_pair(output_nodes[signal_i], 0));
        }
    }
    this->setNumberOfOutputSignals(pipeline_node.size());
    for(int signal_i=0; signal_i<pipeline_node.size(); ++signal_i){
        std::string node_name = pipeline_node[signal_i].first;
        int node_signal_i = pipeline_node[signal_i].second;

        AnySignal* output_signal = m_auto_pipeline->getNode(node_name)->getOutputPort(node_signal_i)->make();
        // 必须为队列信号
        output_signal->transformCategoryToQ(queue_size, get_then_auto_remove, set_then_auto_remove);
        output_signal->disableDataTimestamp();  // 禁用数据时间戳
        this->setOutputPort(output_signal, signal_i);
    }

    m_pipeline_node = pipeline_node;
    this->m_thread_status = true;
    this->m_is_ini = false;
    this->m_callback = nullptr;
    this->m_persistent_flag = false;

    this->m_enable_auto_stop = true;
    this->m_copy_input = copy_input;

    this->m_get_then_auto_remove = get_then_auto_remove;
    this->m_set_then_auto_remove = set_then_auto_remove;

    this->m_is_first_run = true;
}

AutoPipeline::~AutoPipeline(){
    // 重复尝试停止线程
    this->m_thread_status = false;
    if(this->m_is_ini){
        if(m_auto_thread.joinable()){
            m_auto_thread.join();
        }
    }

    // 删除内部节点
    if(m_auto_pipeline != NULL){
        delete m_auto_pipeline;
    }
}

void AutoPipeline::executeNodeInfo(){
    // do nothing
}

void AutoPipeline::run(){
    // 记录输入信号时间戳，用于防止重复数据计算
    if(m_last_timestamp.size() == 0){
        m_last_timestamp = std::vector<double>(this->getNumberOfInputSignals(), 0.0);
    }

    // 缓存数据（缓存上游的输入数据）
    std::vector<AnySignal*> cache_input;
    for(int signal_i=0; signal_i<this->getNumberOfInputSignals(); ++signal_i){
        cache_input.push_back(this->getInputPort(signal_i)->make());
    }

    // long before_time = EagleeyeTime::getCurrentTime();
    while(true){
        if(!this->m_thread_status){
            break;
        }

        int signal_num = this->getNumberOfInputSignals();
        bool is_duplicate_frame = true;
        int no_timestamp_signal_num = 0;
        std::vector<double> input_data_timestamp(signal_num, 0.0);

        // 1.step get input
        for(int signal_i = 0; signal_i<signal_num; ++signal_i){
            if(!this->m_thread_status){
                // 发现退出标记，退出线程运行
                break;
            }

            cache_input[signal_i]->copy(this->getInputPort(signal_i), false);
            if(!this->m_thread_status){
                // 发现退出标记，退出线程运行
                break;
            }
            if(!this->getInputPort(signal_i)->isEnable()){
                // 上游数据已经无效，主动设置退出标记
                this->m_thread_status = false;
                break;
            }

            MetaData data_meta = cache_input[signal_i]->meta();
            if(data_meta.timestamp > 0.0 && m_last_timestamp[signal_i] != data_meta.timestamp){
                // 只要有一个输入信号的时间戳发生了更新，则设置为非重复帧
                is_duplicate_frame = false;
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

        // TODO,对于异步管线，可以引入丢帧策略（如果在管线输入队列里存在超过限制，丢弃新数据）
        if(m_auto_pipeline->isAsyn()){
            while(true){
                bool is_overload = false;
                for(int signal_i = 0; signal_i<signal_num; ++signal_i){
                    std::string placeholder_name = std::string("placeholder_")+std::to_string(signal_i);
                    if(m_auto_pipeline->getInputQueueSize(placeholder_name.c_str()) > 2){
                        is_overload = true;
                        break;
                    }
                }
                if(is_overload){
                    EAGLEEYE_LOGD("Waiting 10ms, pipeline overload.");
                    std::this_thread::sleep_for(std::chrono::microseconds(10000));
                    continue;
                }
                break;
            }
        }

        // 填充到管线数据
        for(int signal_i = 0; signal_i<signal_num; ++signal_i){
            std::string placeholder_name = std::string("placeholder_")+std::to_string(signal_i);
            void* data;         // data address
            size_t* data_size;  // data size
            int data_dims=0;    // 3
            int data_type=0;    // DATA TYPE 
            MetaData data_meta;
            cache_input[signal_i]->getSignalContent(data, data_size, data_dims, data_type, data_meta);

            // // 保存图片，并打上时间戳，调试
            // cv::Mat image(data_size[0], data_size[1], CV_8UC3, data);
            // std::string time_str = std::to_string(this->m_last_timestamp[signal_i]);
            // cv::putText(image, time_str, cv::Point(100,100), 2, 1.0, cv::Scalar(255,0,0));

            // static int count = 0;
            // cv::imwrite(std::to_string(count)+".png", image);
            // count += 1;
            data_meta.rows = data_size[0];
            data_meta.cols = data_size[1];
            data_meta.rotation = 0;
            data_meta.allocate_mode = 0;        // 强迫复制模式，分配输入内存
            m_auto_pipeline->setInput(placeholder_name.c_str(), data, data_meta);
        }

        // 至此，已经获得一帧新数据

        // 尝试清理输入信号队列信息
        // 信号队列🈶三种清理队列数据机制
        // 1. 推入队列时，检查是否超出队列最大值，如果超出吐出尾部数据
        // 2. 读取队列时，是否直接将吐出队列数据
        // 3. 外部进行tryClear()，如果满足出度数，则吐出数据
        // for(int signal_i = 0; signal_i<signal_num; ++signal_i){
        //     this->getInputPort(signal_i)->tryClear();
        // }

        // 基于pipeline是否是异步，决定是否等待返回
        if(m_auto_pipeline->isAsyn()){
            // 异步管线，无需等待返回结果
            // 底层实现需要依靠回调实现数据输出

            // 仅在管线为异步时，考虑首帧问题
            // TODO,由于代理管线的部分节点（模型推理节点）初始化时间，等待代理管线的输入队列为空
            // 代理管线输入队列为空，表明代理管线已经整体完成一次运行
            if(m_is_first_run){
                while(true){
                    bool is_empty = true;
                    for(int signal_i = 0; signal_i<signal_num; ++signal_i){
                        std::string placeholder_name = std::string("placeholder_")+std::to_string(signal_i);
                        if(!(m_auto_pipeline->isInputQueueEmpty(placeholder_name.c_str()))){
                            is_empty = false;
                        }
                    }
                    if(!is_empty){
                        // 等待30ms，再进行检查
                        EAGLEEYE_LOGD("Waiting 30ms, for pipeline first run finish");
                        std::this_thread::sleep_for(std::chrono::microseconds(30000));
                        break;
                    }
                }
                m_is_first_run = false;
            }
            continue;
        }

        if(!this->m_thread_status){
            // 发现退出标记，退出线程运行
            break;
        }
    
        // 运行管线
        m_auto_pipeline->start();

        // 管线输出 
        signal_num = this->getNumberOfOutputSignals();
        bool is_auto_stop = false;
        for(int signal_i = 0; signal_i<signal_num; ++signal_i){
            std::string node_name = m_pipeline_node[signal_i].first;
            int node_signal_i = m_pipeline_node[signal_i].second;

            AnySignal* output_signal = m_auto_pipeline->getNode(node_name)->getOutputPort(node_signal_i);
            if(m_last_timestamp.size() > 0){
                output_signal->meta().timestamp = m_last_timestamp[0];
            }
            this->getOutputPort(signal_i)->copy(output_signal, true);
            if(m_auto_pipeline->getNode(node_name)->getOutputPort(node_signal_i)->meta().is_end_frame){
                is_auto_stop = true;
            }
        }

        // 回调
        if(this->m_callback != nullptr){
            this->m_callback(this, this->m_output_signals);
        }

        // 检查自动结束条件
        if(is_auto_stop && this->m_enable_auto_stop){
            m_thread_status = false;
            break;
        }
    }

    for(int signal_i = 0; signal_i<this->getNumberOfInputSignals(); ++signal_i){
        delete cache_input[signal_i];
    }
}

void AutoPipeline::exit(){
    Superclass::exit();
}

void AutoPipeline::preexit(){
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

void AutoPipeline::postexit(){
    if(this->m_persistent_flag){
        return;
    }
    
    // int signal_num = this->getNumberOfOutputSignals();
    // for(int signal_i = 0; signal_i<signal_num; ++signal_i){
    //     std::string node_name = m_pipeline_node[signal_i].first;
    //     int node_signal_i = m_pipeline_node[signal_i].second;

    //     AnySignal* output_signal = m_auto_pipeline->getNode(node_name)->getOutputPort(node_signal_i);
    //     this->getOutputPort(signal_i)->copy(output_signal);
    // }
    // // 触发自身，跳出数据等待，并结束线程
    // int signal_num = this->getNumberOfInputSignals();
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

void AutoPipeline::reset(){
    Superclass::reset();
    
    m_auto_pipeline->reset();
}

void AutoPipeline::init(){
    Superclass::init();

    if(!this->m_is_ini){
        m_auto_thread = std::thread(std::bind(&AutoPipeline::run,this));
        this->m_is_ini = true;
    }   
}

void AutoPipeline::updateUnitInfo(){
    modified();
    Superclass::updateUnitInfo();
}

void AutoPipeline::setUnitName(const char* unit_name){ 
    this->m_unit_name=std::string("auto-") + unit_name;
}

void AutoPipeline::setCallback(std::string node_name, std::function<void(AnyNode*, std::vector<AnySignal*>)> callback){
    if(node_name == ""){
        this->m_callback = callback;
        return;
    }

    std::string node_name_str = node_name;
    AnyNode* node = NULL;
    if (node_name_str.find("/") == std::string::npos) {
        node = this->m_auto_pipeline->getNode(node_name);
        node_name_str = "";
    }
    else{
        std::string separator = "/";
        std::vector<std::string> name_tree = split(node_name_str, separator);
        node = this->m_auto_pipeline->getNode(name_tree[0]);
        node_name_str = "";
        for(int i=1; i<name_tree.size(); ++i){
            if(i != name_tree.size() - 1){
                node_name_str += name_tree[i]+"/";
            }
            else{
                node_name_str += name_tree[i];
            }
        }
    }
    if(node == NULL){
        EAGLEEYE_LOGE("Node %s not exists.", node_name.c_str());
        return;
    }
    node->setCallback(node_name_str, callback);
}

bool AutoPipeline::stop(bool block, bool force){
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

void AutoPipeline::enableAutoStop(){
    this->m_enable_auto_stop = true;
}

void AutoPipeline::disableAutoStop(){
    this->m_enable_auto_stop = false;
}
}