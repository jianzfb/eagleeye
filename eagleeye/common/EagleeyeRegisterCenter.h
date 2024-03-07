#ifndef _EAGLEEYE_REGISTER_CENTER_H_
#define _EAGLEEYE_REGISTER_CENTER_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/common/EagleeyeStr.h"
#include <mutex>
#include <algorithm>  
#include <functional>
#include <map>
#include <memory>
#include <string>

namespace eagleeye{
class RegisterCenter{
public:
    virtual ~RegisterCenter();
    static RegisterCenter* getInstance();

    bool hasObj(std::string key);
    void* getObj(std::string key);
    bool registerObj(std::string key, void* obj, std::function<void(std::string,void*)> destroy_func);
    bool destroyObjWithPrefix(std::string prefix);
    bool destroyObj(std::string key);

private:
    RegisterCenter();
    std::map<std::string, std::pair<void*, std::function<void(std::string, void*)>>> m_register_map;
    static std::shared_ptr<RegisterCenter> m_instance;

    std::mutex m_mu;
}; 
}
#endif