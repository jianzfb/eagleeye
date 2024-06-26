#ifndef _EAGLEEYE_VIDEOREADNODE_H_
#define _EAGLEEYE_VIDEOREADNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/processnode/ImageIONode.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/MetaOperation.h"
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"


class AVFormatContext;
class AVCodecContext;
namespace eagleeye{
class VideoReadNode:public ImageIONode<ImageSignal<Array<unsigned char, 3>>>, DynamicNodeCreator<VideoReadNode>{
public:
    typedef VideoReadNode                                           Self;
    typedef ImageIONode<ImageSignal<Array<unsigned char, 3>>>       Superclass;

    /**
	 *	@brief get class identity	
	 */	 
	EAGLEEYE_CLASSIDENTITY(VideoReadNode);

    /**
     * @brief Construct a new Video Read Node object
     * 
     */
    VideoReadNode();
    /**
     * @brief Destroy the Video Read Node object
     * 
     */
    virtual ~VideoReadNode();

    /**
	 *	@brief define output port image signal type
	 */
	// EAGLEEYE_OUTPUT_PORT_TYPE(Array<unsigned char, 3>,	0,	IMAGE_DATA);

	/**
	 *	@brief parse video data
	 */
	virtual void executeNodeInfo();

    virtual bool finish();

    /**
     * @brief Set/Get the File Path object
     * 
     * @param file_path 
     */
    virtual void setFilePath(std::string file_path);
    virtual void getFilePath(std::string& file_path);

    /**
     * @brief Set/Get the Frames Number object
     * 
     * @param num 
     */
    void setFramesNumber(int num);
    void getFramesNumber(int& num);

    void setImageFormat(int image_format);
    void getImageFormat(int& image_format);

    bool isPullError(){return m_is_pull_error;};

private:
    VideoReadNode(const VideoReadNode&);
    void operator=(const VideoReadNode&);
    
    int m_frame_count;
    int m_frame_total;
    AVFormatContext* m_avf_cxt;
    AVCodecContext* m_avc_cxt;

    Matrix<Array<unsigned char, 3>> m_next;
    Matrix<Array<unsigned char, 3>> m_nextnext;

    MetaData m_next_meta;
    MetaData m_nextnext_meta;

    int m_stream_index;
    double m_video_fps;
    bool m_decoder_finish;
    bool m_first_call;
    int m_rotate_degree;

    int m_image_format;
    bool m_is_pull_error;
}; 
}
#endif