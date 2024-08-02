#ifndef _EAGLEEYE_MESSAGE_H_
#define _EAGLEEYE_MESSAGE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/CJsonObject.hpp"
#include <string>
#include <queue>
#include <functional> 
#include <memory>

namespace eagleeye{
class Message{
public:
    friend class MessageCmp;
    Message(double timestamp=0, bool status=true, bool is_timeliness=false){
        this->m_timestamp = timestamp;
        this->m_status = status;
        this->m_is_timeliness = is_timeliness;
        m_json_obj = new neb::CJsonObject();
    }
    virtual ~Message(){
        delete m_json_obj;
    }

    class Null{}; 
    std::string serialize(){
        return m_json_obj->ToFormattedString();
    }
    template<typename T>
    void set(std::string key, T value){
        m_json_obj->Add(key, value);
    }

    void set(std::string key, Null val=Null()){
        m_json_obj->AddNull(key);
    }

    void set(std::string key, std::vector<int> value){
        if(value.size() > 0){
            neb::CJsonObject obj;
            for(int i=0; i<value.size(); ++i){
                obj.Add(value[i]);
            }

            m_json_obj->Add(key, obj);          
            return;  
        }

        m_json_obj->AddEmptySubArray(key);
    }

    void set(std::string key, std::vector<float> value){
        if(value.size() > 0){
            neb::CJsonObject obj;
            for(int i=0; i<value.size(); ++i){
                obj.Add(value[i]);
            }

            m_json_obj->Add(key, obj);
            return;
        }

        m_json_obj->AddEmptySubArray(key);
    }

    void set(std::string key, std::vector<double> value){
        if(value.size() > 0){
            neb::CJsonObject obj;
            for(int i=0; i<value.size(); ++i){
                obj.Add(value[i]);
            }

            m_json_obj->Add(key, obj);
            return;
        }

        m_json_obj->AddEmptySubArray(key);
    }    

    void set(std::string key, std::vector<std::string> value){
        if(value.size() > 0){
            neb::CJsonObject obj;
            for(int i=0; i<value.size(); ++i){
                obj.Add(value[i]);
            }

            m_json_obj->Add(key, obj);
            return;
        }

        m_json_obj->AddEmptySubArray(key);
    }

    void copy(std::string info){
        if(info == ""){
            return;
        }
        m_json_obj->Parse(info);
    }

    template<typename T>
    bool get(std::string key, T& value){
        if(m_json_obj->IsNull(key)){
            return false;
        }
        m_json_obj->Get(key, value);
        return true;
    }

    void setTimestamp(double timestamp){
        m_timestamp = timestamp;
    }
    double getTimestamp(){
        return m_timestamp;
    }

    void setStatus(bool status){
        m_status = status;
    }
    bool getStatus(){
        return m_status;
    }

private:
    double m_timestamp;
    bool m_status;
    neb::CJsonObject* m_json_obj;
    bool m_is_timeliness;
}; 

class MessageCmp {
public:
    bool operator()(const std::shared_ptr<Message>& lhs, const std::shared_ptr<Message>& rhs) const{
        if(lhs->m_is_timeliness){
            // 新消息排序在前
            return lhs->m_timestamp < rhs->m_timestamp;
        }
        else{
            // 老消息排序在前
            return lhs->m_timestamp > rhs->m_timestamp;
        }
    }
};
}
#endif