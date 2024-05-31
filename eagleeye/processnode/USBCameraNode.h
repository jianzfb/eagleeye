#ifndef _EAGLEEYE_USB_CAMERA_NODE_H_
#define _EAGLEEYE_USB_CAMERA_NODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"


namespace eagleeye{
class USBCameraNode:public AnyNode, DynamicNodeCreator<USBCameraNode>{
public:
    typedef USBCameraNode       Self;
    typedef AnyNode             Superclass;

    EAGLEEYE_CLASSIDENTITY(USBCameraNode);

    USBCameraNode();
    virtual ~USBCameraNode();

	/**
	 *	@brief parse data
	 */
	virtual void executeNodeInfo();
    virtual void processUnitInfo();

    void setCameraId(std::string camera_id);
    void getCameraId(std::string& camera_id);

    void setImageFormat(int image_format);
    void getImageFormat(int& image_format);

    bool isPullError(){return m_is_pull_error;};

private:
    std::string m_camera_id;
    int m_image_format;
    bool m_is_pull_error;
    bool m_is_camera_open;
}; 
}
#endif