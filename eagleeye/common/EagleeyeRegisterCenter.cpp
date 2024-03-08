#include "eagleeye/common/EagleeyeRegisterCenter.h"
#include "eagleeye/common/EagleeyeStr.h"
namespace eagleeye{
std::shared_ptr<RegisterCenter> RegisterCenter::m_instance(new RegisterCenter(), [](RegisterCenter* d) { delete d; });    
RegisterCenter::RegisterCenter(){}
RegisterCenter::~RegisterCenter(){}
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

bool RegisterCenter::registerObj(std::string key, void* obj, std::function<void(std::string, void*)> destroy_func){
    std::unique_lock<std::mutex> locker(m_mu);
    if(m_register_map.find(key) != m_register_map.end()){
        return false;
    }

    m_register_map[key] = std::make_pair(obj, destroy_func);
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
        }
    }

    return true;
}
}