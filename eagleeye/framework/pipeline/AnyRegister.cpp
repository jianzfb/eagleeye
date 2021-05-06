#include "eagleeye/framework/pipeline/AnyRegister.h"
#include "eagleeye/common/EagleeyeLog.h"

namespace eagleeye
{
std::shared_ptr<AnyRegister> AnyRegister::m_register;    
std::shared_ptr<AnyRegister> AnyRegister::get(){
    if(!m_register.get()){
        m_register = 
            std::shared_ptr<AnyRegister>(new AnyRegister(), 
                            [](AnyRegister* ptr){delete ptr;});

    }
    return m_register;
}    

AnyRegister::AnyRegister(){

}

AnyRegister::~AnyRegister(){
    std::map<std::string, AnySignal*>::iterator iter, iend(m_register_map.end());
    for(iter=m_register_map.begin(); iter != iend; ++iter){
        delete iter->second;
    }
}

bool AnyRegister::registerSignal(std::string name, AnySignal* sig){
    if(sig == NULL){
        EAGLEEYE_LOGE("Register signal %s is NULL.", name.c_str());
        return false;
    }

    if(m_register_map.find(name) != m_register_map.end()){
        EAGLEEYE_LOGD("Signal %s has existed in register map.", name.c_str());
    }

    AnySignal* sig_cp = sig->make();
    sig_cp->copy(sig);
    m_register_map[name] = sig_cp;
    return true;
}

AnySignal* AnyRegister::signal(std::string name){
    if(m_register_map.find(name) == m_register_map.end()){
        EAGLEEYE_LOGE("Register signal %s dont exist.", name.c_str());
        return NULL;
    }

    return m_register_map[name];
}

bool AnyRegister::clear(std::string name){
    if(name == ""){
        m_register_map.clear();
        return true;
    }

    if(m_register_map.find(name) == m_register_map.end()){
        EAGLEEYE_LOGE("Register signal %s dont exist.", name.c_str());
        return false;
    }

    m_register_map.erase(m_register_map.find(name));
    return true;
}
} // namespace eagleeye
