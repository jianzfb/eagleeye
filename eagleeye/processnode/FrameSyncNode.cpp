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
    std::cout<<"1"<<std::endl;
    int input_num = this->getNumberOfInputSignals();
    std::cout<<"input_num "<<input_num<<std::endl;
    std::cout<<"2"<<std::endl;
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
    std::cout<<"3"<<std::endl;

    // 组织同步帧
    double criterion_time_stamp = -1.0;
    int criterion_signal_i = -1;

    // 发现基准时间戳
    for(int signal_i=0; signal_i<input_num; ++signal_i){
        {
            std::cout<<"check sig "<<signal_i<<std::endl;
            std::unique_lock<std::mutex> locker(this->m_mu);
            while(this->m_meta_cache_queue[signal_i].size() == 0){
                this->m_cond.wait(locker);
                if(this->m_meta_cache_queue[signal_i].size() > 0){
                    break;
                }
            }
        }

        MetaData signal_meta = this->m_meta_cache_queue[signal_i].front();
        if(criterion_time_stamp < 0.0){
            criterion_time_stamp = signal_meta.timestamp;
            criterion_signal_i = signal_i;
        }
        else{
            if(signal_meta.timestamp > criterion_time_stamp){
                criterion_time_stamp = signal_meta.timestamp;
                criterion_signal_i = signal_i;
            }
        }
    }
    std::cout<<"4"<<std::endl;
    std::cout<<"criterion_time_stamp "<<criterion_time_stamp<<std::endl;
    std::cout<<"criterion_signal_i "<<criterion_signal_i<<std::endl;

    // 至此，肯定会找到一个基准时间点
    if(criterion_signal_i < 0){
        EAGLEEYE_LOGE("Conldnt find criterion signal.");
        return;
    }

    std::cout<<"5"<<std::endl;

    std::unique_lock<std::mutex> locker(this->m_mu);
    for(int signal_i=0; signal_i<input_num; ++signal_i){
        std::cout<<"xx"<<std::endl;
        while(this->m_meta_cache_queue[signal_i].size() == 0){
            this->m_cond.wait(locker);
            if(this->m_meta_cache_queue[signal_i].size() > 0){
                break;
            }
        }

        std::cout<<"yy"<<std::endl;
        std::cout<<"signal_i "<<signal_i<<std::endl;

        Matrix<Array<unsigned char, 3>> signal_frame = this->m_frame_cache_queue[signal_i].front();
        MetaData signal_meta = this->m_meta_cache_queue[signal_i].front();
        while(std::abs(signal_meta.timestamp - criterion_time_stamp) > m_sync_time_delta){
            this->m_frame_cache_queue[signal_i].pop();
            this->m_meta_cache_queue[signal_i].pop();

            while(this->m_frame_cache_queue[signal_i].size() == 0){
                this->m_cond.wait(locker);
                if(this->m_frame_cache_queue[signal_i].size() > 0){
                    break;
                }
            }
            signal_frame = this->m_frame_cache_queue[signal_i].front();
            signal_meta = this->m_meta_cache_queue[signal_i].front();
        }

        std::cout<<"set output "<<signal_i<<std::endl;

        ImageSignal<Array<unsigned char, 3>>* img_sig = (ImageSignal<Array<unsigned char, 3>>*)(this->getOutputPort(signal_i));
        std::cout<<"1111"<<std::endl;
        img_sig->setData(signal_frame, signal_meta);
        std::cout<<"2222"<<std::endl;
        this->m_frame_cache_queue[signal_i].pop();
        std::cout<<"3333"<<std::endl;
        this->m_meta_cache_queue[signal_i].pop();
        std::cout<<"4444"<<std::endl;
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

        int signal_num = this->getNumberOfInputSignals();
        for(int signal_i=0; signal_i<signal_num; ++signal_i){
            ImageSignal<Array<unsigned char, 3>>* img_sig = (ImageSignal<Array<unsigned char, 3>>*)(this->getInputPort(signal_i));

            std::unique_lock<std::mutex> locker(this->m_mu);
            std::cout<<"m_frame_cache_queue. size "<<m_frame_cache_queue.size()<<std::endl;
            while(this->m_frame_cache_queue[signal_i].size() >= m_max_cache_frame_num){
                locker.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                locker = std::unique_lock<std::mutex>(this->m_mu);
            }

            std::cout<<"push signal i "<<signal_i<<std::endl;
            MetaData meta;
            Matrix<Array<unsigned char, 3>> data = img_sig->getData(meta);
            this->m_frame_cache_queue[signal_i].push(data);
            this->m_meta_cache_queue[signal_i].push(meta);
            locker.unlock();
        }

        this->m_cond.notify_all();
    }
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