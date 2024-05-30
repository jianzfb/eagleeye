#ifndef _EAGLEEYE_ANDROID_CAMERA_NODE_H_
#define _EAGLEEYE_ANDROID_CAMERA_NODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"


#if defined(__ANDROID__) || defined(ANDROID)
class ACameraManager;
namespace eagleeye{
class AndroidCameraNode:public AnyNode, DynamicNodeCreator<AndroidCameraNode>{
public:
    typedef AndroidCameraNode           Self;
    typedef AnyNode                     Superclass;

    /**
	 *	@brief get class identity	
	 */	 
	EAGLEEYE_CLASSIDENTITY(AndroidCameraNode);

    AndroidCameraNode();
    virtual ~AndroidCameraNode();

	/**
	 *	@brief parse video data
	 */
	virtual void executeNodeInfo();
    virtual void processUnitInfo();
    
    /**
     *  @brief 
     */
    void setCameraFacing(std::string facing);
    void getCameraFacing(std::string& facing);

    void setCameraID(std::string camera_id);
    void getCameraID(std::string& camera_id);

    void setImageFormat(int image_format);
    void getImageFormat(int& image_format);

    bool isPullError(){return m_is_pull_error;};

private:
    AndroidCameraNode(const AndroidCameraNode&);
    void operator=(const AndroidCameraNode&);

    bool m_is_camera_open;
    std::string m_camera_facing;
    double m_timestamp;
    std::string m_camera_id;
    int m_image_format;

    bool m_is_pull_error;
};
}

#endif
#endif