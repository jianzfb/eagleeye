#ifndef _EAGLEEYE_MESSAGE_CENTER_H_
#define _EAGLEEYE_MESSAGE_CENTER_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/EagleeyeMessage.h"
#include <map>
#include <queue>
#include <functional> 
#include <memory>
#include <mutex>
#include <condition_variable>

namespace eagleeye{
class MessageLock{
public:
    MessageLock(){
        status = true;
    }
    std::mutex mu;
    std::condition_variable cond;
    bool status;
};

class MessageCenter{
public:
    virtual ~MessageCenter();
    static MessageCenter* getInstance();

    // 基于key消费队列消息
    std::shared_ptr<Message> get(std::string key);

    // 基于key插入消息进入队列
    bool insert(std::string key, std::shared_ptr<Message> message);

    // 清空key消息队列
    bool clear(std::string key);

    // 基于key创建消息队列
    bool create(std::string key);

    // 基于key删除消息队列
    bool remove(std::string key);

private:
    MessageCenter();

    std::map<std::string, std::priority_queue<std::shared_ptr<Message>, std::vector<std::shared_ptr<Message>>, MessageCmp>> m_message_map;
    std::map<std::string, std::shared_ptr<MessageLock>> m_lock_map;
    std::mutex m_mu;

    static std::shared_ptr<MessageCenter> m_instance;
};
}


#endif