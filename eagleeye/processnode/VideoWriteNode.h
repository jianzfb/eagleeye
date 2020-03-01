#ifndef _EAGLEEYE_VIDEOWRITENODE_H_
#define _EAGLEEYE_VIDEOWRITENODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/processnode/ImageIONode.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/MetaOperation.h"

class AVFormatContext;
class AVCodecContext;
class AVFrame;
class AVStream;
class AVPacket;
class AVFrame;
namespace eagleeye{
class VideoWriteNode:public ImageIONode<ImageSignal<Array<unsigned char, 3>>>{
public:
    typedef VideoWriteNode                                           Self;
    typedef ImageIONode<ImageSignal<Array<unsigned char, 3>>>       Superclass;

    /**
	 *	@brief get class identity	
	 */	 
	EAGLEEYE_CLASSIDENTITY(VideoWriteNode);

    /**
     * @brief Construct a new Video Read Node object
     * 
     */
    VideoWriteNode();
    /**
     * @brief Destroy the Video Read Node object
     * 
     */
    virtual ~VideoWriteNode();

	/**
	 *	@brief parse video data
	 */
	virtual void executeNodeInfo();

    /**
     * @brief Set/Get the File Path object
     * 
     * @param file_path 
     */
    virtual void setFilePath(std::string file_path);
    virtual void getFilePath(std::string& file_path){};

private:
    VideoWriteNode(const VideoWriteNode&);
    void operator=(const VideoWriteNode&);

    /**
     * @brief finish writing process
     * 
     */
    void finish();

    /**
     * @brief flush encoder
     * 
     * @param fmt_ctx 
     * @param stream_index 
     * @return int 
     */
    int flush_encoder(AVFormatContext *fmt_ctx, unsigned int stream_index);

    bool m_is_init;
    bool m_is_finish;
    int m_fps;
    AVFormatContext* m_output_cxt;
    AVCodecContext* m_codec_cxt;
    AVStream* stream;
    AVPacket* packet;
    AVFrame* frame;
    uint8_t *pointers[4];
    int linesizes[4];
};
}
#endif
