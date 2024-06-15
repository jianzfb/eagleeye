#include "eagleeye/common/EagleeyeRegisterCenter.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/EagleeyeLog.h"
namespace eagleeye{
std::shared_ptr<RegisterCenter> RegisterCenter::m_instance(new RegisterCenter(), [](RegisterCenter* d) { delete d; });    
RegisterCenter::RegisterCenter(){}
RegisterCenter::~RegisterCenter(){
    m_run_check = false;
    if(m_future.valid()){
        m_future.wait();
    }
}
RegisterCenter* RegisterCenter::getInstance(){
    return m_instance.get();
}

bool RegisterCenter::hasObj(std::string key){
    std::unique_lock<std::mutex> locker(m_mu);
    if(m_register_map.find(key) == m_register_map.end()){
        return false;
    }
    return true;
}

bool RegisterCenter::hasObjWithPrefix(std::string prefix){
    std::unique_lock<std::mutex> locker(m_mu);
    std::map<std::string, std::pair<void*, std::function<void(std::string, void*)>>>::iterator iter, iend(m_register_map.end());
    for(iter = m_register_map.begin(); iter != iend; ++iter){
        if(startswith(iter->first, prefix)){
            return true;
        }
    }
    return false;
}

void* RegisterCenter::getObj(std::string key){
    std::unique_lock<std::mutex> locker(m_mu);
    if(m_register_map.find(key) == m_register_map.end()){
        return NULL;
    }
    return m_register_map[key].first;
}

std::string RegisterCenter::getInfo(std::string key){
    std::unique_lock<std::mutex> locker(m_mu);
    if(m_register_info.find(key) == m_register_info.end()){
        return "";
    }
    return m_register_info[key];
}

bool RegisterCenter::registerObj(std::string key, void* obj, std::function<void(std::string, void*)> destroy_func, std::string info){
    std::unique_lock<std::mutex> locker(m_mu);
    if(m_register_map.find(key) != m_register_map.end()){
        return false;
    }

    m_register_map[key] = std::make_pair(obj, destroy_func);
    m_register_start_time[key] = std::chrono::steady_clock::now();
    m_register_info[key] = info;
    return true;
}

bool RegisterCenter::destroyObj(std::string key){
    std::unique_lock<std::mutex> locker(m_mu);
    if(m_register_map.find(key) == m_register_map.end()){
        return false;
    }

    std::function<void(std::string, void*)> destroy_obj_func = m_register_map[key].second;
    void* obj = m_register_map[key].first;
    destroy_obj_func(key, obj);
    m_register_map.erase(key);
    m_register_start_time.erase(key);
    m_register_info.erase(key);
    return true;
}

bool RegisterCenter::destroyObjWithPrefix(std::string prefix){
    std::unique_lock<std::mutex> locker(m_mu);
    std::map<std::string, std::pair<void*, std::function<void(std::string, void*)>>>::iterator iter, iend(m_register_map.end());
    for(iter = m_register_map.begin(); iter != iend; ++iter){
        if(startswith(iter->first, prefix)){
            std::function<void(std::string, void*)> destroy_obj_func = m_register_map[iter->first].second;
            void* obj = m_register_map[iter->first].first;
            destroy_obj_func(iter->first, obj);
            m_register_map.erase(iter->first);
            m_register_start_time.erase(iter->first);
            m_register_info.erase(iter->first);
        }
    }

    return true;
}

void RegisterCenter::enableTimeout(int timeout_seconds){
    m_run_check = true;
    m_future = std::async(std::launch::async, &RegisterCenter::startTimerThread, this, timeout_seconds);
}

void RegisterCenter::startTimerThread(int timeout_seconds){
    while(m_run_check == true){
        std::this_thread::sleep_for(60s);
        std::vector<std::string> keys_to_destroy;
        {
            std::lock_guard<std::mutex> lock(m_mu);
            for(auto &[key, destroy_func] : m_register_map){
                if(m_register_start_time.count(key) == 0){
                    EAGLEEYE_LOGE("RegisterCenter found error key = [%s], it has no start time", key.c_str());
                    keys_to_destroy.emplace_back(key);
                }
                if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - m_register_start_time[key]).count()> timeout_seconds) {
                    keys_to_destroy.emplace_back(key);
                } 
            }
        }
        for(const auto& key: keys_to_destroy){
            EAGLEEYE_LOGI("RegisterCenter found timeout key, now destroy key = [%s]", key.c_str());
            if(not destroyObj(key)){
                EAGLEEYE_LOGE("RegisterCenter destroy key = [%s] failed", key.c_str());
            }
        }
    }
}
}