#include "eagleeye/common/EagleeyeCameraCenter.h"
#include "eagleeye/common/EagleeyeLog.h"


namespace eagleeye{
std::shared_ptr<CameraCenter> CameraCenter::m_instance(new CameraCenter(), [](CameraCenter* d) { delete d; });
CameraCenter::CameraCenter(){}
CameraCenter::~CameraCenter(){}

CameraCenter* CameraCenter::getInstance(){
    return m_instance.get();
}

bool CameraCenter::isExist(std::string camera_address){
    std::unique_lock<std::mutex> locker(m_mu);
    if(m_camera_map.find(camera_address) == m_camera_map.end()){
        EAGLEEYE_LOGE("Camera %s not existed.", camera_address.c_str());
        return false;
    }

    return true;
}

bool CameraCenter::activeCamera(std::string camera_address, int pixel_format){
    std::unique_lock<std::mutex> locker(m_mu);
    if(m_camera_map.find(camera_address) != m_camera_map.end()){
        return true;
    }

    AutoNode* camera_source = new AutoNode(
        [&](){
            RTSPReadNode* rts_node = new RTSPReadNode(); 
            rts_node->setFilePath(camera_address);
            rts_node->setImageFormat(pixel_format);     // 0: rgb, 1: bgr, 2: rgba, 3: bgra
            return rts_node;
        },
        1,
        false
    );
    camera_source->setPersistent(true);
    camera_source->init();

    RTSPReadNode* inner_rts_node = (RTSPReadNode*)(camera_source->getInnerIns());
    if(inner_rts_node->isRTSPStreamPullError()){
        // 错误，删除对象
        camera_source->setPersistent(false);
        camera_source->exit();
        delete camera_source;
        return false;
    }

    // 激活相机，不计入占用计数
    m_camera_map[camera_address] = std::make_pair((AnyNode*)camera_source, 0);
    return true;
}

bool CameraCenter::addCamera(std::string camera_address, int pixel_format){
    std::unique_lock<std::mutex> locker(m_mu);
    if(m_camera_map.find(camera_address) != m_camera_map.end()){
        // 占用计数 +1
        m_camera_map[camera_address].second += 1;
        EAGLEEYE_LOGD("Camera %s use +1 (now %d)", camera_address.c_str(), m_camera_map[camera_address].second);
        return true;
    }

    AutoNode* camera_source = new AutoNode(
        [&](){
            RTSPReadNode* rts_node = new RTSPReadNode(); 
            rts_node->setFilePath(camera_address);
            rts_node->setImageFormat(pixel_format);     // 0: rgb, 1: bgr, 2: rgba, 3: bgra
            return rts_node;
        },
        1,
        false
    );
    camera_source->setPersistent(true);
    camera_source->init();

    RTSPReadNode* inner_rts_node = (RTSPReadNode*)(camera_source->getInnerIns());
    if(inner_rts_node->isRTSPStreamPullError()){
        // 错误，删除对象
        camera_source->setPersistent(false);
        camera_source->exit();
        delete camera_source;
        return false;
    }

    // 占用计数 +1
    m_camera_map[camera_address] = std::make_pair((AnyNode*)camera_source, 1);
    EAGLEEYE_LOGD("Camera %s use +1 (now %d)", camera_address.c_str(), 1);
    return true;
}

bool CameraCenter::removeCamera(std::string camera_address){
    std::unique_lock<std::mutex> locker(m_mu);
    if(m_camera_map.find(camera_address) == m_camera_map.end()){
        EAGLEEYE_LOGE("Camera %s not existed.", camera_address.c_str());
        return false;
    }

    m_camera_map[camera_address].second -= 1;
    if(m_camera_map[camera_address].second <= 0){
        AutoNode* camera_node = (AutoNode*)(m_camera_map[camera_address].first);
        camera_node->setPersistent(false);
        camera_node->exit();
        delete camera_node;
        m_camera_map.erase(camera_address);
    }
    return true;
}


AnyNode* CameraCenter::getCamera(std::string camera_address){
    std::unique_lock<std::mutex> locker(m_mu);
    if(m_camera_map.find(camera_address) == m_camera_map.end()){
        EAGLEEYE_LOGE("Camera %s not existed.", camera_address.c_str());
        return NULL;
    }

    return m_camera_map[camera_address].first;
}


}