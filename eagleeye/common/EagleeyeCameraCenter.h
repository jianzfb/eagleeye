#ifndef _EAGLEEYE_CAMERA_CENTER_H_
#define _EAGLEEYE_CAMERA_CENTER_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/EagleeyeMessage.h"
#include <map>
#include <functional> 
#include <memory>
#include <mutex>
#include "eagleeye/processnode/AutoNode.h"

namespace eagleeye{
enum CameraType{
    CAMERA_NETWORK = 0,
    CAMERA_USB,
    CAMERA_ANDROID_NATIVE,
    CAMERA_VIDEO,
};

class CameraCenter{
public:
    virtual ~CameraCenter();
    static CameraCenter* getInstance();

    bool isExist(std::string camera_address);
    bool activeCamera(std::string camera_address, int pixel_format=1, CameraType camera_type=CAMERA_NETWORK);
    bool addCamera(std::string camera_address, int pixel_format=1, CameraType camera_type=CAMERA_NETWORK);
    bool removeCamera(std::string camera_address);

    AnyNode* getCamera(std::string camera_address);

private:
    CameraCenter();

    std::map<std::string, std::tuple<AnyNode*, int, CameraType>> m_camera_map;
    std::mutex m_mu;

    static std::shared_ptr<CameraCenter> m_instance;

};
}
#endif