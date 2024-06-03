#ifndef _EAGLEEYE_USB_CAMERA_NODE_H_
#define _EAGLEEYE_USB_CAMERA_NODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/processnode/ImageIONode.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/basic/MetaOperation.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"

class AVCodecContext;
class AVCodec;
class AVFrame;
class AVFormatContext;
class AVBufferRef;
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
    void setImageRotation(int image_rotation);
    bool isPullError(){return m_is_pull_error;};

private:
    USBCameraNode(const USBCameraNode&);
    void operator=(const USBCameraNode&);

    void postprocess_by_rga();      // RK芯片，独立图像处理器
    std::string m_camera_id;
    int m_camera_index;
    int m_image_format;
    bool m_is_pull_error;
    bool m_is_camera_open;

    bool m_is_init_error;

    AVCodecContext* m_pCodecCtx;
    const AVCodec* m_pCodec;
    AVFormatContext* m_format_ctx;

    int m_video_stream_index;
    int m_output_image_format;
    int m_output_image_rotation;

    void* m_mpp_ctx;
    void* m_mpp_api;
    void* m_frm_grp;

    std::mutex m_out_mu;
	std::condition_variable m_out_cond;
    std::queue<std::pair<unsigned char*, __int64_t>> m_out_queue;

    std::mutex m_postprocess_mu;
	std::condition_variable m_postprocess_cond;
    std::queue<std::pair<AVFrame*, __int64_t>> m_postprocess_queue;

    std::queue<void*> m_postprocess_mpp_queue;
    std::thread m_thread;
    bool m_exit_flag;
    int64_t m_image_h;              // image h
    int64_t m_image_w;              // image w
    int m_max_queue_size;
}; 
}
#endif