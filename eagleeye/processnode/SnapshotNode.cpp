#include "eagleeye/processnode/SnapshotNode.h"
#include "eagleeye/common/EagleeyeFile.h"
#include "eagleeye/common/EagleeyeTime.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "libyuv.h"
#ifdef EAGLEEYE_MINIO
#include "miniocpp/client.h"
#endif

#ifdef EAGLEEYE_RKCHIP
#include "im2d_version.h"
#include "rk_type.h"
#include "rk_mpi.h"
#include "RgaUtils.h"
#include "im2d_buffer.h"
#include "im2d_type.h"
#include "im2d_single.h"
#include "utils/utils.h"
#include "osal/inc/mpp_env.h"
#include "osal/inc/mpp_mem.h"
#include "osal/inc/mpp_common.h"
#include "utils/mpi_enc_utils.h"
#include "mpp_rc_api.h"
#endif

#ifdef EAGLEEYE_FFMPEG
extern "C" {
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libswresample/swresample.h"
#include "libavutil/opt.h"
#include "libavutil/channel_layout.h"
#include "libavutil/parseutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/fifo.h"
#include "libavutil/hwcontext.h"
#include "libavutil/intreadwrite.h"
#include "libavutil/dict.h"
#include "libavutil/display.h"
#include "libavutil/mathematics.h"
#include "libavutil/pixdesc.h"
#include "libavutil/avstring.h"
#include "libavutil/imgutils.h"
#include "libavutil/timestamp.h"
#include "libavutil/bprint.h"
#include "libavutil/time.h"
#include "libavutil/threadmessage.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"
#include "libavutil/frame.h"
#include "libavcodec/avcodec.h"
}
#endif

namespace eagleeye{
SnapeshotNode::SnapeshotNode(){
    this->setNumberOfInputSignals(1);
    this->setNumberOfOutputSignals(1);
    this->setOutputPort(new ImageSignal<Array<unsigned char, 3>>(), 0);
    m_is_init = false;
    this->m_image_format = 1;   // 默认BGR

    EAGLEEYE_MONITOR_VAR(std::string, setFilePath, getFilePath, "file","","");
    EAGLEEYE_MONITOR_VAR(std::string, setPrefix, getPrefix, "prefix","","");
    EAGLEEYE_MONITOR_VAR(std::string, setFolder, getFolder, "folder","","");

    EAGLEEYE_MONITOR_VAR(std::string, setBaseURL, getBaseURL, "base_url","","");
    EAGLEEYE_MONITOR_VAR(std::string, setAccessKey, getAccessKey, "access_key","","");
    EAGLEEYE_MONITOR_VAR(std::string, setSecretKey, getSecretKey, "secret_key","","");
    EAGLEEYE_MONITOR_VAR(std::string, setBucketName, getBucketName, "bucket_name","","");
    EAGLEEYE_MONITOR_VAR(bool, setIsUpload, getIsUpload, "is_upload","","");
    EAGLEEYE_MONITOR_VAR(bool, setIsSecure, getIsSecure, "is_secure","","");

    m_pkt_buf = NULL;
    m_frame_buf = NULL;
    m_md_info = NULL;    
    m_buf_grp = NULL;
    m_mpp_ctx = NULL;
    m_mpp_api = NULL;

    m_frame_size = 0;
    m_header_size = 0;

    m_is_serial = false;
    m_snapshot_count = 0;

#if defined(EAGLEEYE_FFMPEG) && !defined(EAGLEEYE_RKCHIP)
    m_codec_cxt = NULL;
    m_frame = NULL;
    m_pkt = NULL;
    m_encoder = NULL;
#endif
}

SnapeshotNode::~SnapeshotNode(){
#ifdef EAGLEEYE_RKCHIP
    if(m_pkt_buf != NULL){
        mpp_buffer_put((MppBuffer)(m_pkt_buf));
    }
    if(m_frame_buf != NULL){
        mpp_buffer_put((MppBuffer)(m_frame_buf));
    }
    if(m_md_info != NULL){
        mpp_buffer_put((MppBuffer)(m_md_info));
    }
    if(m_buf_grp != NULL){
        mpp_buffer_group_put((MppBufferGroup)(m_buf_grp));
    }
    if(m_mpp_ctx != NULL){
        MppCtx mpp_ctx = (MppCtx)m_mpp_ctx;
        MppApi* mpp_api = (MppApi*)m_mpp_api;

        mpp_api->reset(mpp_ctx);
        mpp_destroy(mpp_ctx);
    }
#endif

#if defined(EAGLEEYE_FFMPEG) && !defined(EAGLEEYE_RKCHIP)
    if(m_codec_cxt != NULL){
        avcodec_close(m_codec_cxt);
        avcodec_free_context(&m_codec_cxt);
    }
    
    if(m_frame != NULL){
        av_frame_free(&m_frame);
    }
    if(m_pkt != NULL){
        av_packet_free(&m_pkt);
    }
#endif
}

#ifdef EAGLEEYE_RKCHIP
extern MPP_RET mpp_enc_cfg_setup(MppEncCfg  cfg, MppApi* mpi, MppCtx ctx, int frame_width, int frame_height, int frame_elem_size, int frame_pix_fmt, MppCodingType type);
#endif

void SnapeshotNode::executeNodeInfo(){
    // 支持RGB/BGR, RGBA/BGRA
    int image_h = 0;
    int image_w = 0;
    int elem_size = 3;
    unsigned char* image_ptr = NULL;
    MetaData image_meta_data;
    if(m_image_format == 0 || m_image_format == 1){
        ImageSignal<Array<unsigned char,3>>* input_img_signal = 
                        (ImageSignal<Array<unsigned char,3>>*)(this->getInputPort(0));
        m_c3_image = input_img_signal->getData(image_meta_data);
        if(m_c3_image.empty()){
            // empty, directly return
            this->getOutputPort(0)->meta().is_snapshot_frame = false;
            return;
        }
        if(!m_c3_image.isContinuous()){
            m_c3_image = m_c3_image.clone();
        }

        image_h = m_c3_image.rows();
        image_w = m_c3_image.cols();
        image_ptr = m_c3_image.cpu<unsigned char>();
        elem_size = 3;
#ifdef EAGLEEYE_RKCHIP
        // RK硬件加速，不支持3通道格式，不得不转换到4通道
        m_c4_image = Matrix<Array<unsigned char,4>>(image_h, image_w);
        unsigned char* c3_ptr = m_c3_image.cpu<unsigned char>();
        unsigned char* c4_ptr = m_c4_image.cpu<unsigned char>();
        for(int i=0; i<image_h; ++i){
            unsigned char* c3_row_ptr = c3_ptr + i*image_w*3;
            unsigned char* c4_row_ptr = c4_ptr + i*image_w*4;
            for(int j=0; j<image_w; ++j){
                c4_row_ptr[j*4] = c3_row_ptr[j*3];
                c4_row_ptr[j*4+1] = c3_row_ptr[j*3+1];
                c4_row_ptr[j*4+2] = c3_row_ptr[j*3+2];
                c4_row_ptr[j*4+3] = 0;
            }
        }
        image_ptr = c4_ptr;
        elem_size = 4;
#endif
    }
    else{
        ImageSignal<Array<unsigned char,4>>* input_img_signal = 
                        (ImageSignal<Array<unsigned char,4>>*)(this->getInputPort(0));
        m_c4_image = input_img_signal->getData(image_meta_data);
        if(m_c4_image.empty()){
            // empty, directly return
            this->getOutputPort(0)->meta().is_snapshot_frame = false;
            return;
        }
        if(!m_c4_image.isContinuous()){
            m_c4_image = m_c4_image.clone();
        }

        image_h = m_c4_image.rows();
        image_w = m_c4_image.cols();
        elem_size = 4;
        image_ptr = m_c4_image.cpu<unsigned char>();
    }

    if(!image_meta_data.is_snapshot_frame){
        // no snapshot, directly return
        this->getOutputPort(0)->meta().is_snapshot_frame = false;
        return;
    }

    // snapshot
    this->getOutputPort(0)->meta().is_snapshot_frame = true;

    // 设置文件名
    if(image_meta_data.name != ""){
        // 检查目录
        if(this->m_folder != "./"){
            if(!isdirexist(this->m_folder.c_str())){
                createdirectory(this->m_folder.c_str());
            }
        }

        if(this->m_is_serial){
            this->m_file_path = this->m_folder+this->m_prefix + std::string(image_meta_data.name) + std::string("_") + std::to_string(this->m_snapshot_count) +".jpg";
        }
        else{
            this->m_file_path = this->m_folder+this->m_prefix + std::string(image_meta_data.name) +".jpg";
        }
    }

    if(this->m_file_path.empty()){
        // 检查目录
        if(this->m_folder != "./"){
            if(!isdirexist(this->m_folder.c_str())){
                createdirectory(this->m_folder.c_str());
            }
        }

        // 动态生成唯一文件名
        if(this->m_is_serial){
            this->m_file_path = this->m_folder+this->m_prefix +  std::to_string(this->m_snapshot_count)+".jpg";
        }
        else{
            this->m_file_path = this->m_folder+this->m_prefix + EagleeyeTime::getTimeStamp()+".jpg";
        }
    }

    // open file
    m_output_file.open(m_file_path.c_str(), std::ios::binary);
    EAGLEEYE_LOGD("open snapshot %s at %d", m_file_path.c_str(), int(EagleeyeTime::getCurrentTime()));

    if(!this->m_is_init){
#ifdef EAGLEEYE_RKCHIP
        /*
        * Can use packet with normal malloc buffer as input not pkt_buf.
        * Please refer to vpu_api_legacy.cpp for normal buffer case.
        * Using pkt_buf buffer here is just for simplifing demo.
        */
        MppBufferGroup buf_grp = nullptr;
        mpp_buffer_group_get_internal(&buf_grp, MPP_BUFFER_TYPE_DRM);
        MppBuffer pkt_buf;
        m_frame_size = MPP_ALIGN(image_w*elem_size, 64) * MPP_ALIGN(image_h, 64);
        mpp_buffer_get(buf_grp, &pkt_buf, m_frame_size);
        m_pkt_buf = pkt_buf;

        MppBuffer frame_buf;
        MppFrameFormat fmt;
        if(m_image_format == 2 || m_image_format == 0){
            //RGBA
            fmt = MPP_FMT_RGBA8888;
        }
        else{
            //BGRA
            fmt = MPP_FMT_BGRA8888;
        }

        m_header_size = 0;
        if (MPP_FRAME_FMT_IS_FBC(fmt)) {
            if ((fmt & MPP_FRAME_FBC_MASK) == MPP_FRAME_FBC_AFBC_V1)
                m_header_size = MPP_ALIGN(MPP_ALIGN(image_w, 16) * MPP_ALIGN(image_h, 16) / 16, SZ_4K);
            else
                m_header_size = MPP_ALIGN(image_w, 16) * MPP_ALIGN(image_h, 16) / 16;
        }
        mpp_buffer_get(buf_grp, &frame_buf, m_frame_size + m_header_size);
        m_frame_buf = frame_buf;

        MppBuffer md_info;
        int mdinfo_size = (MPP_VIDEO_CodingHEVC == MPP_VIDEO_CodingMJPEG) ?
                      (MPP_ALIGN(image_w*elem_size, 32) >> 5) *
                      (MPP_ALIGN(image_h, 32) >> 5) * 16 :
                      (MPP_ALIGN(image_w*elem_size, 64) >> 6) *
                      (MPP_ALIGN(image_h, 16) >> 4) * 16;
        mpp_buffer_get(buf_grp, &md_info, mdinfo_size);
        m_md_info = md_info;

        MppCtx mpp_ctx;
        MppApi* mpp_api;
        MPP_RET ret = MPP_OK;
        ret = mpp_create(&mpp_ctx, &mpp_api);
        if (MPP_OK != ret) {
            EAGLEEYE_LOGE("MPP mpp_create failure.");
        }

        MppPollType timeout = MPP_POLL_BLOCK;
        ret = mpp_api->control(mpp_ctx, MPP_SET_OUTPUT_TIMEOUT, &timeout);
        if (MPP_OK != ret) {
            EAGLEEYE_LOGE("mpi control set output timeout %d ret %d", timeout, ret);
            return;
        }

        ret = mpp_init(mpp_ctx, MPP_CTX_ENC, MPP_VIDEO_CodingMJPEG);
        if (ret) {
            EAGLEEYE_LOGE("mpp_init failed ret %d", ret);
            return;
        }

        MppEncCfg cfg;
        ret = mpp_enc_cfg_init(&cfg);
        if (ret) {
            EAGLEEYE_LOGE("mpp_enc_cfg_init failed ret %d", ret);
            return;
        }

        ret = mpp_api->control(mpp_ctx, MPP_ENC_GET_CFG, cfg);
        if (ret) {
            EAGLEEYE_LOGE("get enc cfg failed ret %d", ret);
            return;
        }

        if(this->m_image_format == 2 || this->m_image_format == 0){
            ret = mpp_enc_cfg_setup(cfg, mpp_api, mpp_ctx, image_w, image_h, elem_size, 2, MPP_VIDEO_CodingMJPEG);
        }
        else{
            ret = mpp_enc_cfg_setup(cfg, mpp_api, mpp_ctx, image_w, image_h, elem_size, 3, MPP_VIDEO_CodingMJPEG);
        }
        if (ret) {
            EAGLEEYE_LOGE("mpp setup failed ret %d", ret);
            return;
        }

        m_mpp_ctx = mpp_ctx;
        m_mpp_api = mpp_api;
#endif

#if defined(EAGLEEYE_FFMPEG) && !defined(EAGLEEYE_RKCHIP)
        // 初始化编码器（使用ffmpeg AV_CODEC_ID_MJPEG）
        m_encoder = avcodec_find_encoder(AV_CODEC_ID_MJPEG);

        //初始化并设置编码器上下文
        m_codec_cxt = avcodec_alloc_context3(m_encoder);
        m_codec_cxt->bit_rate = 3000000;
        m_codec_cxt->width = image_w;
        m_codec_cxt->height = image_h;
        m_codec_cxt->time_base.num = 1;
        m_codec_cxt->time_base.den = 25;
        m_codec_cxt->pix_fmt = AV_PIX_FMT_YUV420P;
        m_codec_cxt->gop_size = 10;
        m_codec_cxt->max_b_frames = 0;
        m_codec_cxt->thread_count = 1;
        m_codec_cxt->strict_std_compliance = FF_COMPLIANCE_UNOFFICIAL;

        //编码器初始化
        int ret = avcodec_open2(m_codec_cxt, m_encoder, NULL);
        if(ret < 0){
            EAGLEEYE_LOGE("Could not avcodec_open2 err=%d.", ret);
        }

        m_pkt = av_packet_alloc();
        if(!m_pkt){
            EAGLEEYE_LOGE("Could not allocate video packet");
        }
        m_frame = av_frame_alloc();
        if (!m_frame) {
            EAGLEEYE_LOGE("Could not allocate video frame");
        }

        m_frame->format = m_codec_cxt->pix_fmt;
        m_frame->width  = m_codec_cxt->width;
        m_frame->height = m_codec_cxt->height;

        ret = av_frame_get_buffer(m_frame, 0);
        if (ret < 0) {
            EAGLEEYE_LOGE("Could not allocate the video frame data");
        }
#endif

        m_is_init = true;
    }

#ifdef EAGLEEYE_RKCHIP
    // RGB/BGR， RGBA/BGRA
    MppCtx mpp_ctx = (MppCtx)(m_mpp_ctx);
    MppApi* mpp_api = (MppApi*)m_mpp_api;
    MppPacket packet = NULL;
    MppBuffer pkt_buf = (MppBuffer)m_pkt_buf;
    MPP_RET ret = MPP_OK;

    MppMeta meta = NULL;
    MppFrame frame = NULL;
    MppBuffer frame_buf = (MppBuffer)(m_frame_buf);
    void *buf = mpp_buffer_get_ptr(frame_buf);
    RK_U32 eoi = 1;
    memcpy(buf, image_ptr, image_h*image_w*elem_size);

    // 初始化frame
    ret = mpp_frame_init(&frame);
    if (ret) {
        mpp_err_f("mpp_frame_init failed\n");
        return;
    }

    mpp_frame_set_width(frame, image_w);
    mpp_frame_set_height(frame, image_h);
    mpp_frame_set_hor_stride(frame, image_w*elem_size);
    mpp_frame_set_ver_stride(frame, image_h);
    if(m_image_format == 2 || m_image_format == 0){
        // RGBA
        mpp_frame_set_fmt(frame, MPP_FMT_RGBA8888);
    }
    else{
        // BGRA
        mpp_frame_set_fmt(frame, MPP_FMT_BGRA8888);
    }
    mpp_frame_set_eos(frame, 0);
    mpp_frame_set_buffer(frame, frame_buf);

    meta = mpp_frame_get_meta(frame);
    mpp_packet_init_with_buffer(&packet, pkt_buf);
    /* NOTE: It is important to clear output packet length!! */
    mpp_packet_set_length(packet, 0);
    mpp_meta_set_packet(meta, KEY_OUTPUT_PACKET, packet);
    mpp_meta_set_buffer(meta, KEY_MOTION_INFO, m_md_info);

    /*
        * NOTE: in non-block mode the frame can be resent.
        * The default input timeout mode is block.
        *
        * User should release the input frame to meet the requirements of
        * resource creator must be the resource destroyer.
        */
    ret = mpp_api->encode_put_frame(mpp_ctx, frame);
    if (ret) {
        mpp_frame_deinit(&frame);
        return;
    }
    mpp_frame_deinit(&frame);

    do {
        ret = mpp_api->encode_get_packet(mpp_ctx, &packet);
        if (ret) {
            return;
        }

        if (packet) {
            // write packet to file here
            void *ptr   = mpp_packet_get_pos(packet);
            size_t len  = mpp_packet_get_length(packet);
            m_output_file.write((char*)ptr, len);

            /* for low delay partition encoding */
            if (mpp_packet_is_partition(packet)) {
                eoi = mpp_packet_is_eoi(packet);
            }
            mpp_packet_deinit(&packet);
        }
    } while (!eoi);
#endif

#if defined(EAGLEEYE_FFMPEG) && !defined(EAGLEEYE_RKCHIP)
    // rgb->yuv420p, bgr->yuv420p, rgba->yuv420p, bgra->yuv420p
    if(m_image_format == 0){
        // RGB
        libyuv::RAWToI420(image_ptr, 
            image_w*3, 
            m_frame->data[0],m_frame->linesize[0],
            m_frame->data[1],m_frame->linesize[1],
            m_frame->data[2],m_frame->linesize[2],
            m_frame->width,
            m_frame->height);
    }
    else if(m_image_format == 1){
        // BGR
        libyuv::RGB24ToI420(image_ptr, 
            image_w*3, 
            m_frame->data[0],m_frame->linesize[0],
            m_frame->data[1],m_frame->linesize[1],
            m_frame->data[2],m_frame->linesize[2],
            m_frame->width,
            m_frame->height);
    }
    else if(m_image_format == 2){
        libyuv::RGBAToI420(image_ptr, 
            image_w*4, 
            m_frame->data[0],m_frame->linesize[0],
            m_frame->data[1],m_frame->linesize[1],
            m_frame->data[2],m_frame->linesize[2],
            m_frame->width,
            m_frame->height);
    }
    else{  
        libyuv::BGRAToI420(image_ptr, 
            image_w*4, 
            m_frame->data[0],m_frame->linesize[0],
            m_frame->data[1],m_frame->linesize[1],
            m_frame->data[2],m_frame->linesize[2],
            m_frame->width,
            m_frame->height);   
    }
    
    int ret = av_frame_make_writable(m_frame);
    if(ret < 0){
        EAGLEEYE_LOGE("Could not av_frame_make_writable");
    }
    ret = avcodec_send_frame(m_codec_cxt, m_frame);
    if(ret < 0) {
        EAGLEEYE_LOGE("Error sending a frame for encoding");
    }    
    ret = avcodec_receive_packet(m_codec_cxt, m_pkt);
    if(ret < 0){
        EAGLEEYE_LOGE("Error during encoding");        
    }

    m_output_file.write((char*)m_pkt->data, m_pkt->size);
    av_packet_unref(m_pkt);
#endif

    m_output_file.close();
    EAGLEEYE_LOGD("finish snapshot %s at %d", m_file_path.c_str(), int(EagleeyeTime::getCurrentTime()));


#ifdef EAGLEEYE_MINIO
    // 运行至此，说明已经保存文件
    if(m_is_upload){
        this->uploader(this->m_file_path);
    }
#endif //EAGLEEYE_MINIO

    this->m_snapshot_count += 1;
}

void SnapeshotNode::setImageFormat(int image_format){
    // image_format: 0: RGB; 1: BGR, 2: RGBA, 3: BGRA
    if(image_format > 3){
        EAGLEEYE_LOGE("image format only support 0(RGB),1(BGR),2(RGBA),3(BGRA)");
        return;
    }

    this->m_image_format = image_format;
}

void SnapeshotNode::setFilePath(std::string file_path){
    this->m_file_path = file_path;
}

void SnapeshotNode::getFilePath(std::string& file_path){
    file_path = this->m_file_path;
}

void SnapeshotNode::setPrefix(std::string prefix){
    this->m_prefix = prefix;
}

void SnapeshotNode::getPrefix(std::string& prefix){
    prefix = this->m_prefix;
}

void SnapeshotNode::setFolder(std::string folder){
    this->m_folder = folder;
    if(!endswith(this->m_folder, "/")){
        this->m_folder = folder +"/";
    }
}
void SnapeshotNode::getFolder(std::string& folder){
    folder = this->m_folder;
}

void SnapeshotNode::setSerial(bool is_serial){
    this->m_is_serial = is_serial;
}

int SnapeshotNode::getSerialNum(){
    return m_snapshot_count;
}

void SnapeshotNode::setBaseURL(std::string url){
    this->m_base_url = url;
}

void SnapeshotNode::getBaseURL(std::string& url){
    url = this->m_base_url;
}

void SnapeshotNode::setAccessKey(std::string access_key){
    this->m_access_key = access_key;
}

void SnapeshotNode::getAccessKey(std::string& access_key){
    access_key = this->m_access_key;
}

void SnapeshotNode::setSecretKey(std::string secret_key){
    this->m_secret_key = secret_key;
}

void SnapeshotNode::getSecretKey(std::string& secret_key){
    secret_key = this->m_secret_key;
}

void SnapeshotNode::setBucketName(std::string bucket_name){
    this->m_bucket_name = bucket_name;
}

void SnapeshotNode::getBucketName(std::string& bucket_name){
    bucket_name = this->m_bucket_name;
}

void SnapeshotNode::setIsUpload(bool upload){
    this->m_is_upload = upload;
}

void SnapeshotNode::getIsUpload(bool& upload){
    upload = this->m_is_upload;
}

void SnapeshotNode::setIsSecure(bool secure){
    this->m_is_secure = secure;
}

void SnapeshotNode::getIsSecure(bool& secure){
    secure = this->m_is_secure;
}

bool SnapeshotNode::uploader(const std::string &src_file){
#ifdef EAGLEEYE_MINIO
    if(src_file.empty()){
        EAGLEEYE_LOGE("error src file = %s ",src_file);
    }
    //0. makr clent
    std::shared_ptr<minio::s3::Client> client = nullptr;
    minio::s3::BaseUrl base_url(m_base_url, m_is_secure);
    minio::creds::StaticProvider provider(m_access_key,m_secret_key);
    try
    {
        client =std::make_shared<minio::s3::Client>(base_url, &provider);
    }
    catch(const std::exception& e)
    {
        EAGLEEYE_LOGE("creat client error  = %s", e.what());
        return false;
    }

    if(client == nullptr){
        EAGLEEYE_LOGE("creat client error ");
        return false;
    }

    //1. check bucket
    bool exist = false;
    try{
        minio::s3::BucketExistsArgs args;
        args.bucket = m_bucket_name;
        minio::s3::BucketExistsResponse resp = client->BucketExists(args);
        if (!resp) {
            EAGLEEYE_LOGE("unable to do bucket existence check: error = %s", resp.Error());
            return false;
        }
        exist = resp.exist;
    }
    catch(const std::exception& e)
    {
        EAGLEEYE_LOGE("BucketExists throw error = %s", e.what());
        return false;
    }

    //2. make bucket
    if(not exist){
        try{
            minio::s3::MakeBucketArgs args;
            args.bucket = m_bucket_name;
            minio::s3::MakeBucketResponse resp = client->MakeBucket(args);
            if (!resp) {
                EAGLEEYE_LOGE("unable to create bucket, error = %s ", resp.Error());
                return false;
            }
        }
        catch(const std::exception& e){
            EAGLEEYE_LOGE("MakeBucket throw error = %s",e.what());
            return false;
        }
    }

    //3. upload
    auto getDestFileName = [](const std::string &src_file){
        auto found = src_file.find_last_of("/\\");
        if(found == std::string::npos){
            return src_file;
        }
        return src_file.substr(found+1);
    };
    std::string dst_file = getDestFileName(src_file);
    if(dst_file.empty()){
        EAGLEEYE_LOGE("failed upload object [%s]");
        return false;
    }

    try{
        minio::s3::UploadObjectArgs args;
        args.bucket = m_bucket_name;
        args.object = dst_file;
        args.filename = src_file;
        args.content_type = "video/mp4";

        minio::s3::UploadObjectResponse resp = client->UploadObject(args);
        if (!resp) {
            EAGLEEYE_LOGE("unable to upload object [%s], error = %s",src_file, resp.Error());
            return false;
        }   
    }
    catch(const std::exception& e)
    {
        EAGLEEYE_LOGE("UploadObject throw error = %s", e.what());
        return false;
    }
    return true;
#else
    return true;
#endif
}
}