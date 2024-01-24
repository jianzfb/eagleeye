#ifndef _EAGLEEYE_VIDEOWRITENODE_H_
#define _EAGLEEYE_VIDEOWRITENODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/processnode/ImageIONode.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/basic/MetaOperation.h"
#include <string>
#include <fstream>
#include <iostream>
#include "eagleeye/framework/pipeline/DynamicNodeCreater.h"


class AVCodec;
class AVCodecContext;
class AVFrame;
class AVPacket;
namespace eagleeye{
class VideoWriteNode:public ImageIONode<ImageSignal<Array<unsigned char, 3>>>, DynamicNodeCreator<VideoWriteNode>{
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
    virtual void getFilePath(std::string& file_path);

    /**
     * @brief Set/Get the Prefix object
     * 
     * @param prefix 
     */
    virtual void setPrefix(std::string prefix);
    virtual void getPrefix(std::string& prefix);

    /**
     * @brief Set/Get the Prefix object
     * 
     * @param prefix 
     */
    virtual void setFolder(std::string folder);
    virtual void getFolder(std::string& folder);

    /**
     * @brief Set/Get the Force Stop object
     * 
     * @param stop 
     */
    void setStop(int stop);
    void getStop(int& stop);

    void setImageFormat(int image_format);

    /**
     * @brief Set/Get the Force Start object
     * 
     * @param start 
     */
    void setStart(int start);
    void getStart(int& start);

    void setFPS(int fps);
    void getFPS(int& fps);

private:
    VideoWriteNode(const VideoWriteNode&);
    void operator=(const VideoWriteNode&);

    /**
     * @brief finish writing process
     * 
     */
    void writeFinish();

    bool m_is_init;
    bool m_is_finish;
    int m_fps;

    std::string m_prefix;
    std::string m_folder;

    int m_manually_stop;
    int m_manually_start;

    std::ofstream m_output_file;
    AVCodecContext* m_codec_cxt;
    const AVCodec* m_encoder;
    AVFrame *m_frame;
    AVPacket *m_pkt;
    int m_frame_count;

    Matrix<unsigned char> m_temp;

    Matrix<Array<unsigned char, 3>> m_c3_image;
    Matrix<Array<unsigned char, 4>> m_c4_image;
    int m_frame_size;
    int m_header_size;
    int m_image_format;

    void* m_pkt_buf;
    void* m_frame_buf;
    void* m_md_info;
    void* m_buf_grp;

    void* m_mpp_ctx;
    void* m_mpp_api;    
};
}
#endif
