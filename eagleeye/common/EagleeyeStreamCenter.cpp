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

AnyNode* StreamCenter::createStream(std::string name, int queue_size, std::vector<std::string> input_types, std::vector<std::string> input_categorys){
    std::unique_lock<std::mutex> locker(m_mu);
    if(m_stream_info.find(name) != m_stream_info.end()){
        return m_stream_info[name];
    }

    PlaceholderQueue* data_node = new PlaceholderQueue(queue_size);
    for(int input_i=0; input_i<input_types.size(); ++input_i){
        data_node->config(input_i, input_types[input_i], input_categorys[input_i]);
    }

    m_stream_info[name] = data_node;
    return m_stream_info[name];
}

AnyNode* StreamCenter::createVideoStream(std::string name, int queue_size, std::string encode_mode){
    std::unique_lock<std::mutex> locker(m_mu);
    if(m_stream_info.find(name) != m_stream_info.end()){
        return m_stream_info[name];
    }

    m_stream_info[name] = new VideoStreamNode(queue_size);
    return m_stream_info[name];
}

AnyNode* StreamCenter::getStream(std::string name){
    std::unique_lock<std::mutex> locker(m_mu);
    if(m_stream_info.find(name) != m_stream_info.end()){
        return m_stream_info[name];
    }

    return NULL;
}

bool StreamCenter::removeStream(std::string name){
    std::unique_lock<std::mutex> locker(m_mu);
    if(m_stream_info.find(name) != m_stream_info.end()){
        delete m_stream_info[name];
        m_stream_info.erase(name);
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