#ifndef _EAGLEEYE_RTSPREAD_NODE_H_
#define _EAGLEEYE_RTSPREAD_NODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/processnode/ImageIONode.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/MetaOperation.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"

class AVCodecContext;
class AVCodec;
class AVFrame;
class AVFormatContext;
namespace eagleeye{
class RTSPReadNode:public ImageIONode<ImageSignal<Array<unsigned char, 3>>>, DynamicNodeCreator<RTSPReadNode>{
public:
    typedef RTSPReadNode Self;
    typedef ImageIONode<ImageSignal<Array<unsigned char, 3>>>       Superclass;

    /**
	 *	@brief get class identity	
	 */	 
	EAGLEEYE_CLASSIDENTITY(RTSPReadNode);

    RTSPReadNode();
    virtual ~RTSPReadNode();

	/**
	 *	@brief parse video data
	 */
	virtual void executeNodeInfo();
    virtual void processUnitInfo();

    virtual void setFilePath(std::string file_path);
    virtual void getFilePath(std::string& file_path);
    
    void setImageFormat(int image_format);
    void setImageRotation(int image_rotation);

    void setRTSPTransport(std::string rtsp_transport);
    void setOvertime(std::string overtime);

    void setMaxQueue(int size);

private:
    RTSPReadNode(const RTSPReadNode&);
    void operator=(const RTSPReadNode&);

    void postprocess_by_libyuv();   // CPU处理（libyuv）
    void postprocess_by_rga();      // RK芯片，独立图像处理器

    std::string m_rtsp_address;
    std::string m_rtsp_transport;
    std::string m_overtime;

    AVCodecContext* m_pCodecCtx;
    const AVCodec* m_pCodec;
    AVFormatContext* m_format_ctx;

    int m_video_stream_index;
    int m_output_image_format;
    int m_output_image_rotation;

    void* m_mpp_ctx;
    void* m_mpp_api;

    std::mutex m_out_mu;
	std::condition_variable m_out_cond;
    std::queue<std::pair<Matrix<Array<unsigned char,3>>, __int64_t>> m_out_queue;

    std::mutex m_postprocess_mu;
	std::condition_variable m_postprocess_cond;
    std::queue<AVFrame*> m_postprocess_queue;
    std::queue<void*> m_postprocess_mpp_queue;
    std::thread m_thread;
    bool m_exit_flag;

    int m_max_queue_size;
};
}
#endif