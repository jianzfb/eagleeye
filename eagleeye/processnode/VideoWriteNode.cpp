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
#endif
namespace  eagleeye
{
VideoWriteNode::VideoWriteNode(){
    this->setNumberOfInputSignals(1);
    this->setNumberOfOutputSignals(1);
    this->setOutputPort(this->makeOutputSignal(), 0);
    this->m_is_init = false;
    this->m_is_finish = true;
    this->m_fps = 30;

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
}

VideoWriteNode::~VideoWriteNode(){
    if(!this->m_is_finish){
        this->writeFinish();
    }
} 

static bool encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt, std::ofstream& fp)
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

void VideoWriteNode::executeNodeInfo(){
    ImageSignal<Array<unsigned char,3>>* input_img_signal = 
                    (ImageSignal<Array<unsigned char,3>>*)(this->getInputPort(0));
    MetaData image_meta_data;
    Matrix<Array<unsigned char, 3>> image = input_img_signal->getData(image_meta_data);
    if(!image.isContinuous()){
        image = image.clone();
    }
    if(!m_is_init && (!image_meta_data.is_start_frame && !m_manually_start)){
        // 对于非首帧(或手动开始)，不可以启动初始化
        // 首帧和尾帧，必须设置
        return;
    }
    int image_h = image.rows();
    int image_w = image.cols();
    if(this->m_file_path.empty()){
        if(this->m_folder != "./"){
            if(!isdirexist(this->m_folder.c_str())){
                createdirectory(this->m_folder.c_str());
            }
        }

        this->setFilePath(this->m_folder+this->m_prefix + EagleeyeTime::getTimeStamp()+".mp4");
    }

    if(this->m_is_init && this->m_manually_stop != 0){
        this->writeFinish();
        EAGLEEYE_LOGD("Success to finish write video.");
        return;
    }

    if(!this->m_is_init){
        EAGLEEYE_LOGD("Start to write video.");

        //打开输出文件流
        m_output_file.open(m_file_path.c_str(), std::ios::binary);

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

        m_is_init = true;
        m_is_finish = false;
        this->m_frame_count = 0;
    }

// #ifndef EAGLEEYE_RKCHIP
    long start_time = EagleeyeTime::getCurrentTime();
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
    long end_time = EagleeyeTime::getCurrentTime();
    std::cout<<"cvt time "<<end_time-start_time<<std::endl;
// #endif

// #ifdef EAGLEEYE_RKCHIP
//     long start_time = EagleeyeTime::getCurrentTime();
//     int src_width = image_w;
//     int src_height = image_h;
//     int src_format = RK_FORMAT_RGB_888;
//     if(image_meta_data.color_format == -1 || image_meta_data.color_format == 0){
//         src_format = RK_FORMAT_RGB_888;
//     }
//     else{
//         src_format = RK_FORMAT_BGR_888;
//     }
//     int src_buf_size = src_width * src_height * get_bpp_from_format(src_format);
//     rga_buffer_t src_img, dst_img;
//     rga_buffer_handle_t src_handle, dst_handle;
//     memset(&src_img, 0, sizeof(src_img));
//     memset(&dst_img, 0, sizeof(dst_img));

//     int dst_width, dst_height, dst_format;
//     dst_width = src_width;
//     dst_height = src_height;
//     dst_format = RK_FORMAT_YCbCr_420_P;
    
//     src_handle = importbuffer_virtualaddr(image.cpu<char>(), src_buf_size);
//     if(m_temp.numel() != int(dst_width*dst_height*1.5)){
//         m_temp = Matrix<unsigned char>(1, int(dst_width*dst_height*1.5));
//     }
//     dst_handle = importbuffer_virtualaddr(m_temp.cpu<char>(), int(dst_width*dst_height*1.5));

//     src_img = wrapbuffer_handle(src_handle, src_width, src_height, src_format);
//     dst_img = wrapbuffer_handle(dst_handle, dst_width, dst_height, dst_format);
//     imcvtcolor(src_img, dst_img, src_format, dst_format);
//     if (src_handle)
//         releasebuffer_handle(src_handle);
//     if (dst_handle)
//         releasebuffer_handle(dst_handle);
//     long end_time = EagleeyeTime::getCurrentTime();

//     memcpy(m_frame->data[0], m_temp.cpu<char>(), m_frame->linesize[0]);
//     memcpy(m_frame->data[1], m_temp.cpu<char>() + m_frame->linesize[0], m_frame->linesize[1]);
//     memcpy(m_frame->data[2], m_temp.cpu<char>() + m_frame->linesize[0] + m_frame->linesize[1], m_frame->linesize[2]);
//     std::cout<<"linesize "<<m_frame->linesize[0]<<" "<<m_frame->linesize[1]<<" "<<m_frame->linesize[2]<<std::endl;

//     std::cout<<"cvt time "<<end_time-start_time<<std::endl;
// #endif

    int ret = av_frame_make_writable(m_frame);
    if(ret < 0){
        EAGLEEYE_LOGE("Could not av_frame_make_writable");
    }
    m_frame->pts = this->m_frame_count;
    this->m_frame_count += 1;

    /* encode the image */
    start_time = EagleeyeTime::getCurrentTime();
    encode(m_codec_cxt, m_frame, m_pkt, m_output_file);
    end_time = EagleeyeTime::getCurrentTime();
    std::cout<<"encode time "<<end_time-start_time<<std::endl;

    // stop
    if(image_meta_data.is_end_frame){
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
    /* flush the encoder */
    encode(m_codec_cxt, NULL, m_pkt, m_output_file);
    /* add sequence end code to have a real mpeg file */
    uint8_t endcode[] = { 0, 0, 1, 0xb7 };
    if (m_encoder->id == AV_CODEC_ID_MPEG1VIDEO || m_encoder->id == AV_CODEC_ID_MPEG2VIDEO)
        m_output_file.write((char*)endcode, sizeof(endcode));

	m_output_file.close();
	avcodec_free_context(&m_codec_cxt);    
    av_frame_free(&m_frame);
    av_packet_free(&m_pkt);

    m_codec_cxt = NULL;
    this->m_file_path = "";
    this->m_is_finish = true;
    this->m_is_init = false;
    this->m_manually_stop = 0;
    this->m_manually_start = 0;    
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

void VideoWriteNode::setFPS(int fps){
    this->m_fps = fps;
    this->modified();
}

void VideoWriteNode::getFPS(int& fps){
    fps = this->m_fps;
}
} // namespace  eagleeye

#endif