#include "eagleeye/common/EagleeyeCameraCenter.h"
#include "eagleeye/processnode/AndroidCameraNode.h"
#include "eagleeye/processnode/RTSPReadNode.h"
#include "eagleeye/processnode/USBCameraNode.h"
#include "eagleeye/processnode/VideoReadNode.h"
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

bool CameraCenter::activeCamera(std::string camera_address, int pixel_format, CameraType camera_type){
#ifndef EAGLEEYE_FFMPEG
    return false;
#else
    if(camera_type != CAMERA_NETWORK && 
        camera_type != CAMERA_USB &&
        camera_type != CAMERA_ANDROID_NATIVE &&
        camera_type != CAMERA_VIDEO){
        EAGLEEYE_LOGE("Camera center only support network camera, usb camera, and video");
        return false;
    }

    std::unique_lock<std::mutex> locker(m_mu);
    if(m_camera_map.find(camera_address) != m_camera_map.end()){
        return true;
    }

    AutoNode* camera_source = new AutoNode(
        [&](){
            if(camera_type == CAMERA_NETWORK){
                RTSPReadNode* rts_node = new RTSPReadNode(); 
                rts_node->setFilePath(camera_address);
                rts_node->setImageFormat(pixel_format);     // 0: rgb, 1: bgr, 2: rgba, 3: bgra
                return (AnyNode*)rts_node;
            }
            else if(camera_type == CAMERA_USB){
                USBCameraNode* camera_node = new USBCameraNode(); 
                camera_node->setCameraId(camera_address);
                camera_node->setImageFormat(pixel_format);     // 0: rgb, 1: bgr, 2: rgba, 3: bgra
                return (AnyNode*)camera_node;
            }
            else if(camera_type == CAMERA_ANDROID_NATIVE){
#if defined(__ANDROID__) || defined(ANDROID)
                AndroidCameraNode* camera_node = new AndroidCameraNode();
                camera_node->setCameraFacing(camera_address);
                camera_node->setImageFormat(pixel_format);
                return (AnyNode*)camera_node;
#else
                AnyNode* camera_node = NULL;
                return camera_node;
#endif 
            }
            else if(camera_type == CAMERA_VIDEO){
                VideoReadNode* video_node = new VideoReadNode();
                video_node->setFilePath(camera_address);
                video_node->setImageFormat(pixel_format);
                return (AnyNode*)video_node;
            }
        },
        1,
        false
    );
    camera_source->setPersistent(true);
    camera_source->init();

    if(camera_type == CAMERA_NETWORK){
        RTSPReadNode* inner_rts_node = (RTSPReadNode*)(camera_source->getInnerIns());
        if(inner_rts_node->isPullError()){
            // 错误，删除对象
            camera_source->setPersistent(false);
            camera_source->exit();
            delete camera_source;
            return false;
        }
    }
    else if(camera_type == CAMERA_ANDROID_NATIVE){
#if defined(__ANDROID__) || defined(ANDROID)        
        AndroidCameraNode* inner_rts_node = (AndroidCameraNode*)(camera_source->getInnerIns());
        if(inner_rts_node->isPullError()){
            // 错误，删除对象
            camera_source->setPersistent(false);
            camera_source->exit();
            delete camera_source;
            return false;
        }
#else
        return false;
#endif
    }
    else if(camera_type == CAMERA_VIDEO){
        VideoReadNode* inner_rts_node = (VideoReadNode*)(camera_source->getInnerIns());
        if(inner_rts_node->isPullError()){
            // 错误，删除对象
            camera_source->setPersistent(false);
            camera_source->exit();
            delete camera_source;
            return false;
        }
    }

    // 激活相机，不计入占用计数
    m_camera_map[camera_address] = std::make_tuple((AnyNode*)camera_source, 0, camera_type);
    return true;
#endif
}

bool CameraCenter::addCamera(std::string camera_address, int pixel_format, CameraType camera_type){
#ifndef EAGLEEYE_FFMPEG
    return false;
#else
    std::unique_lock<std::mutex> locker(m_mu);
    if(m_camera_map.find(camera_address) != m_camera_map.end()){
        // 占用计数 +1
        std::get<1>(m_camera_map[camera_address]) += 1;
        EAGLEEYE_LOGD("Camera %s use +1 (now %d)", camera_address.c_str(), std::get<1>(m_camera_map[camera_address]));
        return true;
    }

    AutoNode* camera_source = new AutoNode(
        [&](){
            if(camera_type == CAMERA_NETWORK){
                RTSPReadNode* rts_node = new RTSPReadNode(); 
                rts_node->setFilePath(camera_address);
                rts_node->setImageFormat(pixel_format);     // 0: rgb, 1: bgr, 2: rgba, 3: bgra
                return (AnyNode*)rts_node;
            }
            else if(camera_type == CAMERA_USB){
                USBCameraNode* camera_node = new USBCameraNode(); 
                camera_node->setCameraId(camera_address);
                camera_node->setImageFormat(pixel_format);     // 0: rgb, 1: bgr, 2: rgba, 3: bgra
                return (AnyNode*)camera_node;
            }
            else if(camera_type == CAMERA_ANDROID_NATIVE){
#if defined(__ANDROID__) || defined(ANDROID)
                AndroidCameraNode* camera_node = new AndroidCameraNode();
                camera_node->setCameraFacing(camera_address);
                camera_node->setImageFormat(pixel_format);
                return (AnyNode*)camera_node;
#else
                AnyNode* camera_node = NULL;
                return camera_node;
#endif
            }
            else if(camera_type == CAMERA_VIDEO){
                VideoReadNode* video_node = new VideoReadNode();
                video_node->setFilePath(camera_address);
                video_node->setImageFormat(pixel_format);
                return (AnyNode*)video_node;
            }
        },
        1,
        false
    );
    camera_source->setPersistent(true);
    camera_source->init();

    if(camera_type == CAMERA_NETWORK){
        RTSPReadNode* inner_rts_node = (RTSPReadNode*)(camera_source->getInnerIns());
        if(inner_rts_node->isPullError()){
            // 错误，删除对象
            camera_source->setPersistent(false);
            camera_source->exit();
            delete camera_source;
            return false;
        }
    }
    else if(camera_type == CAMERA_ANDROID_NATIVE){
#if defined(__ANDROID__) || defined(ANDROID)
        AndroidCameraNode* inner_rts_node = (AndroidCameraNode*)(camera_source->getInnerIns());
        if(inner_rts_node->isPullError()){
            // 错误，删除对象
            camera_source->setPersistent(false);
            camera_source->exit();
            delete camera_source;
            return false;
        }
#else
        return false;
#endif
    }
    else if(camera_type == CAMERA_VIDEO){
        VideoReadNode* inner_rts_node = (VideoReadNode*)(camera_source->getInnerIns());
        if(inner_rts_node->isPullError()){
            // 错误，删除对象
            camera_source->setPersistent(false);
            camera_source->exit();
            delete camera_source;
            return false;
        }
    }

    // 占用计数 +1
    m_camera_map[camera_address] = std::make_tuple((AnyNode*)camera_source, 1, camera_type);
    EAGLEEYE_LOGD("Camera %s use +1 (now %d)", camera_address.c_str(), 1);
    return true;
#endif
}

bool CameraCenter::removeCamera(std::string camera_address){
    std::unique_lock<std::mutex> locker(m_mu);
    if(m_camera_map.find(camera_address) == m_camera_map.end()){
        EAGLEEYE_LOGE("Camera %s not existed.", camera_address.c_str());
        return false;
    }

    std::get<1>(m_camera_map[camera_address]) -= 1;
    if(std::get<1>(m_camera_map[camera_address]) <= 0){
        AutoNode* camera_node = (AutoNode*)(std::get<0>(m_camera_map[camera_address]));
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

    return std::get<0>(m_camera_map[camera_address]);
}
}