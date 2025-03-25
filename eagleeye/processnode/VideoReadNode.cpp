#include "eagleeye/processnode/VideoReadNode.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/basic/MatrixMath.h"
#include <chrono>

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
#include "libavutil/dict.h"

#include "libavfilter/avfilter.h"
#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"
#include "libavutil/frame.h"
#include <libavcodec/avcodec.h> 
#include <libavformat/avformat.h>
#include "libswscale/swscale.h"
}
namespace eagleeye
{
VideoReadNode::VideoReadNode(){
    // 输出信号
    this->setNumberOfOutputSignals(2);
	this->setOutputPort(this->makeOutputSignal(),0);

    // 设置时间戳端口
    ImageSignal<double>* timestamp_sig = new ImageSignal<double>();
    Matrix<double> timestamp(1,1);
    timestamp_sig->setData(timestamp);
    this->setOutputPort(timestamp_sig,1);
    this->getOutputPort(1)->setSignalType(EAGLEEYE_SIGNAL_TIMESTAMP);

    // // 输入信号
    // this->setNumberOfInputSignals(1);

    this->m_frame_count = 0;
    this->m_frame_total = 0;
    this->m_decoder_finish = false;
    this->m_first_call = true;
    this->m_image_format = 1;
    this->m_is_pull_error = false;

    EAGLEEYE_MONITOR_VAR(std::string, setFilePath, getFilePath, "file","","");
    EAGLEEYE_MONITOR_VAR(int, setFramesNumber, getFramesNumber, "frames", "", "");
}   

VideoReadNode::~VideoReadNode(){
    if(m_sws != nullptr){
        sws_freeContext(m_sws);
        m_sws = nullptr;
    }

    if(m_frame != nullptr){
        av_frame_free(&m_frame);
        m_frame = nullptr;
    }

    if(m_pkt != nullptr){
        av_packet_free(&m_pkt);
        m_pkt = nullptr;
    }

    if(m_cctx != nullptr){
        avcodec_free_context(&m_cctx);
        m_cctx = nullptr;
    }

    if(m_ctx != nullptr){
        avformat_close_input(&m_ctx);
        m_ctx = nullptr;
    }
}

void VideoReadNode::processUnitInfo(){
    if(m_decoder_finish){
        return;
    }
    Superclass::processUnitInfo();
    modified();
}

void VideoReadNode::executeNodeInfo(){
    auto time_to_sleep_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(1)/m_video_fps);
    std::this_thread::sleep_for(time_to_sleep_ms);
    
    if(m_decoder_finish){
        EAGLEEYE_LOGI("video read finish");
        return;
    }
    if(m_first_call){
        if(not initVideoDecoder(m_file_path)){
            EAGLEEYE_LOGE("init video decoder failed");
            return;
        }
        m_first_call = false;
    }
    if(not decoderOneFrame()){
        EAGLEEYE_LOGE("decoder one frame failed");
        return;
    }
    EAGLEEYE_LOGI("frame index  = [%d]", m_frame_count);
    return;
}

void VideoReadNode::setFilePath(std::string file_path){
	m_file_path = file_path;
    this->m_frame_count = 0;
    this->m_frame_total = 0;
    this->m_decoder_finish = false;
    this->m_first_call = true;
    ImageSignal<Array<unsigned char,3>>* output_img_signal = (ImageSignal<Array<unsigned char,3>>*)(this->getOutputPort(0));
    output_img_signal->meta().is_end_frame=false;
    m_next = Matrix<Array<unsigned char, 3>>();
    m_nextnext = Matrix<Array<unsigned char, 3>>();
	//force time to update
	modified();
}

bool VideoReadNode::initVideoDecoder(const std::string& src){
    if(src.empty()){
        return false;
    }
    m_ctx = avformat_alloc_context();
    if (m_ctx == nullptr)
    {
        return false;
    }
    if (auto ret = avformat_open_input(&m_ctx, src.c_str(), nullptr, nullptr); ret != 0)
    {
        EAGLEEYE_LOGE("call avformat_open_input return %d, source = %s", ret, src.c_str());
        return false;
    }

    auto GetFirstStreamByType = [&](enum AVMediaType type)  -> int
    {
        for (int i = 0; i < m_ctx->nb_streams; i++)
        {
            if (m_ctx->streams[i]->codecpar->codec_type == type)
            {
                return i;
            }
        }
        return -1;
    };

    m_streamIdx = GetFirstStreamByType(AVMediaType::AVMEDIA_TYPE_VIDEO);
    if (m_streamIdx < 0)
    {
        EAGLEEYE_LOGE("find AVMEDIA_TYPE_VIDEO failed");
        return false;
    }

    auto GetAVCodecContext = [&](int idx) -> AVCodecContext*
    {
        auto *par = m_ctx->streams[idx]->codecpar;

        const AVCodec *decoder = nullptr;

        decoder = avcodec_find_decoder(par->codec_id);
        if (decoder == nullptr)
        {
            return nullptr;
        }
        auto *cctx = avcodec_alloc_context3(decoder);
        if (cctx == nullptr)
        {
            return nullptr;
        }
        if (avcodec_parameters_to_context(cctx, par) != 0 or avcodec_open2(cctx, decoder, nullptr) != 0)
        {
            avcodec_free_context(&cctx);
            return nullptr;
        }
        return cctx;
    };

    m_cctx = GetAVCodecContext(m_streamIdx);
    if (m_cctx == nullptr)
    {
        EAGLEEYE_LOGE("call GetAVCodecContext return nullptr");
        return false;
    }
    m_cctx->thread_count = m_threadNum;
    m_cctx->thread_type = FF_THREAD_FRAME;
    m_pkt = av_packet_alloc();
    if (m_pkt == nullptr)
    {
        EAGLEEYE_LOGE("call av_packet_alloc return nullptr");
        return false;
    }
    m_frame = av_frame_alloc();
    if (m_frame == nullptr)
    {
        EAGLEEYE_LOGE("call av_frame_alloc return nullptr");
        return false;
    }


    m_width = m_cctx->width;
    m_height = m_cctx->height;

    m_sws = sws_alloc_context();
    if (m_sws == nullptr)
    {
        EAGLEEYE_LOGE("call sws_alloc_context return nullptr");
        return false;
    }

    EAGLEEYE_LOGI("video reader open source = %s success", src.c_str());

    // get fps 
    AVStream* stream = m_ctx->streams[m_streamIdx];
    double fps = av_q2d(stream->avg_frame_rate);
    m_video_fps = fps > 0 ? fps: m_video_fps;
    m_frame_total = stream->nb_frames;

    // get rotation
    auto GetRotation = [&](int idx){
        AVStream *st = m_ctx->streams[idx];
        AVDictionaryEntry *rotate_tag = av_dict_get(st->metadata, "rotate", NULL, 0);
        float theta = 0.0;
    
        if (rotate_tag && *rotate_tag->value && strcmp(rotate_tag->value, "0")) {
            //char *tail;
            //theta = av_strtod(rotate_tag->value, &tail);
            theta = atof(rotate_tag->value);
            // if (*tail)
                // theta = 0;
        }
    
        theta -= 360*floor(theta/360 + 0.9/360);
    
        if (fabs(theta - 90*round(theta/90)) > 2){
            EAGLEEYE_LOGE("Odd ration angle");
        }
        EAGLEEYE_LOGD("got ration = [%d]", theta);
        return theta;
    };

    m_rotation = GetRotation(m_stream_index);
    return true;
}

bool VideoReadNode::decoderOneFrame(){
    while (av_read_frame(m_ctx, m_pkt) == 0)
    {
        std::unique_ptr<AVPacket, decltype(av_packet_unref) *> unref_guard{m_pkt, av_packet_unref};
        if (m_pkt->stream_index != m_streamIdx)
        {
            continue;
        }
        if (auto err_code = avcodec_send_packet(m_cctx, m_pkt); err_code != 0)
        {
            EAGLEEYE_LOGE("can't send packet to decoder, return [%d]", err_code);
            continue;
        }
        if (auto err_code = avcodec_receive_frame(m_cctx, m_frame); err_code != 0)
        {
            if (err_code == AVERROR(EAGAIN))
            {
                continue;
            }
            EAGLEEYE_LOGE("can't recevie frame from decoder, return [%d]", err_code);
        }

        // Only BGR24
        m_sws = sws_getCachedContext(m_sws,
                                       m_frame->width, m_frame->height, static_cast<AVPixelFormat>(m_frame->format),
                                       m_width, m_height, AVPixelFormat::AV_PIX_FMT_BGR24, SWS_BILINEAR, nullptr, nullptr, nullptr);
        //int data_size = av_image_get_buffer_size(AVPixelFormat(frame->format), Width, Height, 1);
        Matrix<Array<unsigned char,3>> frame_rgb_data(m_frame->height, m_frame->width);
        uint8_t* rgb_buffer[1] = {(uint8_t*)frame_rgb_data.dataptr()};
        int rgb_stride[1] = { 3 * m_frame->width};
        sws_scale(m_sws, m_frame->data, m_frame->linesize, 0, m_frame->height, rgb_buffer, rgb_stride);
        ImageSignal<Array<unsigned char,3>>* output_img_signal = 
                    (ImageSignal<Array<unsigned char,3>>*)(this->getOutputPort(0));
        output_img_signal->meta().rotation = m_rotation;
        output_img_signal->setData(frame_rgb_data, output_img_signal->meta());

        ImageSignal<double>* output_timestamp_signal = (ImageSignal<double>*)(this->getOutputPort(1));
        output_timestamp_signal->getData().at(0,0) = m_frame_count++;
        return true;
    }
    return TryFlushing();
}

bool VideoReadNode::TryFlushing(){
    avcodec_send_packet(m_cctx, NULL);
                    
    auto ret = avcodec_receive_frame(m_cctx, m_frame);
    if(ret == AVERROR_EOF)
    {
        m_decoder_finish = true;
        EAGLEEYE_LOGE("Decoder dont need FLUSHING!!!");
        return true;
    }
    else if(ret == 0)
    {
        // Only BGR24
        m_sws = sws_getCachedContext(m_sws,
                                       m_frame->width, m_frame->height, static_cast<AVPixelFormat>(m_frame->format),
                                       m_width, m_height, AVPixelFormat::AV_PIX_FMT_BGR24, SWS_BILINEAR, nullptr, nullptr, nullptr);
        //int data_size = av_image_get_buffer_size(AVPixelFormat(frame->format), Width, Height, 1);
        Matrix<Array<unsigned char,3>> frame_rgb_data(m_frame->height, m_frame->width);
        uint8_t* rgb_buffer[1] = {(uint8_t*)frame_rgb_data.dataptr()};
        int rgb_stride[1] = { 3 * m_frame->width};
        sws_scale(m_sws, m_frame->data, m_frame->linesize, 0, m_frame->height, rgb_buffer, rgb_stride);
        ImageSignal<Array<unsigned char,3>>* output_img_signal = 
                    (ImageSignal<Array<unsigned char,3>>*)(this->getOutputPort(0));
        output_img_signal->meta().rotation = this->m_rotate_degree;
        output_img_signal->setData(frame_rgb_data, output_img_signal->meta());

        ImageSignal<double>* output_timestamp_signal = (ImageSignal<double>*)(this->getOutputPort(1));
        output_timestamp_signal->getData().at(0,0) = m_frame_count++;
    }
    return true;
}

void VideoReadNode::getFilePath(std::string& file_path){
    file_path = this->m_file_path;
}

bool VideoReadNode::finish(){
    Superclass::finish();
    if(!m_next.isempty()){
        this->modified();
        return false;
    }
    return true;
}

void VideoReadNode::setFramesNumber(int num){
    // do nothing
}

void VideoReadNode::getFramesNumber(int& num){
    num = this->m_frame_total;
}

void VideoReadNode::setImageFormat(int image_format){
    // image_format: 0: RGB; 1: BGR, 2: RGBA, 3: BGRA
    if(image_format > 3){
        EAGLEEYE_LOGE("image format only support 0(RGB),1(BGR),2(RGBA),3(BGRA)");
        return;
    }

    this->m_image_format = image_format;
    if(this->m_image_format <= 1){
        this->setOutputPort(new ImageSignal<Array<unsigned char,3>>(),0);
        if(this->m_image_format == 0){
            this->getOutputPort(0)->setSignalType(EAGLEEYE_SIGNAL_RGB_IMAGE);
        }
        else{
            this->getOutputPort(0)->setSignalType(EAGLEEYE_SIGNAL_BGR_IMAGE);
        }
    }
    else{
        this->setOutputPort(new ImageSignal<Array<unsigned char,4>>(),0);
        if(this->m_image_format == 2){
            this->getOutputPort(0)->setSignalType(EAGLEEYE_SIGNAL_RGBA_IMAGE);
        }
        else{
            this->getOutputPort(0)->setSignalType(EAGLEEYE_SIGNAL_BGRA_IMAGE);
        }
    }
}
void VideoReadNode::getImageFormat(int& image_format){
    image_format = m_image_format;
}
} // namespace  eagleeye

#endif