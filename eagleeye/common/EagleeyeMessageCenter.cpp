#include "eagleeye/common/EagleeyeMessageCenter.h"
#include "eagleeye/common/EagleeyeLog.h"
namespace eagleeye{
std::shared_ptr<MessageCenter> MessageCenter::m_instance(new MessageCenter(), [](MessageCenter* d) { delete d; });

MessageCenter::MessageCenter(){

}
MessageCenter::~MessageCenter(){

}

MessageCenter* MessageCenter::getInstance(){
    return m_instance.get();
}

std::shared_ptr<Message> MessageCenter::get(std::string key, int timeout){
    // 获得消息
    std::unique_lock<std::mutex> locker(m_mu);    
    if(m_message_map.find(key) == m_message_map.end()){
        locker.unlock();
        return std::shared_ptr<Message>();
    }

    while(m_message_map[key].second.size() == 0){
        if(timeout <= 0){
            m_cond.wait(locker);
        }
        else{
            if(m_cond.wait_for(locker, std::chrono::seconds(timeout)) == std::cv_status::timeout){
                // 超时，不再等待
                return std::shared_ptr<Message>();
            }
        }
    }

    std::shared_ptr<Message> message = m_message_map[key].second.top();
    m_message_map[key].second.pop();
    locker.unlock();
    return message;
}

bool MessageCenter::insert(std::string key, std::shared_ptr<Message> message){
    // 添加消息
    std::unique_lock<std::mutex> locker(m_mu);
    if(m_message_map.find(key) == m_message_map.end()){
        locker.unlock();
        return false;
    }

    this->m_message_map[key].second.push(message); 
	locker.unlock();

    // notify
    m_cond.notify_all();
    return true;
}

bool MessageCenter::create(std::string key){
    std::unique_lock<std::mutex> locker(m_mu);
    if(m_message_map.find(key) != m_message_map.end()){
        EAGLEEYE_LOGE("Fail to create duplicate name message.");
        return false;
    }
    m_message_map[key] = std::make_pair(
        0,
        std::priority_queue<std::shared_ptr<Message>, std::vector<std::shared_ptr<Message>>, MessageCmp>()
    );
    return true;
}

bool MessageCenter::remove(std::string key){
    std::unique_lock<std::mutex> locker(m_mu);
    if(m_message_map.find(key) == m_message_map.end()){
        return false;
    }

    m_message_map.erase(key);
    return true;
}

bool MessageCenter::clear(std::string key){
    std::unique_lock<std::mutex> locker(m_mu);
    if(m_message_map.find(key) == m_message_map.end()){
        locker.unlock();
        return false;
    }

    while(m_message_map[key].second.size() != 0){
        m_message_map[key].second.pop();
    }
    locker.unlock();
    return true;
}
}