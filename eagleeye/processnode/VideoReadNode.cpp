#include "eagleeye/processnode/VideoReadNode.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/basic/MatrixMath.h"

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
    this->setNumberOfOutputSignals(1);
	this->setOutputPort(this->makeOutputSignal(),0);

    // // 输入信号
    // this->setNumberOfInputSignals(1);

    this->m_frame_count = 0;
    this->m_frame_total = 0;
    this->m_decoder_finish = false;
    this->m_first_call = true;

    m_avf_cxt = NULL;
    m_avc_cxt = NULL;
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
    if((m_decoder_finish || m_first_call) && this->getNumberOfInputSignals() > 0){
        StringSignal* input_sig = (StringSignal*)(this->getInputPort(0));
        std::string video_path = input_sig->getData();
        this->setFilePath(video_path);
    }

    ImageSignal<Array<unsigned char,3>>* output_img_signal = 
                    (ImageSignal<Array<unsigned char,3>>*)(this->getOutputPort(0));
    if(!m_next.isempty()){
        output_img_signal->setData(m_next);
        m_next = m_nextnext;
        m_nextnext = Matrix<Array<unsigned char,3>>();

        if(m_next.isempty()){
            m_decoder_finish = true;
        }
    }
    if(m_decoder_finish){
        output_img_signal->meta()->is_end_frame = false;
        return;
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

        output_img_signal->meta()->fps = this->m_video_fps;
        output_img_signal->meta()->nb_frames = this->m_frame_total;

        AVDictionaryEntry *tag = NULL;
        tag = av_dict_get(m_avf_cxt->streams[m_stream_index]->metadata,"rotate", tag, 0);
        if (tag==NULL){
            m_rotate_degree = 0;
        }
        else{
            int angle = atoi(tag->value);
            angle %= 360;
            m_rotate_degree = angle;
        }
    }

    EAGLEEYE_LOGD("video direction %d", m_rotate_degree);

    // 1.step 逐帧解析
    //为avpacket分配内存
    AVPacket* packet = av_packet_alloc();
    //为avFrame分配内存
    AVFrame* frame = av_frame_alloc();

    int iterator_count = 1;
    if(this->m_first_call){
        iterator_count = 3;
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
            SwsContext* swsContext = sws_getContext(frame->width, frame->height, (AVPixelFormat)frame->format,frame->width, frame->height, AV_PIX_FMT_RGB24,
                                SWS_BILINEAR, NULL, NULL, NULL);
            int num_bytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, frame->width, frame->height, 1);
            // std::cout<<"num bytes "<<num_bytes<<std::endl;
            // std::cout<<"size "<<frame->width*frame->height*3<<std::endl;
            // std::cout<<"height "<<frame->height<<" width "<<frame->width<<std::endl;
            // std::cout<<"linesize "<<frame->linesize[0]<<" "<<frame->linesize[1]<<" "<<frame->linesize[2]<<std::endl;

            Matrix<Array<unsigned char,3>> frame_rgb_data(frame->height, frame->width);
            uint8_t* rgb_buffer[1] = {(uint8_t*)frame_rgb_data.dataptr()};
            int rgb_stride[1] = { 3 * frame->width};
            sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height, rgb_buffer, rgb_stride);
            output_img_signal->meta()->rotation = this->m_rotate_degree;
            if(index==0 && iterator_count==3){
                // 首次调用,直接输出赋值
                output_img_signal->setData(frame_rgb_data);
            }
            else if(index == 1 && iterator_count==3){
                m_next = frame_rgb_data;
            }
            else{
                // 每次调用，获得下下次数据
                m_nextnext = frame_rgb_data;
            }

            sws_freeContext(swsContext);
        }
        else{
            // 没有发现
            m_nextnext = Matrix<Array<unsigned char,3>>();
        }
    }

    if(m_nextnext.isempty()){
        output_img_signal->meta()->is_end_frame=true;
    }

    av_free_packet(packet);
    av_packet_free(&packet);
    av_frame_free(&frame);
    this->m_frame_count += 1;

    EAGLEEYE_LOGD("extract frame %d (total %d)", this->m_frame_count, this->m_frame_total);
    this->m_first_call = false;
}

void VideoReadNode::setFilePath(std::string file_path)
{
	m_file_path = file_path;
    this->m_frame_count = 0;
    this->m_frame_total = 0;
    this->m_decoder_finish = false;
    this->m_first_call = true;
    ImageSignal<Array<unsigned char,3>>* output_img_signal = (ImageSignal<Array<unsigned char,3>>*)(this->getOutputPort(0));
    output_img_signal->meta()->is_end_frame=false;
    m_next = Matrix<Array<unsigned char, 3>>();
    m_nextnext = Matrix<Array<unsigned char, 3>>();
	//force time to update
	modified();
}

void VideoReadNode::getFilePath(std::string& file_path){
    file_path = this->m_file_path;
}

void VideoReadNode::feadback(std::map<std::string, int>& node_state_map){
    if(m_decoder_finish){
        Superclass::feadback(node_state_map);
    }
    else if(!m_next.isempty()){
        this->modified();
    }
}
} // namespace  eagleeye

#endif