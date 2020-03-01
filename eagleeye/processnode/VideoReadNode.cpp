#include "eagleeye/processnode/VideoReadNode.h"
#include "eagleeye/common/EagleeyeLog.h"

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
#include <libavcodec/avcodec.h> 
#include <libavformat/avformat.h>
#include "libswscale/swscale.h"
}
namespace eagleeye
{
VideoReadNode::VideoReadNode(){
    // 输出信号
    this->setNumberOfOutputSignals(1);
	this->setOutputPort(this->makeOutputSignal(),0);

    // // 输入信号
    // this->setNumberOfInputSignals(1);

    this->m_frame_count = 0;
    this->m_frame_total = 0;
    this->m_isover = false;
    this->m_first_call = true;

    EAGLEEYE_MONITOR_VAR(std::string, setFilePath, getFilePath, "file","","");
}   

VideoReadNode::~VideoReadNode(){
    if(m_avf_cxt != NULL){
        avformat_free_context(m_avf_cxt);
        m_avf_cxt = NULL;
    }
    if(m_avc_cxt != NULL){
        avcodec_close(m_avc_cxt);
        m_avc_cxt = NULL;
    }
}

void VideoReadNode::executeNodeInfo(){
    ImageSignal<Array<unsigned char,3>>* output_img_signal = (ImageSignal<Array<unsigned char,3>>*)(this->getOutputPort(0));
    if(!m_next.isempty()){
        output_img_signal->setData(m_next);
        m_next = Matrix<Array<unsigned char,3>>();
    }

    // 0.step 解析视频
    if(this->m_frame_total == 0){
        // 解析新视频
        if(m_avf_cxt != NULL){
            avformat_free_context(m_avf_cxt);
            m_avf_cxt = NULL;
        }
        if(m_avc_cxt != NULL){
            avcodec_close(m_avc_cxt);
            m_avc_cxt = NULL;
        }

        m_avf_cxt = avformat_alloc_context();
        int ret = avformat_open_input(&m_avf_cxt,m_file_path.c_str(),NULL,NULL);
        if(ret < 0){
            EAGLEEYE_LOGD("couldnt open video file");
            avformat_free_context(m_avf_cxt);
            m_avf_cxt = NULL;
            return;
        }

        ret = avformat_find_stream_info(m_avf_cxt,NULL);
        if(ret < 0){
            EAGLEEYE_LOGD("couldnt find stream");
            avformat_free_context(m_avf_cxt);
            m_avf_cxt = NULL;
            return;
        }

        // 打印视频信息
        av_dump_format(m_avf_cxt,0,m_file_path.c_str(),0);
        m_stream_index = -1;
        for(int i = 0 ; i < m_avf_cxt->nb_streams; i++){
            if(m_avf_cxt->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO){
                m_stream_index = i;
                break;
            }
        }
        if(m_stream_index == -1){
            EAGLEEYE_LOGD("couldnt find stream");
            avformat_free_context(m_avf_cxt);
            m_avf_cxt = NULL;
            return;
        }

        m_video_fps = av_q2d(m_avf_cxt->streams[m_stream_index]->r_frame_rate);
        m_frame_total = m_avf_cxt->streams[m_stream_index]->nb_frames;
        EAGLEEYE_LOGD("video total frames %d with FPS %f", m_frame_total, (float)(m_video_fps));
        m_avc_cxt = m_avf_cxt->streams[m_stream_index]->codec;
        enum AVCodecID codecId = m_avc_cxt->codec_id;
        AVCodec* codec = avcodec_find_decoder(codecId);
        if(!codec){
            EAGLEEYE_LOGD("couldnt find decoder");
            avcodec_close(m_avc_cxt);
            avformat_free_context(m_avf_cxt);
            m_avc_cxt = NULL;
            m_avf_cxt = NULL;
            return;
        }
    
        ret = avcodec_open2(m_avc_cxt,codec,NULL);
        if(ret < 0){
            EAGLEEYE_LOGD("decoder dont work");
            avcodec_close(m_avc_cxt);
            avformat_free_context(m_avf_cxt);
            m_avc_cxt = NULL;
            m_avf_cxt = NULL;
            return;
        }

        output_img_signal->fps = this->m_video_fps;
        output_img_signal->nb_frames = this->m_frame_total;
    }

    // 1.step 逐帧解析
    //为avpacket分配内存
    AVPacket* packet = av_packet_alloc();
    //为avFrame分配内存
    AVFrame* frame = av_frame_alloc();

    int iterator_count = 1;
    if(this->m_first_call){
        iterator_count = 2;
    }
    for(int index=0; index<iterator_count; ++index){
        bool is_finding = false;
        while(av_read_frame(m_avf_cxt, packet) >= 0){
            if(packet && packet->stream_index == m_stream_index){
                int gotFrame = 0;
                int ret = avcodec_decode_video2(m_avc_cxt, frame, &gotFrame, packet);
                if(gotFrame == 0){
                    continue;
                }
                is_finding = true;
                break;
            }
        }

        if(is_finding){
            // 发现视频帧
            SwsContext* swsContext = sws_getContext(frame->width, frame->height, AV_PIX_FMT_YUV420P,frame->width, frame->height, AV_PIX_FMT_RGB24,
                                NULL, NULL, NULL, NULL);

            int linesize[8] = {frame->linesize[0] * 3};
            int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_BGR24, frame->width, frame->height, 1);
            uint8_t* p_global_bgr_buffer = (uint8_t*) malloc(num_bytes * sizeof(uint8_t));
            uint8_t* bgr_buffer[8] = {p_global_bgr_buffer};
            
            sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height, bgr_buffer, linesize);
            Matrix<Array<unsigned char,3>> frame_rgb_data(frame->height, frame->width, (void*)bgr_buffer[0], true);
            if(index==0 && iterator_count==2){
                // 首次调用,直接输出赋值
                output_img_signal->setData(frame_rgb_data);
            }
            else{
                // 每次调用，获得下次数据
                m_next = frame_rgb_data;
            }

            sws_freeContext(swsContext);
            free(p_global_bgr_buffer);
        }
        else{
            // 没有发现
            m_next = Matrix<Array<unsigned char,3>>();
        }
    }

    if(m_next.isempty()){
        this->m_isover = true;
        output_img_signal->is_final=true;
    }
    av_packet_free(&packet);
    av_frame_free(&frame);
    this->m_frame_count += 1;
    this->m_first_call = false;
}

void VideoReadNode::setFilePath(std::string file_path)
{
	m_file_path = file_path;
    this->m_frame_count = 0;
    this->m_frame_total = 0;
    this->m_isover = false;
    this->m_first_call = true;
    ImageSignal<Array<unsigned char,3>>* output_img_signal = (ImageSignal<Array<unsigned char,3>>*)(this->getOutputPort(0));
    output_img_signal->is_final=false;
	//force time to update
	modified();
}
void VideoReadNode::getFilePath(std::string& file_path){
    file_path = this->m_file_path;
}

bool VideoReadNode::selfcheck(){
    return !this->m_isover;
}

void VideoReadNode::feadback(std::map<std::string, int>& node_state_map){
    // 1.step call base feadback
    Superclass::feadback(node_state_map);

    // 2.step whether need to update
    if(!m_next.isempty()){
        this->modified();
    }
}

} // namespace  eagleeye

#endif