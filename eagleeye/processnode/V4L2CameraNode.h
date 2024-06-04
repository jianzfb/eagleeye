#ifndef _EAGLEEYE_V4L2_CAMERA_NODE_H_
#define _EAGLEEYE_V4L2_CAMERA_NODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"
#include <linux/videodev2.h>
#include <list>
#include <mutex>


namespace eagleeye{
class V4L2CameraNode:public AnyNode, DynamicNodeCreator<V4L2CameraNode>{
public:
    typedef V4L2CameraNode       Self;
    typedef AnyNode             Superclass;

    EAGLEEYE_CLASSIDENTITY(V4L2CameraNode);

    V4L2CameraNode();
    virtual ~V4L2CameraNode();

	/**
	 *	@brief parse data
	 */
	virtual void executeNodeInfo();
    virtual void processUnitInfo();

    void setCameraId(std::string camera_id);
    void getCameraId(std::string& camera_id);

    void setImageFormat(int image_format);
    void setImageRotation(int image_rotation);

    void setPreviewSize(int width, int height);
    bool isPullError(){return m_is_pull_error;};

private:
    V4L2CameraNode(const V4L2CameraNode&);
    void operator=(const V4L2CameraNode&);

    int cameraOpen(const char *device, unsigned int width, unsigned int height, unsigned int pixelformat);
    int cameraInit();
    void cameraUninit();
    void cameraClose();

    void startCameraStreaming();
    void stopCameraStreaming();
    int grabCameraRawFrame(void *raw_base);

    bool isSupportPreviewSize(unsigned int& width, unsigned int& height, unsigned int pixelformat);

    std::string m_camera_id;
    bool m_is_pull_error;
    bool m_is_camera_open;
    bool m_is_init_error;

    int m_output_image_format;
    int m_output_image_rotation;

    int width;
    int height;
    int pixelformat;

    int fd;
    int start;
    unsigned char *mem;
    struct v4l2_buffer buf;

    unsigned char* raw;         // 缓存来自相机的原始数据
    int raw_size;               // 缓存大小

    int m_preview_width;
    int m_preview_height;
}; 
}
#endif