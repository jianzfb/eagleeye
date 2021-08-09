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
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
}
namespace  eagleeye
{
VideoWriteNode::VideoWriteNode(){
    this->setNumberOfInputSignals(1);
    this->setNumberOfOutputSignals(1);
    this->setOutputPort(this->makeOutputSignal(), 0);
    this->m_is_init = false;
    this->m_is_finish = true;
    this->m_fps = 0;
    m_output_cxt = NULL;
    m_codec_cxt = NULL;
    stream = NULL;
    packet = NULL;
    frame = NULL;
    avcodec_register_all();
    EAGLEEYE_MONITOR_VAR(std::string, setFilePath, getFilePath, "file","","");
    EAGLEEYE_MONITOR_VAR(std::string, setPrefix, getPrefix, "prefix","","");
    EAGLEEYE_MONITOR_VAR(std::string, setFolder, getFolder, "folder","","");

    EAGLEEYE_MONITOR_VAR(int, setStop, getStop, "stop","0","2");

    this->m_prefix = "video_";
    this->m_folder = "/sdcard/";
    this->m_manually_stop = 0;
}   

VideoWriteNode::~VideoWriteNode(){
    if(!this->m_is_finish){
        this->writeFinish();
    }
} 

void VideoWriteNode::executeNodeInfo(){
    ImageSignal<Array<unsigned char,3>>* input_img_signal = 
                    (ImageSignal<Array<unsigned char,3>>*)(this->getInputPort(0));
    MetaData image_meta_data;
    Matrix<Array<unsigned char, 3>> image = input_img_signal->getData(image_meta_data);
    if(!image.isContinuous()){
        image = image.clone();
    }

    if(!m_is_init && !image_meta_data.is_start_frame){
        // 对于非首帧，不可以启动初始化
        // 首帧和尾帧，必须设置
        return;
    }

    if(this->m_file_path.empty()){
        if(this->m_folder != "./"){
            if(!isdirexist(this->m_folder.c_str())){
                createdirectory(this->m_folder.c_str());
            }
        }

        this->setFilePath(this->m_folder+this->m_prefix + EagleeyeTime::getTimeStamp()+".mp4");
        EAGLEEYE_LOGD("Video path %s.", this->m_file_path.c_str());
    }

    if(this->m_is_init && this->m_manually_stop != 0){
        // 开始保存
        this->m_file_path = "";
        this->m_is_finish = true;
        this->m_is_init = false;
        this->m_manually_stop = 0;
        this->writeFinish();

        EAGLEEYE_LOGD("Success to finish write video.");
        return;
    }

    if(!this->m_is_init){
        EAGLEEYE_LOGD("Start to write video.");
        //新建一个输出的AVFormatContext并分配内存
        m_output_cxt = avformat_alloc_context();
        AVOutputFormat* fmt = av_guess_format(NULL, this->m_file_path.c_str(), NULL);
        m_output_cxt->oformat = fmt;
        //创建和初始化一个和该URL相关的AVIOContext
        if(avio_open(&m_output_cxt->pb,this->m_file_path.c_str(),AVIO_FLAG_READ_WRITE) < 0){
            EAGLEEYE_LOGD("Couldnt open write file.");
            avformat_free_context(m_output_cxt);
            return;
        }
        //构建新的Stream
        stream = avformat_new_stream(m_output_cxt, NULL);
        if(stream == NULL){
            EAGLEEYE_LOGD("Couldnt build new stream.");
            return;
        }
        stream->time_base.num = 1;
        stream->time_base.den = 30;

        //初始化AVStream信息
        m_codec_cxt = stream->codec;
        m_codec_cxt->codec_id = m_output_cxt->oformat->video_codec;
        m_codec_cxt->codec_type = AVMEDIA_TYPE_VIDEO;
        m_codec_cxt->pix_fmt = AV_PIX_FMT_YUV420P;
        m_codec_cxt->height = image.rows();
        m_codec_cxt->width = image.cols();
        m_codec_cxt->time_base.num = 1;
        m_codec_cxt->time_base.den = 30;
        m_codec_cxt->bit_rate = 400000;  
        m_codec_cxt->gop_size = 250;
        m_codec_cxt->max_b_frames = 1;

        //H.264
        if(m_codec_cxt->codec_id == AV_CODEC_ID_H264) {
            m_codec_cxt->refs = 3;
            m_codec_cxt->qcompress = 1.0;
            m_codec_cxt->qmin = 10;
            m_codec_cxt->qmax = 20;
        }

        if (m_codec_cxt->codec_id == AV_CODEC_ID_MPEG2VIDEO)
            m_codec_cxt->max_b_frames = 2;
        if (m_codec_cxt->codec_id == AV_CODEC_ID_MPEG1VIDEO)
            m_codec_cxt->mb_decision = 2;

        AVCodec* codec = avcodec_find_encoder(m_codec_cxt->codec_id);
        if(!codec){
            EAGLEEYE_LOGD("Couldnt find encoder.");
            return;
        }
        if(avcodec_open2(m_codec_cxt,codec,NULL) < 0){
            EAGLEEYE_LOGD("Couldnt open encoder.");
            return;
        }
        avcodec_parameters_from_context(stream->codecpar, m_codec_cxt);

        // 写入文件头
        avformat_write_header(m_output_cxt, NULL);

        // 申请空间 (frame, packet, data)
        packet = av_packet_alloc();
        av_new_packet(packet, m_codec_cxt->width * m_codec_cxt->height * 3);        
        frame = av_frame_alloc();
        av_image_alloc(frame->data, frame->linesize, m_codec_cxt->width, m_codec_cxt->height,AV_PIX_FMT_YUV420P,32);
        m_is_init = true;
        m_is_finish = false;
    }

    // // 将RGB24转换成YUVframe
    // SwsContext* swsContext = sws_getContext(image.cols(), 
    //                                         image.rows(),
    //                                         AV_PIX_FMT_RGB24 ,
    //                                         image.cols(), 
    //                                         image.rows(),
    //                                         AV_PIX_FMT_YUV420P ,
    //                                         NULL, NULL, NULL, NULL);
    // uint8_t *const rgb_bufer[1] = {(uint8_t*)image.dataptr()};
    // int srcStride[1];
    // srcStride[0] = image.cols()*3;

    frame->pts = this->m_fps;
    this->m_fps += 1;
    frame->format = m_codec_cxt->pix_fmt;
    frame->width  = image.cols();
    frame->height = image.rows();

    // sws_scale(swsContext, rgb_bufer, srcStride, 0, image.rows(), frame->data, frame->linesize);
    // sws_freeContext(swsContext);

    EAGLEEYE_TIME_START(A);
    // 使用libyuv 转换空间
    libyuv::RAWToI420((uint8_t*)image.dataptr(), 
                image.cols()*3, 
                frame->data[0],frame->linesize[0],
                frame->data[1],frame->linesize[1],
                frame->data[2],frame->linesize[2],
                frame->width,
                frame->height);
    EAGLEEYE_TIME_END(A);

    EAGLEEYE_TIME_START(B);
    int got_picture = 0;
    int ret = avcodec_encode_video2(m_codec_cxt, packet, frame, &got_picture);
    if(ret < 0){
        EAGLEEYE_LOGD("Fail to encoder.");
        return;
    }
    EAGLEEYE_TIME_END(B);

    if(got_picture == 1){
        EAGLEEYE_TIME_START(C);
        //将packet中的数据写入本地文件
        packet->stream_index = stream->index;
        av_packet_rescale_ts(packet, m_codec_cxt->time_base, stream->time_base);
        packet->pos = -1;
        av_interleaved_write_frame(m_output_cxt, packet);
        EAGLEEYE_TIME_END(C);
        // av_packet_unref(packet);
        // av_frame_unref(frame);
    }

    if(image_meta_data.is_end_frame){
        // 当前是视频流的最后一帧
        this->m_file_path = "";
        this->m_is_finish = true;
        this->m_is_init = false;
        this->m_manually_stop = 0;
        this->writeFinish();

        EAGLEEYE_LOGD("Success to finish write video.");
    }
}

void VideoWriteNode::setFilePath(std::string file_path){
    if(!this->m_is_finish){
        EAGLEEYE_LOGD("Force unfinish video stop.");
        this->writeFinish();        
    }

    this->m_is_init = false;
    this->m_file_path = file_path;
    this->m_is_finish = true;
    this->m_fps = 0;
    this->m_manually_stop = 0;
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

void VideoWriteNode::writeFinish(){
    //将流尾写入输出媒体文件并释放文件数据
    flushEncoder(m_output_cxt, 0);
    av_write_trailer(m_output_cxt);

    if(this->m_codec_cxt){
        avcodec_close(m_codec_cxt);
    }

    if(this->m_output_cxt){
        avio_close(m_output_cxt->pb);
        avformat_free_context(m_output_cxt);
    }

    av_freep(&frame->data[0]);
    av_frame_free(&frame);
    av_free_packet(packet);
    av_packet_free(&packet);

    frame = NULL;
    packet = NULL;
    m_output_cxt = NULL;
    m_codec_cxt = NULL;
    this->m_is_finish = true;
}

int VideoWriteNode::flushEncoder(AVFormatContext *fmt_ctx, unsigned int stream_index)
{
    int ret;
    int got_frame;
    AVPacket enc_pkt;
    if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities))
        return 0;
    while (1) {
        printf("Flushing stream #%u encoder\n", stream_index);
        enc_pkt.data = NULL;
        enc_pkt.size = 0;
        av_init_packet(&enc_pkt);
        ret = avcodec_encode_video2(fmt_ctx->streams[stream_index]->codec, &enc_pkt,
            NULL, &got_frame);
        av_frame_free(NULL);
        if (ret < 0)
            break;
        if (!got_frame)
        {
            ret = 0; break;
        }

        // parpare packet for muxing  
        enc_pkt.stream_index = stream_index;
        av_packet_rescale_ts(&enc_pkt,
            fmt_ctx->streams[stream_index]->codec->time_base,
            fmt_ctx->streams[stream_index]->time_base);
        ret = av_interleaved_write_frame(fmt_ctx, &enc_pkt);
        if (ret < 0)
            break;
    }
    return ret;
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

} // namespace  eagleeye

#endif