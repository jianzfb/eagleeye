#include "eagleeye/processnode/PipelineNode.h"
#include "eagleeye/common/EagleeyeLog.h"
#include <functional>
#include <thread>
#include <chrono>

namespace eagleeye{
PipelineNode::PipelineNode(std::function<AnyPipeline*()> pipeline_generator, std::vector<std::pair<std::string, int>> pipeline_node){
    m_auto_pipeline = pipeline_generator();

    // 设置输出端口
    this->setNumberOfOutputSignals(pipeline_node.size());
    for(int signal_i=0; signal_i<pipeline_node.size(); ++signal_i){
        std::string node_name = pipeline_node[signal_i].first;
        int node_signal_i = pipeline_node[signal_i].second;

        AnySignal* output_signal = m_auto_pipeline->getNode(node_name)->getOutputPort(node_signal_i)->make();
        this->setOutputPort(output_signal, signal_i);
    }

    m_pipeline_node = pipeline_node;
}

PipelineNode::~PipelineNode(){
    // 删除内部节点
    delete m_auto_pipeline;

    for(int cache_input_i=0; cache_input_i<m_cache_input.size(); ++cache_input_i){
        delete m_cache_input[cache_input_i];
    }
}

void PipelineNode::executeNodeInfo(){
    // 管线输入
    int signal_num = this->getNumberOfInputSignals();
    if(m_cache_input.size() == 0){
        for(int signal_i=0; signal_i<signal_num; ++signal_i){
            m_cache_input.push_back(
                this->getInputPort(signal_i)->make()
            );
        }
    }

    for(int signal_i = 0; signal_i<signal_num; ++signal_i){
        void* data;         // data address
        size_t* data_size;  // data size
        int data_dims=0;    // 3
        int data_type=0;    // DATA TYPE 
        MetaData data_meta;
        m_cache_input[signal_i]->copy(this->getInputPort(signal_i));
        m_cache_input[signal_i]->getSignalContent(data, data_size, data_dims, data_type, data_meta);

        std::string placeholder_name = std::string("placeholder_")+std::to_string(signal_i);
        data_meta.rows = data_size[0];
        data_meta.cols = data_size[1];
        data_meta.rotation = 0;
        m_auto_pipeline->setInput(placeholder_name.c_str(), data, data_meta);
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
}

void PipelineNode::setUnitName(const char* unit_name){ 
    this->m_unit_name=std::string("pipelinenode-") + unit_name;
}
}