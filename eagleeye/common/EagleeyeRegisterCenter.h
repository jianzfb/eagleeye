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
#include <chrono>
#include <future>
#include <thread>
using namespace std::chrono_literals;

namespace eagleeye{
class RegisterCenter{
public:
    virtual ~RegisterCenter();
    static RegisterCenter* getInstance();

    bool hasObjWithPrefix(std::string prefix);
    bool hasObj(std::string key);

    void* getObj(std::string key);
    bool registerObj(std::string key, void* obj, std::function<void(std::string,void*)> destroy_func);
    bool destroyObjWithPrefix(std::string prefix);
    bool destroyObj(std::string key);
    void enableTimeout(int timeout_seconds);

private:
    RegisterCenter();
    std::map<std::string, std::pair<void*, std::function<void(std::string, void*)>>> m_register_map;
    std::map<std::string, std::chrono::steady_clock::time_point> m_register_start_time;
    static std::shared_ptr<RegisterCenter> m_instance;

    std::mutex m_mu;

    void startTimerThread(int timeout_seconds);
    std::atomic_bool m_run_check = false;
    std::future<void> m_future;

}; 
}
#endif