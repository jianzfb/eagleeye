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
class AVFormatContext;
class AVStream;
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

    void setPause(int pause);
    void getPause(int& pause);

    void setFPS(int fps);
    void getFPS(int& fps);

    /*
     * @brief 设置序列标记（加入序列命名）
     */
    void setSerial(bool is_serial);

    /**
     * @brief get serial number
     */
    int getSerialNum();

    /*
     * @brief minio
     */
    void setBaseURL(std::string url);

    void getBaseURL(std::string& url);

    void setAccessKey(std::string access_key);

    void getAccessKey(std::string& access_key);

    void setSecretKey(std::string secret_key);

    void getSecretKey(std::string& secret_key);

    void setBucketName(std::string bucket_name);

    void getBucketName(std::string& bucket_name);

    void setIsUpload(bool upload);

    void getIsUpload(bool& upload);

    void setIsSecure(bool secure);

    void getIsSecure(bool& secure);

private:
    VideoWriteNode(const VideoWriteNode&);
    void operator=(const VideoWriteNode&);

    /**
     * @brief finish writing process
     * 
     */
    void writeFinish(AnySignal* out_sig=NULL);

    /**
      * @brief h264 to mp4 for rk
      */
    bool h264Muxing(char*packet, std::uint64_t packet_size);

    bool m_is_init;
    bool m_is_finish;
    int m_fps;

    bool m_is_header_init;

    std::string m_prefix;
    std::string m_folder;

    int m_manually_stop;
    int m_manually_start;
    int m_manually_pause;

    AVFormatContext* m_fmt_context_ff = nullptr;
    AVStream* m_stream_ff = nullptr;
    AVCodecContext* m_codec_cxt = nullptr;
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

    bool m_is_serial;
    int m_video_count;

    //for rk h264 to mp4
    AVFormatContext* m_fmt_context = nullptr;
    AVStream* m_stream = nullptr;
    AVCodecContext* m_codec_ctx = nullptr;
    AVPacket* m_av_packet = nullptr;

    //for upload for minio
    std::string m_base_url;
    std::string m_access_key;
    std::string m_secret_key;
    std::string m_bucket_name;
    bool uploader(const std::string &src_file);
    bool m_is_upload = false;
    bool m_is_secure = false;
};
}
#endif
