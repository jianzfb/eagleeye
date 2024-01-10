#include "eagleeye/processnode/RTSPReadNode.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/basic/MatrixMath.h"
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
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/opt.h"
#include "libavcodec/avfft.h"
#include "libavutil/imgutils.h"
#include <libavutil/mathematics.h>
#include <libavutil/time.h>
#include <libavutil/samplefmt.h>
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

#define msleep(msec) usleep(msec * 1000)
namespace eagleeye{
RTSPReadNode::RTSPReadNode(){
    // 输出信号
    this->setNumberOfOutputSignals(2);  // image signal, timestamp signal
    EAGLEEYE_MONITOR_VAR(std::string, setFilePath, getFilePath, "rtsp","","");

    m_overtime = "20000";
    m_rtsp_transport = "tcp";   // tcp, udp
    //Find H.264 Decoder
    m_pCodec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if(m_pCodec == NULL){
        printf("Couldn't find Codec.\n");
    }

    m_pCodecCtx = avcodec_alloc_context3(m_pCodec);
    if (!m_pCodecCtx) {
        EAGLEEYE_LOGE("Could not allocate video codec context");
    }
    if(avcodec_open2(m_pCodecCtx, m_pCodec, NULL)<0){
        EAGLEEYE_LOGE("Couldn't open codec.");
    }

    // 0: RGB, 1: BGR, 2: RGBA, 3: BGRA
    // 设置默认图像格式(同步设置0端口数据)
    this->setImageFormat(1);

    // 设置时间戳端口
    this->setOutputPort(new ImageSignal<double>(),1);
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
    m_thread = std::thread(std::bind(&RTSPReadNode::postprocess_by_rga,this));
#endif

#ifndef EAGLEEYE_RKCHIP
    m_thread = std::thread(std::bind(&RTSPReadNode::postprocess_by_libyuv,this));
#endif
}

RTSPReadNode::~RTSPReadNode(){
    avcodec_free_context(&m_pCodecCtx);
    if(m_format_ctx != NULL){
        avformat_close_input(&m_format_ctx);
        avformat_free_context(m_format_ctx);
    }
    this->m_exit_flag = true;

    std::unique_lock<std::mutex> in_locker(this->m_postprocess_mu);
#ifndef EAGLEEYE_RKCHIP
    m_postprocess_queue.push(NULL);
#endif

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
#endif
}

void RTSPReadNode::executeNodeInfo(){
    // 每调用一次，从流中读取一帧
    bool is_found = false;
    AVPacket pkt;

    // 如果解码失败，则重新读取下一针
    unsigned char* ntp_data = NULL;
    double ntp_time;

    while(1){
        while(av_read_frame(m_format_ctx, &pkt) >= 0){
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
                                    msleep(1);
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
                        msleep(3);
                    } while (1);

                    mpp_packet_deinit(&mpp_packet);
#endif

#ifndef EAGLEEYE_RKCHIP
                    // 使用ffmpeg解码         
                    int pkt_done = 0;
                    do {
                        if (!pkt_done) {
                            int ret = avcodec_send_packet(m_pCodecCtx, &pkt);
                            if (ret == 0){
                                pkt_done = 1;
                            }
                        }

                        if (pkt_done)
                            break;

                        msleep(5);
                    } while(1);

                    do{
                        AVFrame* pframe = av_frame_alloc();
                        int ret = avcodec_receive_frame(m_pCodecCtx, pframe);
                        if (ret < 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                            av_frame_free(&pframe);
                            break;
                        }

                        {
                            std::unique_lock<std::mutex> locker(this->m_postprocess_mu);
                            m_postprocess_queue.push(pframe);
                            locker.unlock();
                            m_postprocess_cond.notify_all();
                        }
                    } while(1);
#endif

                    av_packet_unref(&pkt);
                    break;
                }
            }
            av_packet_unref(&pkt);
        }

        std::unique_lock<std::mutex> locker(this->m_out_mu);
        if(this->m_out_queue.size() == 0){
            // 在没有后处理好的数据（颜色空间变换，尺度变换，等），则直接读取下一帧
            locker.unlock();
            continue;
        }

        // 解析时间戳
        RTSPState* rtsp_state = (RTSPState*) m_format_ctx->priv_data;
        RTSPStream* rtsp_stream = rtsp_state->rtsp_streams[m_video_stream_index];
        RTPDemuxContext* rtp_demux_context = (RTPDemuxContext*) rtsp_stream->transport_priv;

        uint32_t seconds = ((rtp_demux_context->first_rtcp_ntp_time >> 32) & 0xffffffff)-2208988800;
        uint32_t fraction  = (rtp_demux_context->first_rtcp_ntp_time & 0xffffffff);
        double useconds = ((double) fraction / 0xffffffff);
        std::pair<unsigned char*, __int64_t> out = this->m_out_queue.front();
        this->m_out_queue.pop();
        ntp_data = out.first;

        double pts = 0;
        if(out.second == AV_NOPTS_VALUE || out.second < 0){
            pts = 0;
        }
        else{
            pts = out.second - (rtp_demux_context->range_start_offset + rtp_demux_context->rtcp_ts_offset);
        }
        ntp_time = seconds + useconds + pts * av_q2d(m_format_ctx->streams[m_video_stream_index]->time_base);
        locker.unlock();
        break;
    }

    // static int count = 0;
    // if(count % 30 == 0){
    //     std::ofstream file_path_handle;
    //     file_path_handle.open("./temp/"+std::to_string(ntp_time)+".png", std::ios::binary);
    //     file_path_handle.write(ntp_data, int(ntp_data.rows()*ntp_data.cols()*3));
    // }
    // count += 1;

    if(this->m_output_image_format <= 1){
        // RGB/BGR
        ImageSignal<Array<unsigned char,3>>* output_img_signal = (ImageSignal<Array<unsigned char,3>>*)(this->getOutputPort(0));
        MetaData output_meta = output_img_signal->meta();
        output_meta.timestamp = ntp_time;
        output_img_signal->setData(
            Matrix<Array<unsigned char,3>>(this->m_image_h,this->m_image_w,ntp_data,false,true), 
            output_meta
        );
    }
    else{
        // RGBA/BGRA
        ImageSignal<Array<unsigned char,4>>* output_img_signal = (ImageSignal<Array<unsigned char,4>>*)(this->getOutputPort(0));
        MetaData output_meta = output_img_signal->meta();
        output_meta.timestamp = ntp_time;
        output_img_signal->setData(
            Matrix<Array<unsigned char,4>>(this->m_image_h,this->m_image_w,ntp_data,false,true), 
            output_meta
        );
    }
}

void RTSPReadNode::postprocess_by_rga(){
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

void RTSPReadNode::postprocess_by_libyuv(){
    unsigned char* y_ptr = NULL;
    unsigned char* u_ptr = NULL;
    unsigned char* v_ptr = NULL;

    Matrix<unsigned char> frame_y;
    Matrix<unsigned char> frame_u;
    Matrix<unsigned char> frame_v;

    int image_w = 0;
    int image_h = 0;    

    do{
        AVFrame* pframe = NULL;
        if(this->m_exit_flag){
            // 退出
            break;
        }

        {
            std::unique_lock<std::mutex> locker(this->m_postprocess_mu);
            if(this->m_postprocess_queue.size() == 0){
                m_postprocess_cond.wait(locker);
                if(this->m_postprocess_queue.size() == 0){
                    continue;
                }
            }

            pframe = this->m_postprocess_queue.front();
            this->m_postprocess_queue.pop();
            if(pframe == NULL){
                continue;
            }
        }

        image_w = pframe->width;
        image_h = pframe->height;
        y_ptr = pframe->data[0];
        u_ptr = pframe->data[1];
        v_ptr = pframe->data[2];

        //yuv to rgb/bgr
        if(this->m_output_image_rotation != 0){
            if(frame_y.numel() != image_h*image_w){
                frame_y = Matrix<unsigned char>(1, int(image_h*image_w));
            }
            if(frame_u.numel() != (image_h*image_w*0.25)){
                frame_u = Matrix<unsigned char>(1, int(image_h*image_w*0.25));
            }
            if(frame_v.numel() != (image_h*image_w*0.25)){
                frame_v = Matrix<unsigned char>(1, int(image_h*image_w*0.25));
            }

            if(this->m_output_image_rotation == 90){
                libyuv::I420Rotate(
                        y_ptr, image_w,
                        u_ptr, (image_w >> 1),
                        v_ptr, (image_w >> 1),
                        frame_y.cpu<unsigned char>(), image_h,
                        frame_u.cpu<unsigned char>(), (image_h >> 1),
                        frame_v.cpu<unsigned char>(), (image_h >> 1),
                        image_w, image_h, libyuv::kRotate90);
                int temp = image_w;
                image_w = image_h;
                image_h = temp;
            }
            else if(this->m_output_image_rotation == 180){
                libyuv::I420Rotate(
                        y_ptr, image_w,
                        u_ptr, (image_w >> 1),
                        v_ptr, (image_w >> 1),
                        frame_y.cpu<unsigned char>(), image_w,
                        frame_u.cpu<unsigned char>(), (image_w >> 1),
                        frame_v.cpu<unsigned char>(), (image_w >> 1),
                        image_w, image_h, libyuv::kRotate180);                        
            }
            else if(this->m_output_image_rotation == 270){
                libyuv::I420Rotate(
                        y_ptr, image_w,
                        u_ptr, (image_w >> 1),
                        v_ptr, (image_w >> 1),
                        frame_y.cpu<unsigned char>(), image_h,
                        frame_u.cpu<unsigned char>(), (image_h >> 1),
                        frame_v.cpu<unsigned char>(), (image_h >> 1),
                        image_w, image_h, libyuv::kRotate270);
                int temp = image_w;
                image_w = image_h;
                image_h = temp;                        
            }
        }
        else{
            frame_y = Matrix<unsigned char>(1, int(image_h*image_w), y_ptr);
            frame_u = Matrix<unsigned char>(1, int(image_h*image_w*0.25), u_ptr);
            frame_v = Matrix<unsigned char>(1, int(image_h*image_w*0.25), v_ptr);
        }

        unsigned char* decode_image = NULL;
        if(this->m_output_image_format <= 1){
            decode_image = new unsigned char[image_h*image_w*3];
        }
        else{
            decode_image = new unsigned char[image_h*image_w*4];
        }
        if(this->m_output_image_format == 0){
            // to RGB
            libyuv::I420ToRAW(frame_y.cpu<unsigned char>(), image_w, 
                                frame_u.cpu<unsigned char>(), (image_w>>1),
                                frame_v.cpu<unsigned char>(), (image_w>>1),
                                decode_image, image_w*3,
                                image_w,
                                image_h);
        }
        else if(this->m_output_image_format == 1){
            // to BGR
            libyuv::I420ToRGB24(frame_y.cpu<unsigned char>(), image_w, 
                                frame_u.cpu<unsigned char>(), (image_w>>1),
                                frame_v.cpu<unsigned char>(), (image_w>>1),
                                decode_image, image_w*3,
                                image_w,
                                image_h);         
        }
        else if(this->m_output_image_format == 2){
            // to RGBA
            libyuv::I420ToRGBA(
                frame_y.cpu<unsigned char>(), image_w, 
                frame_u.cpu<unsigned char>(), (image_w>>1),
                frame_v.cpu<unsigned char>(), (image_w>>1),
                decode_image, image_w*4,
                image_w, image_h); 
        }
        else{
            // to BGRA
            libyuv::I420ToBGRA(
                frame_y.cpu<unsigned char>(), image_w, 
                frame_u.cpu<unsigned char>(), (image_w>>1),
                frame_v.cpu<unsigned char>(), (image_w>>1),
                decode_image, image_w*4,
                image_w, image_h);            
        }

        this->m_image_h = image_h;
        this->m_image_w = image_w;
        {
            std::unique_lock<std::mutex> locker(this->m_out_mu);
            if(this->m_max_queue_size > 0 && this->m_out_queue.size() > this->m_max_queue_size){
                std::pair<unsigned char*, __int64_t> out = this->m_out_queue.front();
                delete[] out.first;
                this->m_out_queue.pop();
            }

            this->m_out_queue.push(std::make_pair(decode_image, pframe->pts));
            locker.unlock();
            m_out_cond.notify_all();
        }

        av_frame_free(&pframe);
    } while(1);
}

void RTSPReadNode::processUnitInfo(){
    Superclass::processUnitInfo();
    modified();
}

void RTSPReadNode::setFilePath(std::string file_path){
    if(m_format_ctx != NULL){
        avformat_free_context(m_format_ctx);
        m_format_ctx = NULL;
    }

    m_format_ctx = avformat_alloc_context();
    this->m_rtsp_address = file_path;
    int ret = -1;
    AVDictionary* format_opts = NULL;
    av_dict_set(&format_opts, "stimeout", m_overtime.c_str(), 0); //设置链接超时时间（us）
    av_dict_set(&format_opts, "rtsp_transport", m_rtsp_transport.c_str(), 0); //设置推流的方式，默认udp。
    av_dict_set(&format_opts, "max_analyze_duration", "10", 0);
    av_dict_set(&format_opts, "probesize", "2048", 0);
    ret = avformat_open_input(&m_format_ctx, file_path.c_str(), nullptr, &format_opts);
    if (ret != 0) {
        EAGLEEYE_LOGE("Fail to open url: %s, return value: %d", file_path.c_str(), ret);
        return;
    }
    ret = avformat_find_stream_info(m_format_ctx, nullptr);
    if (ret < 0) {
        EAGLEEYE_LOGE("Fail to get stream information: %d", ret);
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
        }
    }
    if (m_video_stream_index == -1) {
        EAGLEEYE_LOGE("no video stream");
        return;
    }
}

void RTSPReadNode::getFilePath(std::string& file_path){
    file_path = this->m_rtsp_address;
}

void RTSPReadNode::setImageFormat(int image_format){
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

void RTSPReadNode::setImageRotation(int image_rotation){
    // image_format: 0,90,180,270
    if(image_rotation != 0 && image_rotation != 90 && image_rotation != 180 && image_rotation != 270){
        return;
    }
    this->m_output_image_rotation = image_rotation;
}

void RTSPReadNode::setRTSPTransport(std::string rtsp_transport){
    m_rtsp_transport = rtsp_transport;
}

void RTSPReadNode::setOvertime(std::string overtime){
    m_overtime = overtime;
}

void RTSPReadNode::setMaxQueue(int size){
    this->m_max_queue_size = size;
}
}
#endif