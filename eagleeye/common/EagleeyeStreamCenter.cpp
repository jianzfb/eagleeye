#include "eagleeye/common/EagleeyeStreamCenter.h"
namespace eagleeye{
std::shared_ptr<StreamCenter> StreamCenter::m_instance(new StreamCenter(), [](StreamCenter* d) { delete d; });

StreamCenter::StreamCenter(){

}
StreamCenter::~StreamCenter(){

}

StreamCenter* StreamCenter::getInstance(){
    return m_instance.get();
}

AnyNode* StreamCenter::createStream(std::string name, int queue_size){
    std::unique_lock<std::mutex> locker(m_mu);
    if(m_stream_info.find(name) != m_stream_info.end()){
        return m_stream_info[name];
    }

    m_stream_info[name] = new VideoStreamNode(queue_size);
    return m_stream_info[name];
}

bool StreamCenter::removeStream(std::string name){
    std::unique_lock<std::mutex> locker(m_mu);
    if(m_stream_info.find(name) != m_stream_info.end()){
        delete m_stream_info[name];
    }

    return true;
}

bool StreamCenter::decoce(std::string name, uint8_t* package_data, int package_size){
    std::unique_lock<std::mutex> locker(m_mu);
    if(m_stream_info.find(name) == m_stream_info.end()){
        return false;
    }

    VideoStreamNode* vsn = (VideoStreamNode*)(m_stream_info[name]);
    vsn->decode(package_data, package_size);
    return true;
}
}