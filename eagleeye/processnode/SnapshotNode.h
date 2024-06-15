#ifndef _EAGLEEYE_SNAPSHOT_NODE_H_
#define _EAGLEEYE_SNAPSHOT_NODE_H_
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
class SnapeshotNode:public AnyNode, DynamicNodeCreator<SnapeshotNode>{
public:
    typedef SnapeshotNode               Self;
    typedef AnyNode                     Superclass;

    EAGLEEYE_CLASSIDENTITY(SnapeshotNode);
    SnapeshotNode();
    virtual ~SnapeshotNode();

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

    void setImageFormat(int image_format);

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
    SnapeshotNode(const SnapeshotNode&);
    void operator=(const SnapeshotNode&);
    Matrix<Array<unsigned char, 4>> m_c4_image;
    Matrix<Array<unsigned char, 3>> m_c3_image;

    int m_frame_size;
    int m_header_size;
    int m_image_format;

    void* m_pkt_buf;
    void* m_frame_buf;
    void* m_md_info;
    void* m_buf_grp;

    void* m_mpp_ctx;
    void* m_mpp_api;
    bool m_is_init;

    std::string m_folder;
    std::string m_prefix;
    std::string m_file_path;
    std::ofstream m_output_file;

    bool m_is_serial;
    int m_snapshot_count;

    // FFMPEG
    AVCodecContext* m_codec_cxt;
    const AVCodec* m_encoder;
    AVFrame *m_frame;
    AVPacket *m_pkt;

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