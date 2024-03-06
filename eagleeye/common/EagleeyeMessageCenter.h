#ifndef _EAGLEEYELOG_H_
#define _EAGLEEYELOG_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/cJSON.h"

namespace eagleeye{
class MessageCenter{
public:
    MessageCenter();
    virtual ~MessageCenter();

    static MessageCenter* getInstance();

    std::shared_ptr<BSReplyMes> getNeedReply(std::string key, bool isFirst, int64_t fairKey, std::string keyPrefix);
    void insertMessage(std::string key, std::shared_ptr<BSReplyMes> message, bool isFinished = false);
    
    // 强制结束需要清空数据
    void clearMessage(std::string key);

private:
    std::map<std::string, std::priority_queue<std::shared_ptr<BSReplyMes>, std::vector<std::shared_ptr<BSReplyMes>>, BS::BSReplyMes>> replyMap;
    std::map<std::string, std::shared_ptr<BSSemaphore>> lockMap;
    std::map<std::string, bool> lockWaitStatus;
};
}


#endif