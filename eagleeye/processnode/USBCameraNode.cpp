#include "eagleeye/processnode/USBCameraNode.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/common/EagleeyeTime.h"
#include "libyuv.h"
#include <stdio.h>
#include <unistd.h>
#include <fstream>
#ifdef EAGLEEYE_FFMPEG
extern "C"
{
#include "libavformat/rtsp.h"
#include "libavformat/rtpdec.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/opt.h"
#include "libavcodec/avfft.h"
#include "libavutil/imgutils.h"
#include "libavutil/mathematics.h"
#include "libavutil/time.h"
#include "libavutil/samplefmt.h"
#include "libavutil/hwcontext.h"
#include "libavutil/buffer.h"
}

#ifdef EAGLEEYE_RKCHIP
#include "im2d_version.h"
#include "rk_type.h"
#include "rk_mpi.h"
#include "mpp_log.h"
#include "mpp_frame.h"
#include "mpp_err.h"
#include "RgaUtils.h"
#include "im2d_buffer.h"
#include "im2d_type.h"
#include "im2d_single.h"
#endif

namespace eagleeye{
USBCameraNode::USBCameraNode(){
    m_is_camera_open = false;
    m_is_pull_error = false;
    m_is_init_error = false;

    EAGLEEYE_MONITOR_VAR(std::string, setCameraId, getCameraId, "camera_id","","");

    // 0: RGB, 1: BGR, 2: RGBA, 3: BGRA
    // 设置默认图像格式(同步设置0端口数据)
    this->setImageFormat(1);

    // 设置时间戳端口
    this->setNumberOfOutputSignals(2);  // image signal, timestamp signal
    ImageSignal<double>* timestamp_sig = new ImageSignal<double>();
    Matrix<double> timestamp(1,1);
    timestamp_sig->setData(timestamp);
    this->setOutputPort(timestamp_sig,1);
    this->getOutputPort(1)->setSignalType(EAGLEEYE_SIGNAL_TIMESTAMP);

    // 0,90,180,270
    // 设置默认图像旋转
    this->setImageRotation(0);

    m_mpp_ctx = NULL;
    m_mpp_api = NULL;
    this->m_exit_flag = false;
    this->m_max_queue_size = 0;

    m_format_ctx = NULL;
#ifdef EAGLEEYE_RKCHIP
    MppCtx mpp_ctx;
    MppApi* mpp_api;
    MPP_RET mpp_ret = mpp_create(&mpp_ctx, &mpp_api);
    if (MPP_OK != mpp_ret) {
        EAGLEEYE_LOGE("MPP mpp_create failure.");
    }
    // 不分帧模式
    RK_U32 need_split = 1;
    mpp_ret = mpp_api->control(mpp_ctx, MPP_DEC_SET_PARSER_SPLIT_MODE, (MppParam*)&need_split);
    if (mpp_ret != MPP_OK) {
        EAGLEEYE_LOGE("MPP control MPP_DEC_SET_PARSER_SPLIT_MODE failure.");
    }
    mpp_ret = mpp_api->control(mpp_ctx, MPP_SET_INPUT_BLOCK, (MppParam*)&need_split);
    if (mpp_ret != MPP_OK) {
        EAGLEEYE_LOGE("MPP control MPP_SET_INPUT_BLOCK failure.");
    }

    mpp_ret = mpp_init(mpp_ctx, MPP_CTX_DEC, MPP_VIDEO_CodingAVC);
    if (mpp_ret != MPP_OK) {
        EAGLEEYE_LOGE("MPP mpp_init failure.");
    }

    MppDecCfg cfg       = NULL;
    mpp_dec_cfg_init(&cfg);
    /* get default config from decoder context */
    mpp_ret = mpp_api->control(mpp_ctx, MPP_DEC_GET_CFG, cfg);
    if (mpp_ret) {
        EAGLEEYE_LOGE("failed to get decoder cfg ret %d\n", mpp_ret);
    }

    /*
     * split_parse is to enable mpp internal frame spliter when the input
     * packet is not aplited into frames.
     */
    mpp_ret = mpp_dec_cfg_set_u32(cfg, "base:split_parse", need_split);
    if (mpp_ret) {
        EAGLEEYE_LOGE("failed to set split_parse ret %d\n", mpp_ret);
    }

    mpp_ret = mpp_api->control(mpp_ctx, MPP_DEC_SET_CFG, cfg);
    if (mpp_ret) {
        EAGLEEYE_LOGE("failed to set cfg %p ret %d\n", cfg, mpp_ret);
    }

    m_mpp_ctx = mpp_ctx;
    m_mpp_api = mpp_api;
    m_frm_grp = NULL;
    // 负责解码+颜色空间转换线程
    m_thread = std::thread(std::bind(&USBCameraNode::postprocess_by_rga,this));
#endif
}

USBCameraNode::~USBCameraNode(){
    this->m_exit_flag = true;
    std::unique_lock<std::mutex> in_locker(this->m_postprocess_mu);

#ifdef EAGLEEYE_RKCHIP
    m_postprocess_mpp_queue.push(NULL);
#endif
    in_locker.unlock();
    m_postprocess_cond.notify_all();

    if(m_thread.joinable()){
        m_thread.join();
    }

    // 输出队列中数据
    std::unique_lock<std::mutex> out_locker(this->m_out_mu);
    while(this->m_out_queue.size() > 0){
        std::pair<unsigned char*, __int64_t> out = this->m_out_queue.front();
        delete[] out.first;
        this->m_out_queue.pop();
    }
    out_locker.unlock();

    if(m_pCodecCtx != NULL){
        avcodec_free_context(&m_pCodecCtx);
        m_pCodecCtx = NULL;
    }
    if(m_format_ctx != NULL){
        avformat_close_input(&m_format_ctx);
        m_format_ctx = NULL;
    }

#ifdef EAGLEEYE_RKCHIP
    // 销毁RK资源
    if (m_frm_grp != NULL) {
        mpp_buffer_group_put(m_frm_grp);
        m_frm_grp = NULL;
    }

    MppCtx mpp_ctx = (MppCtx)m_mpp_ctx;
    MppApi* mpp_api = (MppApi*)m_mpp_api;
    mpp_api->reset(mpp_ctx);
    mpp_destroy(mpp_ctx);

    while (true)
    {
        MppFrame mpp_frame = NULL;
        {
            std::unique_lock<std::mutex> locker(this->m_postprocess_mu);
            if(this->m_postprocess_mpp_queue.size() == 0){
                break;
            }

            void* t = this->m_postprocess_mpp_queue.front();
            this->m_postprocess_mpp_queue.pop();
            if(t == NULL){
                continue;
            }
            mpp_frame = (MppFrame)t;
        }
        mpp_frame_deinit(&mpp_frame);
    }
    
#endif
}

void USBCameraNode::postprocess_by_rga(){
#ifdef EAGLEEYE_RKCHIP
    do{
        MppFrame mpp_frame = NULL;
        if(this->m_exit_flag){
            // 退出
            break;
        }

        {
            std::unique_lock<std::mutex> locker(this->m_postprocess_mu);
            if(this->m_postprocess_mpp_queue.size() == 0){
                m_postprocess_cond.wait(locker);
                if(this->m_postprocess_mpp_queue.size() == 0){
                    continue;
                }
            }

            void* t = this->m_postprocess_mpp_queue.front();
            this->m_postprocess_mpp_queue.pop();
            if(t == NULL){
                continue;
            }
            mpp_frame = (MppFrame)t;
        }

        // 转换到颜色空间
        RK_U32 src_width    = mpp_frame_get_width(mpp_frame);
        RK_U32 src_height   = mpp_frame_get_height(mpp_frame);
        RK_U32 src_h_stride = mpp_frame_get_hor_stride(mpp_frame);
        RK_U32 src_v_stride = mpp_frame_get_ver_stride(mpp_frame);
        MppBuffer src_buf   = mpp_frame_get_buffer(mpp_frame);
        RK_S64 pts = mpp_frame_get_pts(mpp_frame);

        int src_format = RK_FORMAT_YCbCr_420_SP;
        int src_buf_size = src_width * src_height * get_bpp_from_format(src_format);
        rga_buffer_t src_img, dst_img;
        rga_buffer_handle_t src_handle, dst_handle;
        memset(&src_img, 0, sizeof(src_img));
        memset(&dst_img, 0, sizeof(dst_img));

        int dst_width, dst_height, dst_format;
        dst_width = src_width;
        dst_height = src_height;
        if(m_output_image_format == 0){
            // RGB
            dst_format = RK_FORMAT_RGB_888;
        }
        else if(m_output_image_format == 1){
            // BGR
            dst_format = RK_FORMAT_BGR_888;
        }
        else if(m_output_image_format == 2){
            // RGBA
            dst_format = RK_FORMAT_RGBA_8888;
        }
        else{
            // BGRA
            dst_format = RK_FORMAT_BGRA_8888;
        }

        int dst_buf_size = dst_width * dst_height * get_bpp_from_format(dst_format);
        unsigned char* dst_buf = new unsigned char[dst_buf_size];
        src_handle = importbuffer_virtualaddr(mpp_buffer_get_ptr(src_buf), src_buf_size);
        dst_handle = importbuffer_virtualaddr(dst_buf, dst_buf_size);

        src_img = wrapbuffer_handle(src_handle, src_width, src_height, src_format);
        dst_img = wrapbuffer_handle(dst_handle, dst_width, dst_height, dst_format);
        imcvtcolor(src_img, dst_img, src_format, dst_format);
        if (src_handle)
            releasebuffer_handle(src_handle);
        if (dst_handle)
            releasebuffer_handle(dst_handle);

        this->m_image_h = dst_height;
        this->m_image_w = dst_width;
        {
            std::unique_lock<std::mutex> locker(this->m_out_mu);
            if(this->m_max_queue_size > 0 && this->m_out_queue.size() > this->m_max_queue_size){
                std::pair<unsigned char*, __int64_t> out = this->m_out_queue.front();
                delete[] out.first;
                this->m_out_queue.pop();
            }

            this->m_out_queue.push(std::make_pair(dst_buf, pts));
            locker.unlock();
            m_out_cond.notify_all();
        }

        mpp_frame_deinit(&mpp_frame);
    } while(1);
#endif
}


void USBCameraNode::executeNodeInfo(){
    if(m_is_pull_error){
        EAGLEEYE_LOGE("pull stream error, skip pull process.");
        m_is_pull_error = false;
        setCameraId(m_camera_id);
        return;
    }

    // 每调用一次，从流中读取一帧
    bool is_found = false;
    AVPacket pkt;

    // 如果解码失败，则重新读取下一针
    unsigned char* ntp_data = NULL;
    double ntp_time;

    while(1){
        bool is_read_frame_ok = false;
        while(av_read_frame(m_format_ctx, &pkt) >= 0){
            // 设置读取成功标记
            is_read_frame_ok = true;
            // 解析图像帧
            if (pkt.stream_index == m_video_stream_index) {
                // Decode AVPacket to Frame
                if(pkt.size){
#ifdef EAGLEEYE_RKCHIP
                    // 使用RK硬件解码
                    MppCtx mpp_ctx = (MppCtx)m_mpp_ctx;
                    MppApi* mpp_api = (MppApi*)m_mpp_api;
                    MppPacket mpp_packet = NULL;
                    mpp_packet_init(&mpp_packet, pkt.data, pkt.size);
                    mpp_packet_set_pts(mpp_packet, pkt.pts);
                    mpp_packet_set_dts(mpp_packet, pkt.dts);
                    RK_U32 pkt_done = 0;
                    RK_U32 ret = 0;
                    do {
                        // send the packet first if packet is not done
                        RK_S32 times = 5;
                        if (!pkt_done) {
                            ret = mpp_api->decode_put_packet(mpp_ctx, mpp_packet);
                            if (MPP_OK == ret){
                                pkt_done = 1;
                            }
                        }

                        do{
                            MppFrame mpp_frame = NULL;
                            ret = mpp_api->decode_get_frame(mpp_ctx, &mpp_frame);
                            if (ret == MPP_ERR_TIMEOUT) {
                                if(times > 0){
                                    times --;
                                    usleep(1 * 1000);
                                    continue;
                                }
                            }
                            if(mpp_frame == NULL || ret != MPP_OK){
                                break;
                            }

                            if (mpp_frame_get_info_change(mpp_frame)) {
                                // 首次解码会进入这里
                                RK_U32 width = mpp_frame_get_width(mpp_frame);
                                RK_U32 height = mpp_frame_get_height(mpp_frame);
                                RK_U32 hor_stride = mpp_frame_get_hor_stride(mpp_frame);
                                RK_U32 ver_stride = mpp_frame_get_ver_stride(mpp_frame);
                                RK_U32 buf_size = mpp_frame_get_buf_size(mpp_frame);

                                EAGLEEYE_LOGD("decode_get_frame get info changed found");
                                EAGLEEYE_LOGD("decoder require buffer w:h [%d:%d] stride [%d:%d] buf_size %d",
                                        width, height, hor_stride, ver_stride, buf_size);

                                if (NULL == this->m_frm_grp) {
                                    /* If buffer group is not set create one and limit it */
                                    ret = mpp_buffer_group_get_internal(&this->m_frm_grp, MPP_BUFFER_TYPE_ION);
                                    if (ret) {
                                        EAGLEEYE_LOGE("get mpp buffer group failed ret %d", ret);
                                        break;
                                    }

                                    /* Set buffer to mpp decoder */
                                    ret = mpp_api->control(mpp_ctx, MPP_DEC_SET_EXT_BUF_GROUP, this->m_frm_grp);
                                    if (ret) {
                                        EAGLEEYE_LOGE("set buffer group failed ret %d", ret);
                                        break;
                                    }
                                } else {
                                    /* If old buffer group exist clear it */
                                    ret = mpp_buffer_group_clear(this->m_frm_grp);
                                    if (ret) {
                                        EAGLEEYE_LOGE("clear buffer group failed ret %d", ret);
                                        break;
                                    }
                                }

                                /* Use limit config to limit buffer count to 24 with buf_size */
                                ret = mpp_buffer_group_limit_config(this->m_frm_grp, buf_size, 24);
                                if (ret) {
                                    EAGLEEYE_LOGE("limit buffer group failed ret %d", ret);
                                    break;
                                }

                                mpp_api->control(mpp_ctx, MPP_DEC_SET_INFO_CHANGE_READY, NULL);
                                mpp_frame_deinit(&mpp_frame);
                                continue;
                            }

                            RK_U32 err_info = mpp_frame_get_errinfo(mpp_frame) | mpp_frame_get_discard(mpp_frame);
                            if (err_info) {
                                // 解码错误
                                EAGLEEYE_LOGE("decoder_get_frame get err info:%d discard:%d.\n",
                                        mpp_frame_get_errinfo(mpp_frame), mpp_frame_get_discard(mpp_frame));
                                mpp_frame_deinit(&mpp_frame);
                                continue;
                            }

                            // color space transform
                            {
                                std::unique_lock<std::mutex> locker(this->m_postprocess_mu);
                                m_postprocess_mpp_queue.push(mpp_frame);
                                locker.unlock();
                                m_postprocess_cond.notify_all();
                            }
                        } while(1);

                        if (pkt_done)
                            break;

                        /*
                        * why sleep here:
                        * mpi->decode_put_packet will failed when packet in internal queue is
                        * full,waiting the package is consumed .Usually hardware decode one
                        * frame which resolution is 1080p needs 2 ms,so here we sleep 3ms
                        * * is enough.
                        */
                        usleep(3 * 1000);
                    } while (1);

                    mpp_packet_deinit(&mpp_packet);
#endif

                    av_packet_unref(&pkt);
                    break;
                }
            }
            av_packet_unref(&pkt);

            // 重置读取成功标记
            is_read_frame_ok = false;
        }
        if(!is_read_frame_ok){
            // 读取帧异常，直接返回
            EAGLEEYE_LOGE("Reading frame from stream abnormal.");
            m_is_pull_error = true;
            return;
        }

        std::unique_lock<std::mutex> locker(this->m_out_mu);
        if(this->m_out_queue.size() == 0){
            // 在没有后处理好的数据（颜色空间变换，尺度变换，等），则直接读取下一帧
            locker.unlock();
            continue;
        }

        std::pair<unsigned char*, __int64_t> out = this->m_out_queue.front();
        this->m_out_queue.pop();
        locker.unlock();
        break;
    }

    if(this->m_output_image_format <= 1){
        // RGB/BGR
        ImageSignal<Array<unsigned char,3>>* output_img_signal = (ImageSignal<Array<unsigned char,3>>*)(this->getOutputPort(0));
        MetaData output_meta = output_img_signal->meta();
        output_meta.timestamp = 0;
        output_img_signal->setData(
            Matrix<Array<unsigned char,3>>(this->m_image_h,this->m_image_w,ntp_data,false,true), 
            output_meta
        );
    }
    else{
        // RGBA/BGRA
        ImageSignal<Array<unsigned char,4>>* output_img_signal = (ImageSignal<Array<unsigned char,4>>*)(this->getOutputPort(0));
        MetaData output_meta = output_img_signal->meta();
        output_meta.timestamp = 0;
        output_img_signal->setData(
            Matrix<Array<unsigned char,4>>(this->m_image_h,this->m_image_w,ntp_data,false,true), 
            output_meta
        );
    }

    ImageSignal<double>* output_timestamp_signal = (ImageSignal<double>*)(this->getOutputPort(1));
    output_timestamp_signal->getData().at(0,0) = 0;
}

void USBCameraNode::processUnitInfo(){
    Superclass::processUnitInfo();
    modified();
}

void USBCameraNode::setCameraId(std::string camera_id){
    if(m_format_ctx != NULL){
        avformat_free_context(m_format_ctx);
        m_format_ctx = NULL;
    }
    m_format_ctx = avformat_alloc_context();

    const AVInputFormat *pInputFormat = av_find_input_format("video4linux2"); // 或者其他与你的摄像头相匹配的格式
    if(pInputFormat == NULL){
        EAGLEEYE_LOGE("Fail find v4l2");
        m_is_pull_error = true;        
        return;
    }
    AVDictionary *options = nullptr;
 
    // 设置设备名称，这里假设是 /dev/video0
    av_dict_set(&options, "video_size", "640x480", 0);
    av_dict_set(&options, "pixel_format", "yuv420p", 0);
    av_dict_set(&options, "framerate", "30", 0);

    int ret = avformat_open_input(&m_format_ctx, camera_id.c_str(), pInputFormat, &options);
    if (ret != 0) {
        // 打开设备失败
        EAGLEEYE_LOGE("Fail to open %s, return value %d", camera_id.c_str(), ret);
        m_is_pull_error = true;
        return;
    }
    
    ret = avformat_find_stream_info(m_format_ctx, nullptr);
    if (ret < 0) {
        // 获取流信息失败
        EAGLEEYE_LOGE("Fail to get stream information %s, return value %d", ret);
        m_is_pull_error = true;
        return;
    }

    // audio/video stream index
    m_video_stream_index = -1;
    EAGLEEYE_LOGD("Number of elements in AVFormatContext.streams: %d", m_format_ctx->nb_streams);
	for (int i = 0; i < m_format_ctx->nb_streams; ++i) {
        const AVStream* stream = m_format_ctx->streams[i];
        EAGLEEYE_LOGD("type of the encoded data: %d", stream->codecpar->codec_id);
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            m_video_stream_index = i;
            EAGLEEYE_LOGD("Dimensions of the video frame in pixels: width: %d, height: %d, pixel format: %d", stream->codecpar->width, stream->codecpar->height, stream->codecpar->format);
            break;
        }
    }

    if (m_video_stream_index == -1) {
        EAGLEEYE_LOGE("no video stream");
        this->m_is_pull_error = true;
        return;
    }

    AVCodecParameters *par = m_format_ctx->streams[m_video_stream_index]->codecpar;
    m_pCodec = avcodec_find_decoder(par->codec_id);
    if (!m_pCodec) {
        // 没有找到解码器
        EAGLEEYE_LOGE("Couldnt find decoder.");
        return;
    }

    m_pCodecCtx = avcodec_alloc_context3(m_pCodec);
    avcodec_parameters_to_context(m_pCodecCtx, par);
    if (avcodec_open2(m_pCodecCtx, m_pCodec, nullptr) < 0) {
        // 打开解码器失败
        EAGLEEYE_LOGE("Couldnt open decoder.");
        return;
    }
 
    m_camera_id = camera_id;
    modified();
}

void USBCameraNode::getCameraId(std::string& camera_id){
    camera_id = m_camera_id; 
}

void USBCameraNode::setImageFormat(int image_format){
    // image_format: 0: RGB; 1: BGR, 2: RGBA, 3: BGRA
    if(image_format > 3){
        EAGLEEYE_LOGE("image format only support 0(RGB),1(BGR),2(RGBA),3(BGRA)");
        return;
    }

    this->m_output_image_format = image_format;
    if(this->m_output_image_format <= 1){
        this->setOutputPort(new ImageSignal<Array<unsigned char,3>>(),0);
        if(this->m_output_image_format == 0){
            this->getOutputPort(0)->setSignalType(EAGLEEYE_SIGNAL_RGB_IMAGE);
        }
        else{
            this->getOutputPort(0)->setSignalType(EAGLEEYE_SIGNAL_BGR_IMAGE);
        }
    }
    else{
        this->setOutputPort(new ImageSignal<Array<unsigned char,4>>(),0);
        if(this->m_output_image_format == 2){
            this->getOutputPort(0)->setSignalType(EAGLEEYE_SIGNAL_RGBA_IMAGE);
        }
        else{
            this->getOutputPort(0)->setSignalType(EAGLEEYE_SIGNAL_BGRA_IMAGE);
        }
    }
}

void USBCameraNode::setImageRotation(int image_rotation){
    // image_format: 0,90,180,270
    if(image_rotation != 0 && image_rotation != 90 && image_rotation != 180 && image_rotation != 270){
        return;
    }
    this->m_output_image_rotation = image_rotation;
}

}
#endif