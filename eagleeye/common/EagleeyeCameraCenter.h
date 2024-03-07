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
#include "eagleeye/processnode/RTSPReadNode.h"

namespace eagleeye{
class CameraCenter{
public:
    virtual ~CameraCenter();
    static CameraCenter* getInstance();

    bool isExist(std::string camera_address);
    bool activeCamera(std::string camera_address, int pixel_format=1);
    bool addCamera(std::string camera_address, int pixel_format=1);
    bool removeCamera(std::string camera_address);

    AnyNode* getCamera(std::string camera_address);

private:
    CameraCenter();

    std::map<std::string, std::pair<AnyNode*, int>> m_camera_map;
    std::mutex m_mu;

    static std::shared_ptr<CameraCenter> m_instance;
};
}
#endif