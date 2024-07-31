#include "eagleeye/processnode/FrameSyncNode.h"
namespace eagleeye{
FrameSyncNode::FrameSyncNode(){
    this->m_thread_status = true;
    this->m_is_ini = false;
    this->m_max_cache_frame_num = 300;  // 最大允许差别10s (10*30)
    m_sync_time_delta = 0.05;           // 0.05s同步误差
}
FrameSyncNode::~FrameSyncNode(){
    this->m_thread_status = false;
    if(this->m_is_ini){
        if(m_auto_thread.joinable()){
            m_auto_thread.join();
        }
    }
}

void FrameSyncNode::executeNodeInfo(){
    int input_num = this->getNumberOfInputSignals();
    if(!this->m_is_ini){
        if(m_frame_cache_queue.size() != input_num){
            m_frame_cache_queue.resize(input_num);
        }
        if(m_meta_cache_queue.size() != input_num){
            m_meta_cache_queue.resize(input_num);
        }

        m_auto_thread = std::thread(std::bind(&FrameSyncNode::run,this));
        m_is_ini = true;
    }

    while(true){
        std::unique_lock<std::mutex> locker(this->m_mu);
        // 计算最大时间跨度
        double min_timestamp = std::numeric_limits<double>::max();
        int min_signal_i = -1;
        double max_timestamp = std::numeric_limits<double>::min();

        while(std::any_of(this->m_meta_cache_queue.begin(), this->m_meta_cache_queue.end(), [](const std::queue<MetaData>& q){return q.empty();})){
            this->m_cond.wait(locker);
            EAGLEEYE_LOGD("m_meta_cache_queue has empty queue, wait ...");
        }
        for(int signal_i=0; signal_i<input_num; ++signal_i){
            Matrix<Array<unsigned char, 3>> signal_frame = this->m_frame_cache_queue[signal_i].front();
            MetaData signal_meta = this->m_meta_cache_queue[signal_i].front();
            if(min_timestamp >= signal_meta.timestamp){
                min_timestamp = signal_meta.timestamp;
                min_signal_i = signal_i;
            }

            if(max_timestamp <= signal_meta.timestamp){
                max_timestamp = signal_meta.timestamp;
            }
        }

        if(max_timestamp - min_timestamp > this->m_sync_time_delta){
            // 除去最早时间数据
            this->m_frame_cache_queue[min_signal_i].pop();
            this->m_meta_cache_queue[min_signal_i].pop();
        }
        else{
            // 成功发现同步帧
            for(int signal_i=0; signal_i<input_num; ++signal_i){
                while(this->m_meta_cache_queue[signal_i].size() == 0){
                    EAGLEEYE_LOGD("fatal error, meta_cache_queue will no empty");
                    this->m_cond.wait(locker);
                    if(this->m_meta_cache_queue[signal_i].size() > 0){
                        break;
                    }
                }
                Matrix<Array<unsigned char, 3>> signal_frame = this->m_frame_cache_queue[signal_i].front();
                MetaData signal_meta = this->m_meta_cache_queue[signal_i].front();

                ImageSignal<Array<unsigned char, 3>>* img_sig = (ImageSignal<Array<unsigned char, 3>>*)(this->getOutputPort(signal_i));
                img_sig->setData(signal_frame, signal_meta);
                this->m_frame_cache_queue[signal_i].pop();
                this->m_meta_cache_queue[signal_i].pop();
            }
            EAGLEEYE_LOGD("fraemSyncNode return success");
            break;
        }
    }
}

void FrameSyncNode::addInputPort(AnySignal* sig){
    int signal_num = this->getNumberOfInputSignals();
    this->setInputPort(sig, signal_num);
}

void FrameSyncNode::setInputPort(AnySignal* sig,int index){
    if(this->getNumberOfInputSignals() < index + 1){
        this->setNumberOfInputSignals(index + 1);
    }

    if(this->getNumberOfOutputSignals() < index + 1){
        this->setNumberOfOutputSignals(index + 1);
    }

    Superclass::setInputPort(sig, index);
    if(this->getOutputPort(index) == NULL){
        this->setOutputPort(sig->make(), index);
    }
}

void FrameSyncNode::run(){
    while (true){
        if(!this->m_thread_status){
            break;
        }

        // 缓存输入帧数据
        int signal_num = this->getNumberOfInputSignals();
        std::unique_lock<std::mutex> locker(this->m_mu);
        for(int signal_i=0; signal_i<signal_num; ++signal_i){
            if(m_frame_cache_queue[signal_i].size() < m_max_cache_frame_num){
                ImageSignal<Array<unsigned char, 3>>* img_sig = (ImageSignal<Array<unsigned char, 3>>*)(this->getInputPort(signal_i));
                MetaData meta;
                Matrix<Array<unsigned char, 3>> data = img_sig->getData(meta);
                this->m_frame_cache_queue[signal_i].push(data.clone());
                this->m_meta_cache_queue[signal_i].push(meta);
            }
        }
        locker.unlock();

        bool all_cache_queue_full = std::all_of(m_frame_cache_queue.begin(), m_frame_cache_queue.end(), [&](const std::queue<Matrix<Array<unsigned char,3>>>& q){
            return q.size() >= m_max_cache_frame_num;
        });

        // 通知
        this->m_cond.notify_all();

        if(all_cache_queue_full){
            EAGLEEYE_LOGD("warning!!, all queue is full, please check");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}


void FrameSyncNode::processUnitInfo(){
    Superclass::processUnitInfo();
    modified();
}


void FrameSyncNode::preexit(){
    // prepare exit
    this->m_thread_status = false;
}

void FrameSyncNode::postexit(){
    if(this->m_is_ini){
        if(m_auto_thread.joinable()){
            m_auto_thread.join();
        }
    }
}
}