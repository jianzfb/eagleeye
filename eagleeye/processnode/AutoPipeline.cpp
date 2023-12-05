#include "eagleeye/processnode/AutoPipeline.h"
#include "eagleeye/common/EagleeyeLog.h"
#include <functional>
#include <thread>
#include <chrono>

namespace eagleeye{
AutoPipeline::AutoPipeline(std::function<AnyPipeline*()> pipeline_generator, std::vector<std::pair<std::string, int>> pipeline_node, int queue_size, bool get_then_auto_remove){
    m_auto_pipeline = pipeline_generator();

    // 设置输出端口
    this->setNumberOfOutputSignals(pipeline_node.size());
    for(int signal_i=0; signal_i<pipeline_node.size(); ++signal_i){
        std::string node_name = pipeline_node[signal_i].first;
        int node_signal_i = pipeline_node[signal_i].second;

        AnySignal* output_signal = m_auto_pipeline->getNode(node_name)->getOutputPort(node_signal_i)->make();
        // 必须为队列信号
        output_signal->transformCategoryToQ(queue_size, get_then_auto_remove);
        this->setOutputPort(output_signal, signal_i);
    }

    m_pipeline_node = pipeline_node;
    this->m_thread_status = true;
    this->m_is_ini = false;
    this->m_last_timestamp = 0.0;
    this->m_callback = nullptr;
    this->m_persistent_flag = false;
}

AutoPipeline::~AutoPipeline(){
    delete m_auto_pipeline;
}

void AutoPipeline::executeNodeInfo(){
    // do nothing
}

void AutoPipeline::run(){
    while(true){
        if(!this->m_thread_status){
            break;
        }

        // 管线输入
        int signal_num = this->getNumberOfInputSignals();
        bool is_duplicate_frame = false;
        double input_data_timestamp = 0.0;
        for(int signal_i = 0; signal_i<signal_num; ++signal_i){
            void* data;         // data address
            size_t* data_size;  // data size
            int data_dims=0;    // 3
            int data_type=0;    // DATA TYPE 
            MetaData data_meta;
            this->getInputPort(signal_i)->getSignalContent(data, data_size, data_dims, data_type, data_meta);
            if(!this->m_thread_status){
                break;
            }

            std::string placeholder_name = std::string("placeholder_")+std::to_string(signal_i);
            data_meta.rows = data_size[0];
            data_meta.cols = data_size[1];
            data_meta.rotation = 0;
            if(data_meta.timestamp > 0.0 && m_last_timestamp == data_meta.timestamp){
                is_duplicate_frame = true;
            }
            input_data_timestamp = data_meta.timestamp;
            if(!is_duplicate_frame){
                m_auto_pipeline->setInput(placeholder_name.c_str(), data, data_meta);
            }
        }
        if(is_duplicate_frame){
            // 重复帧，休息1ms
            std::this_thread::sleep_for(std::chrono::microseconds(1000));
            continue;
        }
        this->m_last_timestamp = input_data_timestamp;
        if(!this->m_thread_status){
            break;
        }

        // 运行管线
        m_auto_pipeline->start();

        // 管线输出 
        signal_num = this->getNumberOfOutputSignals();
        for(int signal_i = 0; signal_i<signal_num; ++signal_i){
            std::string node_name = m_pipeline_node[signal_i].first;
            int node_signal_i = m_pipeline_node[signal_i].second;

            AnySignal* output_signal = m_auto_pipeline->getNode(node_name)->getOutputPort(node_signal_i);
            this->getOutputPort(signal_i)->copy(output_signal);
        }

        if(this->m_callback != nullptr){
            this->m_callback(this, this->m_output_signals);
        }
    }
}

void AutoPipeline::preexit(){
    if(this->m_persistent_flag){
        return;
    }

    // prepare exit
    this->m_thread_status = false;
}

void AutoPipeline::postexit(){
    if(this->m_persistent_flag){
        return;
    }
    
    int signal_num = this->getNumberOfOutputSignals();
    for(int signal_i = 0; signal_i<signal_num; ++signal_i){
        std::string node_name = m_pipeline_node[signal_i].first;
        int node_signal_i = m_pipeline_node[signal_i].second;

        AnySignal* output_signal = m_auto_pipeline->getNode(node_name)->getOutputPort(node_signal_i);
        this->getOutputPort(signal_i)->copy(output_signal);
    }

    if(this->m_is_ini){
        if(m_auto_thread.joinable()){
            m_auto_thread.join();
        }
    }
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

void AutoPipeline::setCallback(std::function<void(AnyNode*, std::vector<AnySignal*>)> callback){
    this->m_callback = callback;
}
}