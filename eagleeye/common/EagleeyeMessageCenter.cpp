#include "eagleeye/common/EagleeyeMessageCenter.h"
#include "eagleeye/common/EagleeyeLog.h"
namespace eagleeye{
std::shared_ptr<MessageCenter> MessageCenter::m_instance(new MessageCenter(), [](MessageCenter* d) { delete d; });

MessageCenter::MessageCenter(){}
MessageCenter::~MessageCenter(){}

MessageCenter* MessageCenter::getInstance(){
  return m_instance.get();
}

std::shared_ptr<Message> MessageCenter::get(std::string key){
    // 获得消息
    {
        std::unique_lock<std::mutex> locker(m_mu);
        if(m_lock_map.find(key) == m_lock_map.end()){
            return std::shared_ptr<Message>();
        }
    }

    std::unique_lock<std::mutex> message_locker(m_lock_map[key]->mu);
    while(m_message_map[key].size() == 0 && m_lock_map[key]->status){
        m_lock_map[key]->cond.wait(message_locker);
        if((m_message_map[key].size() > 0) || (!(m_lock_map[key]->status))){
            break;
        }
    }
    if(!m_lock_map[key]->status){
        message_locker.unlock();
        return std::shared_ptr<Message>();
    }

    std::shared_ptr<Message> message = m_message_map[key].top();
    m_message_map[key].pop();
    message_locker.unlock();
    return message;
}

bool MessageCenter::insert(std::string key, std::shared_ptr<Message> message){
    // 添加消息
    {
        std::unique_lock<std::mutex> locker(m_mu);
        if(m_lock_map.find(key) == m_lock_map.end()){
            return false;
        }
    }

    std::unique_lock<std::mutex> message_locker(m_lock_map[key]->mu);
    if(!m_lock_map[key]->status){
        message_locker.unlock();
        return false;
    }

    this->m_message_map[key].push(message); 
	message_locker.unlock();

    // notify
    m_lock_map[key]->cond.notify_all();
    return true;
}

bool MessageCenter::create(std::string key){
    std::unique_lock<std::mutex> locker(m_mu);
    if(m_lock_map.find(key) != m_lock_map.end()){
        m_lock_map[key]->status = true;
        return true;
    }

    m_lock_map[key] = std::shared_ptr<MessageLock>(new MessageLock(), [](MessageLock* d){delete d;});
    m_lock_map[key]->status = true;
    m_message_map[key] = std::priority_queue<std::shared_ptr<Message>, std::vector<std::shared_ptr<Message>>, MessageCmp>();
    return true;
}

bool MessageCenter::clear(std::string key){
    {
        std::unique_lock<std::mutex> locker(m_mu);
        if(m_lock_map.find(key) == m_lock_map.end()){
            return true;
        }
    }

    std::unique_lock<std::mutex> message_locker(m_lock_map[key]->mu);
    this->m_lock_map[key]->status = false;
    while(m_message_map[key].size() != 0){
        m_message_map[key].pop();
    }
    message_locker.unlock();
    this->m_lock_map[key]->cond.notify_all();
    return true;
}

bool MessageCenter::remove(std::string key){
    std::unique_lock<std::mutex> locker(m_mu);
    if(m_lock_map.find(key) == m_lock_map.end()){
        return true;
    }

    m_lock_map.erase(key);
    m_message_map.erase(key);
    return true;
}
}