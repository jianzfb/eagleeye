#include "eagleeye/processnode/CacheNode.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include <functional>
#include <thread>
#include <chrono>

namespace eagleeye{
CacheNode::CacheNode(std::function<void(GroupSignal*)> post_process){
    m_cache_i = -1;
    m_cache_size = 0;
    this->setNumberOfOutputSignals(1);
    this->setOutputPort(new GroupSignal(), 0);
    this->setNumberOfInputSignals(2);

    m_post_process = post_process;
}

CacheNode::~CacheNode(){
    for(int i=0; i<m_cache_queue.size(); ++i){
        if(m_cache_queue[i] != NULL){
            delete m_cache_queue[i];
            m_cache_queue[i] = NULL;
        }
    }
}

void CacheNode::executeNodeInfo(){
    // input
    // TODO, support any 
    if(!(this->getInputPort(0)->isempty())){
        m_cache_i += 1;
        m_cache_i = m_cache_i % m_cache_size;
        if(m_cache_queue[m_cache_i] != NULL){
            delete m_cache_queue[m_cache_i];
            m_cache_queue[m_cache_i] = NULL;
        }
        m_cache_queue[m_cache_i] = this->getInputPort(0)->make();
        m_cache_queue[m_cache_i]->copy(this->getInputPort(0));
    }

    // output
    ImageSignal<int>* select_i_sig = (ImageSignal<int>*)(this->getInputPort(1));
    Matrix<int> select_i_info = select_i_sig->getData();
    GroupSignal* group_signal = (GroupSignal*)(this->getOutputPort(0));
    group_signal->makeempty();  
    if(select_i_info.empty()){
        return;
    }

    int num = select_i_info.rows() * select_i_info.cols();
    int* select_i_ptr = select_i_info.cpu<int>();
    std::vector<AnySignal*> temp;
    for(int i=0; i<num; ++i){
        int select_i = select_i_ptr[i];
        int cache_i = m_cache_i + select_i;
        cache_i = cache_i + (int(std::abs(cache_i)/m_cache_size)+1) * m_cache_size;
        cache_i = cache_i % m_cache_size;

        if(m_cache_queue[cache_i] == NULL){
            continue;
        }
        temp.push_back(m_cache_queue[cache_i]);
    }
    group_signal->setData(temp);
    if(m_post_process != nullptr){
        m_post_process(group_signal);
    }
}

void CacheNode::setCacheSize(int size){
    for(int i=0; i<m_cache_queue.size(); ++i){
        if(m_cache_queue[i] != NULL){
            delete m_cache_queue[i];
            m_cache_queue[i] = NULL;
        }
    }
    m_cache_size = size;
    m_cache_queue.resize(size);
    for(int i=0; i<m_cache_queue.size(); ++i){
        m_cache_queue[i] = NULL;
    }
}
int CacheNode::getCacheSize(int& size){
    size = m_cache_size;
}
}