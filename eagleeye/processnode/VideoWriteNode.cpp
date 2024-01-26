#include "eagleeye/processnode/VideoWriteNode.h"
#include "eagleeye/processnode/ImageWriteNode.h"
#include "eagleeye/common/EagleeyeFile.h"
#include "eagleeye/common/EagleeyeTime.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "libyuv.h"
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
namespace  eagleeye
{
VideoWriteNode::VideoWriteNode(){
    this->setNumberOfInputSignals(1);
    this->setNumberOfOutputSignals(1);
    this->setOutputPort(this->makeOutputSignal(), 0);
    this->getOutputPort(0)->meta().is_start_frame = false;
    this->getOutputPort(0)->meta().is_end_frame = false;
    this->getOutputPort(0)->meta().is_pause_frame = false;

    this->m_is_init = false;
    this->m_is_header_init = false;
    this->m_is_finish = true;
    this->m_fps = 30;
    this->m_image_format = 1;   // 默认BGR

    // FFMPEG原生编码器
    m_codec_cxt = NULL;
    m_frame_count = 0;
    m_frame = NULL;
    m_pkt = NULL;
    m_encoder = NULL;

    EAGLEEYE_MONITOR_VAR(std::string, setFilePath, getFilePath, "file","","");
    EAGLEEYE_MONITOR_VAR(std::string, setPrefix, getPrefix, "prefix","","");
    EAGLEEYE_MONITOR_VAR(std::string, setFolder, getFolder, "folder","","");

    EAGLEEYE_MONITOR_VAR(int, setStop, getStop, "stop","0","2");
    EAGLEEYE_MONITOR_VAR(int, setStart, getStart, "start","0","2");

    this->m_prefix = "video_";
    this->m_folder = "/sdcard/";
    this->m_manually_stop = 0;
    this->m_manually_start = 0;
    this->m_manually_pause = 0;

    // RK MPP硬件加速编码器
    m_pkt_buf = NULL;
    m_frame_buf = NULL;
    m_md_info = NULL;    
    m_buf_grp = NULL;
    m_mpp_ctx = NULL;
    m_mpp_api = NULL;

    m_frame_size = 0;
    m_header_size = 0;
}

VideoWriteNode::~VideoWriteNode(){
    if(!this->m_is_finish){
        this->writeFinish();
    }

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
}

#ifndef EAGLEEYE_RKCHIP
bool encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt, std::ofstream& fp)
{
    int ret;
    ret = avcodec_send_frame(enc_ctx, frame);
    if (ret < 0) {
        EAGLEEYE_LOGE("Error sending a frame for encoding");
        return false;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(enc_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
            av_packet_unref(pkt);
            return false;
        }
        else if (ret < 0) {
            av_packet_unref(pkt);
            EAGLEEYE_LOGE("Error during encoding");
            return false;
        }

        fp.write((char*)(pkt->data), pkt->size);
        av_packet_unref(pkt);
    }

    return true;
}
#endif

#ifdef EAGLEEYE_RKCHIP
MPP_RET mpp_enc_cfg_setup(MppEncCfg  cfg, MppApi* mpi, MppCtx ctx, int frame_width, int frame_height, int frame_elem_size, int frame_pix_fmt, MppCodingType type){
    MPP_RET ret;
    RK_U32 rotation;
    RK_U32 mirroring;
    RK_U32 flip;
    RK_U32 gop_mode = 0;
    MppEncRefCfg ref = NULL;

    /* setup default parameter */
    int fps_in_den = 1;
    int fps_in_num = 25;
    int fps_out_den = 1;
    int fps_out_num = 25;
    int fps_in_flex = 0;
    int fps_out_flex = 0;

    int bps = frame_width * frame_height / 8 * (fps_out_num / fps_out_den);
    mpp_enc_cfg_set_s32(cfg, "tune:scene_mode", 0);

    mpp_enc_cfg_set_s32(cfg, "prep:width", frame_width);
    mpp_enc_cfg_set_s32(cfg, "prep:height", frame_height);
    mpp_enc_cfg_set_s32(cfg, "prep:hor_stride", frame_width*frame_elem_size);
    mpp_enc_cfg_set_s32(cfg, "prep:ver_stride", frame_height);
    if(frame_pix_fmt == 0){
        //RGB
        mpp_enc_cfg_set_s32(cfg, "prep:format", MPP_FMT_RGB888);
    }
    else if(frame_pix_fmt == 1){
        //BGR
        mpp_enc_cfg_set_s32(cfg, "prep:format", MPP_FMT_BGR888);
    }
    else if(frame_pix_fmt == 2){
        //RGBA
        mpp_enc_cfg_set_s32(cfg, "prep:format", MPP_FMT_RGBA8888);
    }
    else{
        //BGRA
        mpp_enc_cfg_set_s32(cfg, "prep:format", MPP_FMT_BGRA8888);
    }

    MppEncRcMode rc_mode = MPP_ENC_RC_MODE_CBR;
    mpp_enc_cfg_set_s32(cfg, "rc:mode", rc_mode);

    /* fix input / output frame rate */
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_flex", fps_in_flex);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_num", fps_in_num);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_in_denorm", fps_in_den);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_flex", fps_out_flex);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_num", fps_out_num);
    mpp_enc_cfg_set_s32(cfg, "rc:fps_out_denorm", fps_out_den);

    /* drop frame or not when bitrate overflow */
    mpp_enc_cfg_set_u32(cfg, "rc:drop_mode", MPP_ENC_RC_DROP_FRM_DISABLED);
    mpp_enc_cfg_set_u32(cfg, "rc:drop_thd", 20);        /* 20% of max bps */
    mpp_enc_cfg_set_u32(cfg, "rc:drop_gap", 1);         /* Do not continuous drop frame */

    /* setup bitrate for different rc_mode */
    mpp_enc_cfg_set_s32(cfg, "rc:bps_target", bps);
    switch (rc_mode) {
    case MPP_ENC_RC_MODE_FIXQP : {
        /* do not setup bitrate on FIXQP mode */
    } break;
    case MPP_ENC_RC_MODE_CBR : {
        /* CBR mode has narrow bound */
        mpp_enc_cfg_set_s32(cfg, "rc:bps_max", bps * 17 / 16);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_min", bps * 15 / 16);
    } break;
    case MPP_ENC_RC_MODE_VBR :
    case MPP_ENC_RC_MODE_AVBR : {
        /* VBR mode has wide bound */
        mpp_enc_cfg_set_s32(cfg, "rc:bps_max", bps * 17 / 16);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_min", bps * 1 / 16);
    } break;
    default : {
        /* default use CBR mode */
        mpp_enc_cfg_set_s32(cfg, "rc:bps_max", bps * 17 / 16);
        mpp_enc_cfg_set_s32(cfg, "rc:bps_min", bps * 15 / 16);
    } break;
    }

    /* setup qp for different codec and rc_mode */
    int qp_init = 40;
    switch (type) {
    case MPP_VIDEO_CodingAVC :
    case MPP_VIDEO_CodingHEVC : {
        switch (rc_mode) {
        case MPP_ENC_RC_MODE_FIXQP : {
            RK_S32 fix_qp = qp_init;

            mpp_enc_cfg_set_s32(cfg, "rc:qp_init", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_ip", 0);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_min_i", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_max_i", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_min_p", fix_qp);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_max_p", fix_qp);
        } break;
        case MPP_ENC_RC_MODE_CBR :
        case MPP_ENC_RC_MODE_VBR :
        case MPP_ENC_RC_MODE_AVBR : {
            mpp_enc_cfg_set_s32(cfg, "rc:qp_init", qp_init);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max", 51);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min", 10);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i", 51);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i", 10);
            mpp_enc_cfg_set_s32(cfg, "rc:qp_ip", 2);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_min_i", 10);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_max_i", 51);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_min_p", 10);
            mpp_enc_cfg_set_s32(cfg, "rc:fqp_max_p", 51);
        } break;
        default : {
            EAGLEEYE_LOGE("unsupport encoder rc mode %d\n", rc_mode);
        } break;
        }
    } break;
    case MPP_VIDEO_CodingVP8 : {
        /* vp8 only setup base qp range */
        mpp_enc_cfg_set_s32(cfg, "rc:qp_init", 40);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_max",  127);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_min",  0);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_max_i", 127);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_min_i", 0);
        mpp_enc_cfg_set_s32(cfg, "rc:qp_ip", 6);
    } break;
    case MPP_VIDEO_CodingMJPEG : {
        /* jpeg use special codec config to control qtable */
        mpp_enc_cfg_set_s32(cfg, "jpeg:q_factor", 80);
        mpp_enc_cfg_set_s32(cfg, "jpeg:qf_max", 99);
        mpp_enc_cfg_set_s32(cfg, "jpeg:qf_min", 1);
    } break;
    default : {
    } break;
    }

    /* setup codec  */
    mpp_enc_cfg_set_s32(cfg, "codec:type", type);
    switch (type) {
    case MPP_VIDEO_CodingAVC : {
        RK_U32 constraint_set;

        /*
         * H.264 profile_idc parameter
         * 66  - Baseline profile
         * 77  - Main profile
         * 100 - High profile
         */
        mpp_enc_cfg_set_s32(cfg, "h264:profile", 100);
        /*
         * H.264 level_idc parameter
         * 10 / 11 / 12 / 13    - qcif@15fps / cif@7.5fps / cif@15fps / cif@30fps
         * 20 / 21 / 22         - cif@30fps / half-D1@@25fps / D1@12.5fps
         * 30 / 31 / 32         - D1@25fps / 720p@30fps / 720p@60fps
         * 40 / 41 / 42         - 1080p@30fps / 1080p@30fps / 1080p@60fps
         * 50 / 51 / 52         - 4K@30fps
         */
        mpp_enc_cfg_set_s32(cfg, "h264:level", 40);
        mpp_enc_cfg_set_s32(cfg, "h264:cabac_en", 1);
        mpp_enc_cfg_set_s32(cfg, "h264:cabac_idc", 0);
        mpp_enc_cfg_set_s32(cfg, "h264:trans8x8", 1);

        mpp_env_get_u32("constraint_set", &constraint_set, 0);
        if (constraint_set & 0x3f0000)
            mpp_enc_cfg_set_s32(cfg, "h264:constraint_set", constraint_set);
    } break;
    case MPP_VIDEO_CodingHEVC :
    case MPP_VIDEO_CodingMJPEG :
    case MPP_VIDEO_CodingVP8 : {
    } break;
    default : {
        EAGLEEYE_LOGE("unsupport encoder coding type %d\n", type);
    } break;
    }

    RK_U32 split_mode = 0;
    RK_U32 split_arg = 0;
    RK_U32 split_out = 0;

    mpp_env_get_u32("split_mode", &split_mode, MPP_ENC_SPLIT_NONE);
    mpp_env_get_u32("split_arg", &split_arg, 0);
    mpp_env_get_u32("split_out", &split_out, 0);

    if (split_mode) {
        mpp_enc_cfg_set_s32(cfg, "split:mode", split_mode);
        mpp_enc_cfg_set_s32(cfg, "split:arg", split_arg);
        mpp_enc_cfg_set_s32(cfg, "split:out", split_out);
    }

    mpp_env_get_u32("mirroring", &mirroring, 0);
    mpp_env_get_u32("rotation", &rotation, 0);
    mpp_env_get_u32("flip", &flip, 0);

    mpp_enc_cfg_set_s32(cfg, "prep:mirroring", mirroring);
    mpp_enc_cfg_set_s32(cfg, "prep:rotation", rotation);
    mpp_enc_cfg_set_s32(cfg, "prep:flip", flip);

    // config gop_len and ref cfg
    mpp_enc_cfg_set_s32(cfg, "rc:gop", fps_out_num * 2);
    mpp_env_get_u32("gop_mode", &gop_mode, gop_mode);

    int gop_len = 0; 
    int vi_len = 0;
    if (gop_mode) {
        mpp_enc_ref_cfg_init(&ref);

        if (gop_mode < 4)
            mpi_enc_gen_ref_cfg(ref, gop_mode);
        else
            mpi_enc_gen_smart_gop_ref_cfg(ref, gop_len, vi_len);

        mpp_enc_cfg_set_ptr(cfg, "rc:ref_cfg", ref);
    }

    ret = mpi->control(ctx, MPP_ENC_SET_CFG, cfg);
    if (ret) {
        EAGLEEYE_LOGE("mpi control enc set cfg failed ret %d\n", ret);
        return ret;
    }

    if (ref)
        mpp_enc_ref_cfg_deinit(&ref);

    /* optional */
    {
        RK_U32 sei_mode;

        mpp_env_get_u32("sei_mode", &sei_mode, MPP_ENC_SEI_MODE_ONE_FRAME);
        ret = mpi->control(ctx, MPP_ENC_SET_SEI_CFG, &sei_mode);
        if (ret) {
            EAGLEEYE_LOGE("mpi control enc set sei cfg failed ret %d\n", ret);
            return ret;
        }
    }

    if (type == MPP_VIDEO_CodingAVC || type == MPP_VIDEO_CodingHEVC) {
        MppEncHeaderMode header_mode = MPP_ENC_HEADER_MODE_EACH_IDR;
        ret = mpi->control(ctx, MPP_ENC_SET_HEADER_MODE, &header_mode);
        if (ret) {
            EAGLEEYE_LOGE("mpi control enc set header mode failed ret %d\n", ret);
            return ret;
        }
    }
    return ret;
}
#endif

void VideoWriteNode::executeNodeInfo(){
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
        if(!m_c3_image.isContinuous()){
            m_c3_image = m_c3_image.clone();
        }

        image_h = m_c3_image.rows();
        image_w = m_c3_image.cols();
        elem_size = 3;
        image_ptr = m_c3_image.cpu<unsigned char>();
    }
    else{
        ImageSignal<Array<unsigned char,4>>* input_img_signal = 
                        (ImageSignal<Array<unsigned char,4>>*)(this->getInputPort(0));
        m_c4_image = input_img_signal->getData(image_meta_data);
        if(!m_c4_image.isContinuous()){
            m_c4_image = m_c4_image.clone();
        }

        image_h = m_c4_image.rows();
        image_w = m_c4_image.cols();
        elem_size = 4;
        image_ptr = m_c4_image.cpu<unsigned char>();
    }

    if(!image_meta_data.is_start_frame && !m_manually_start){
        // 对于非首帧(或手动开始)，不可以启动初始化
        // 首帧和尾帧，必须设置
        return;
    }

    // 设置文件名
    if(this->m_file_path == "" && image_meta_data.name != ""){
        // 检查目录
        if(this->m_folder != "./"){
            if(!isdirexist(this->m_folder.c_str())){
                createdirectory(this->m_folder.c_str());
            }
        }

        this->m_file_path = this->m_folder+this->m_prefix + std::string(image_meta_data.name) +".mp4";
    }

    if(this->m_file_path.empty()){
        // 检查目录
        if(this->m_folder != "./"){
            if(!isdirexist(this->m_folder.c_str())){
                createdirectory(this->m_folder.c_str());
            }
        }

        // 动态生成唯一文件名
        this->m_file_path = this->m_folder+this->m_prefix + std::string(image_meta_data.name) +".mp4";
    }

    // 至此，说明已经开始录制
    if(m_frame_count == 0){
        //打开输出文件流(一个新的视频存储)
        m_output_file.open(m_file_path.c_str(), std::ios::binary);

        m_is_header_init = false;
        m_is_finish = false;
        this->getOutputPort(0)->meta().is_end_frame = false;
        this->getOutputPort(0)->meta().is_pause_frame = false;
    }

    this->getOutputPort(0)->meta().is_start_frame = true;
    if(image_meta_data.is_pause_frame || m_manually_pause){
        // 对于暂停帧，跳过
        this->getOutputPort(0)->meta().is_pause_frame = true;
        return;
    }
    this->getOutputPort(0)->meta().is_pause_frame = false;

    if(this->m_is_init && this->m_manually_stop != 0){
        this->writeFinish(this->getOutputPort(0));
        EAGLEEYE_LOGD("Success to finish write video.");
        return;
    }

    if(!this->m_is_init){
        EAGLEEYE_LOGD("Start to write video.");
#ifndef EAGLEEYE_RKCHIP
        //设置编码器
        m_encoder = avcodec_find_encoder_by_name("libx264");
        //初始化并设置编码器上下文
        m_codec_cxt = avcodec_alloc_context3(m_encoder);
        m_codec_cxt->bit_rate = 400000;
        m_codec_cxt->width = image_w;
        m_codec_cxt->height = image_h;
        m_codec_cxt->time_base = (AVRational){1, 25};
        m_codec_cxt->framerate = (AVRational){25, 1};
        m_codec_cxt->pix_fmt = AV_PIX_FMT_YUV420P;
        m_codec_cxt->gop_size = 2;
        m_codec_cxt->max_b_frames = 20;
        m_codec_cxt->thread_count = 4;

        // av_opt_set(m_codec_cxt->priv_data, "preset", "veryfast", 0);
        // av_opt_set(m_codec_cxt->priv_data, "tune", "zerolatency", 0);

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
        if(m_image_format == 0){
            //RGB
            fmt = MPP_FMT_RGB888;
        }
        else if(m_image_format == 1){
            //BGR
            fmt = MPP_FMT_BGR888;
        }
        else if(m_image_format == 2){
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
        int mdinfo_size = (MPP_VIDEO_CodingHEVC == MPP_VIDEO_CodingAVC) ?
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

        ret = mpp_init(mpp_ctx, MPP_CTX_ENC, MPP_VIDEO_CodingAVC);
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

        ret = mpp_enc_cfg_setup(cfg, mpp_api, mpp_ctx, image_w, image_h, elem_size, this->m_image_format, MPP_VIDEO_CodingAVC);
        if (ret) {
            EAGLEEYE_LOGE("mpp setup failed ret %d", ret);
            return;
        }
        m_mpp_ctx = mpp_ctx;
        m_mpp_api = mpp_api;
#endif

        m_is_init = true;
    }

#ifndef EAGLEEYE_RKCHIP
    // 颜色空间转换到YUV420P
    if(image_meta_data.color_format == -1 || image_meta_data.color_format == 0){
        // RGB
        libyuv::RAWToI420((uint8_t*)image.dataptr(), 
            image.cols()*3, 
            m_frame->data[0],m_frame->linesize[0],
            m_frame->data[1],m_frame->linesize[1],
            m_frame->data[2],m_frame->linesize[2],
            m_frame->width,
            m_frame->height);
    }
    else{
        // BGR
        libyuv::RGB24ToI420((uint8_t*)image.dataptr(), 
            image.cols()*3, 
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
    m_frame->pts = this->m_frame_count;

    /* encode the image */
    encode(m_codec_cxt, m_frame, m_pkt, m_output_file);
#endif

#ifdef EAGLEEYE_RKCHIP
    // RGB/BGR， RGBA/BGRA
    MppCtx mpp_ctx = (MppCtx)(m_mpp_ctx);
    MppApi* mpp_api = (MppApi*)m_mpp_api;

    MPP_RET ret = MPP_OK;
    MppPacket packet = NULL;
    MppBuffer pkt_buf = (MppBuffer)m_pkt_buf;
    if(!m_is_header_init){
        mpp_packet_init_with_buffer(&packet, pkt_buf);
        /* NOTE: It is important to clear output packet length!! */
        mpp_packet_set_length(packet, 0);

        ret = mpp_api->control(mpp_ctx, MPP_ENC_GET_HDR_SYNC, packet);
        if (ret) {
            EAGLEEYE_LOGE("mpp_api control enc get extra info failed");
            return;
        } else {
            /* get and write sps/pps for H.264 */
            void *ptr   = mpp_packet_get_pos(packet);
            size_t len  = mpp_packet_get_length(packet);
            m_output_file.write((char*)ptr, len);
        }
        mpp_packet_deinit(&packet);
        packet = NULL;
        m_is_header_init = true;
    }

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
    if(m_image_format == 0){
        // RGB
        mpp_frame_set_fmt(frame, MPP_FMT_RGB888);
    }
    else if(m_image_format == 1){
        // BGR
        mpp_frame_set_fmt(frame, MPP_FMT_BGR888);
    }
    else if(m_image_format == 2){
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

    this->m_frame_count += 1;

    // stop
    if(image_meta_data.is_end_frame){
        this->writeFinish(this->getOutputPort(0));
        EAGLEEYE_LOGD("Success to finish write video.");
    }
}

void VideoWriteNode::setImageFormat(int image_format){
    // image_format: 0: RGB; 1: BGR, 2: RGBA, 3: BGRA
    if(image_format > 3){
        EAGLEEYE_LOGE("image format only support 0(RGB),1(BGR),2(RGBA),3(BGRA)");
        return;
    }

    this->m_image_format = image_format;
}

void VideoWriteNode::setFilePath(std::string file_path){
    if(!this->m_is_finish){
        EAGLEEYE_LOGD("Force unfinish video stop.");
        this->writeFinish(this->getOutputPort(0));        
    }

    this->m_file_path = file_path;
    this->m_is_finish = true;
    this->m_fps = 0;
    this->m_manually_stop = 0;
    this->m_manually_start = 0;
    this->m_manually_pause = 0;
}
void VideoWriteNode::getFilePath(std::string& file_path){
    file_path = this->m_file_path;
}

void VideoWriteNode::setPrefix(std::string prefix){
    this->m_prefix = prefix;
}
void VideoWriteNode::getPrefix(std::string& prefix){
    prefix = this->m_prefix;
}

void VideoWriteNode::setFolder(std::string folder){
    this->m_folder = folder;
    if(!endswith(this->m_folder, "/")){
        this->m_folder = folder +"/";
    }
}
void VideoWriteNode::getFolder(std::string& folder){
    folder = this->m_folder;
}

void VideoWriteNode::writeFinish(AnySignal* out_sig){
#ifndef EAGLEEYE_RKCHIP
    /* flush the encoder */
    encode(m_codec_cxt, NULL, m_pkt, m_output_file);
    /* add sequence end code to have a real mpeg file */
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };
    if (m_encoder->id == AV_CODEC_ID_MPEG1VIDEO || m_encoder->id == AV_CODEC_ID_MPEG2VIDEO)
        m_output_file.write((char*)endcode, sizeof(endcode));

	avcodec_free_context(&m_codec_cxt);    
    av_frame_free(&m_frame);
    av_packet_free(&m_pkt);
    m_codec_cxt = NULL;
#endif

	m_output_file.close();
    if(out_sig != NULL){
        out_sig->meta().is_end_frame = true;
        out_sig->meta().is_start_frame = false;
        out_sig->meta().is_pause_frame = false;
    }

    this->m_is_finish = true;
    this->m_is_header_init = false;
    this->m_manually_stop = 0;
    this->m_manually_start = 0;
    this->m_manually_pause = 0;
    this->m_frame_count = 0;
}

void VideoWriteNode::setStop(int stop){
    if(stop != 0){
        stop = 1;
    }

    this->m_manually_stop = stop;
    this->modified();
}
void VideoWriteNode::getStop(int& stop){
    stop = this->m_manually_stop;
}

void VideoWriteNode::setStart(int start){
    if(start != 0){
        start = 1;
    }

    this->m_manually_start = start;
    this->modified();
}
void VideoWriteNode::getStart(int& start){
    start = this->m_manually_start;
}

void VideoWriteNode::setPause(int pause){
    if(pause != 0){
        pause = 1;
    }

    this->m_manually_pause = pause;
    this->modified();
}
void VideoWriteNode::getPause(int& pause){
    pause = this->m_manually_pause;
}

void VideoWriteNode::setFPS(int fps){
    this->m_fps = fps;
    this->modified();
}
void VideoWriteNode::getFPS(int& fps){
    fps = this->m_fps;
}
} // namespace  eagleeye

#endif