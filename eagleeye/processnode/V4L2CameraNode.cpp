#include "eagleeye/processnode/V4L2CameraNode.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/common/EagleeyeTime.h"
#include "libyuv.h"
#include <unistd.h>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

namespace eagleeye{
V4L2CameraNode::V4L2CameraNode(){
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

    start = 0;
    raw = NULL;
    raw_size = 0;

    m_preview_width = 0;
    m_preview_height = 0;
}

V4L2CameraNode::~V4L2CameraNode(){
    this->stopCameraStreaming();
    this->cameraUninit();
    this->cameraClose();

    if(raw != NULL){
        delete[] raw;
        raw_size = 0;
    }
}

int V4L2CameraNode::grabCameraRawFrame(void *raw_base){
    int ret;
    int data_size;

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    /* V4L2: dequeue buffer */
    ret = ioctl(fd, VIDIOC_DQBUF, &buf);
    if (ret < 0) {
        EAGLEEYE_LOGE("Unable query buffer: %s", strerror(errno));
        return ret;
    }

    data_size = buf.bytesused;

    /* copy to userspace */
    memcpy(raw_base, mem,  buf.bytesused);

    /* V4l2: queue buffer again after that */
    ret = ioctl(fd, VIDIOC_QBUF, &buf);
    if (ret < 0) {
        EAGLEEYE_LOGE("Unable query buffer: %s", strerror(errno));
        return ret;
    }

    return data_size;
}

void V4L2CameraNode::executeNodeInfo(){
    if(!this->m_is_camera_open){
        EAGLEEYE_LOGE("Camera %s not open, return directly.", m_camera_id.c_str());
        return;
    }

    if(this->m_is_init_error){
        EAGLEEYE_LOGE("Camera init fail, return directly.");
        return;
    }

    if(raw == NULL || raw_size != buf.length){
        if(raw != NULL){
            delete[] raw;
        }
        raw = new unsigned char[buf.length];
        raw_size = buf.length;
    }
    Matrix<Array<unsigned char,3>> data(height, width);
    unsigned char* bgr_data_ptr = data.cpu<unsigned char>();

    int size;
    int format = -1;
    auto *src_i420_data = (uint8_t *) malloc(sizeof(uint8_t) * width * height * 3 / 2);
    while(start){
        size = grabCameraRawFrame(raw);
        if(size < 0){
            usleep(1000);
            this->m_is_pull_error = true;
            continue;
        }

        int src_y_size = width * height;
        int src_u_size = (width >> 1) * (height >> 1);
        uint8_t *src_i420_y_data = src_i420_data;
        uint8_t *src_i420_u_data = src_i420_data + src_y_size;
        uint8_t *src_i420_v_data = src_i420_data + src_y_size + src_u_size;
        int i420_stride_y = width;
        libyuv::YUY2ToI420(
                raw,
                width * 2,
                src_i420_y_data, i420_stride_y,
                src_i420_u_data, i420_stride_y >> 1,
                src_i420_v_data, i420_stride_y >> 1,
                width, height);

        if(m_output_image_format == 0){
            // RGB
            libyuv::I420ToRAW(
                src_i420_y_data, i420_stride_y, 
                src_i420_u_data, (i420_stride_y>>1),
                src_i420_v_data, (i420_stride_y>>1),
                bgr_data_ptr, width*3,
                width, height);
        }
        else{
            // BGR
            libyuv::I420ToRGB24(
                src_i420_y_data, i420_stride_y, 
                src_i420_u_data, (i420_stride_y>>1),
                src_i420_v_data, (i420_stride_y>>1),
                bgr_data_ptr, width*3,
                width, height);
        }
        break;    
    }
    free(src_i420_data);
    this->m_is_pull_error = false;

    // image data
    ImageSignal<Array<unsigned char,3>>* output_img_signal = (ImageSignal<Array<unsigned char,3>>*)(this->getOutputPort(0));
    MetaData output_meta = output_img_signal->meta();
    output_meta.timestamp = 0;
    output_img_signal->setData(
        data, 
        output_meta
    );

    // time data
    ImageSignal<double>* output_timestamp_signal = (ImageSignal<double>*)(this->getOutputPort(1));
    output_timestamp_signal->getData().at(0,0) = 0;
}

void V4L2CameraNode::processUnitInfo(){
    Superclass::processUnitInfo();
    modified();
}

void V4L2CameraNode::setCameraId(std::string camera_id){
    if(m_camera_id == camera_id){
        EAGLEEYE_LOGE("Active camera id is %s, dont need to reopen", camera_id.c_str());
        return;
    }
    if(m_is_camera_open || (!m_is_init_error)){
        // 关闭已经开启的相机
        this->stopCameraStreaming();
        this->cameraUninit();
        this->cameraClose();

        m_is_camera_open = false;
        m_is_init_error = false;
    }

    int ret = this->cameraOpen(camera_id.c_str(), m_preview_width, m_preview_height, V4L2_PIX_FMT_YUYV);
    if(ret < 0){
        m_is_camera_open = false;
        return;
    }
    m_is_camera_open = true;

    ret = this->cameraInit();
    if(ret < 0){
        m_is_init_error = true;
        return;
    }
    m_is_init_error = false;
    this->startCameraStreaming();
}

void V4L2CameraNode::getCameraId(std::string& camera_id){
    camera_id = m_camera_id; 
}

void V4L2CameraNode::setImageFormat(int image_format){
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

void V4L2CameraNode::setImageRotation(int image_rotation){
    // image_format: 0,90,180,270
    if(image_rotation != 0 && image_rotation != 90 && image_rotation != 180 && image_rotation != 270){
        return;
    }
    this->m_output_image_rotation = image_rotation;
}


int V4L2CameraNode::cameraOpen(const char *filename, unsigned int w, unsigned int h, unsigned int p){
    int ret;
    struct v4l2_format format;

    fd = open(filename, O_RDWR, 0);
    if (fd < 0) {
        EAGLEEYE_LOGE("Error opening device: %s", filename);
        return -1;
    }

    // 检查是否支持申请尺寸
    if(!isSupportPreviewSize(w,h,p)){
        EAGLEEYE_LOGE("Error width %d, height %d not support", w, h);
        return -1;
    }
    
    width = w;
    height = h;
    pixelformat = p;

    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = width;
    format.fmt.pix.height = height;
    format.fmt.pix.pixelformat = pixelformat;

    // MUST set 
    format.fmt.pix.field = V4L2_FIELD_ANY;

    ret = ioctl(fd, VIDIOC_S_FMT, &format);
    if (ret < 0) {
        EAGLEEYE_LOGE("Unable to set format: %s", strerror(errno));
        return -1;
    }
    return 0;
}

int V4L2CameraNode::cameraInit(){
    int ret;
    struct v4l2_requestbuffers rb;

    start = false;

    /* V4L2: request buffers, only 1 frame */
    rb.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    rb.memory = V4L2_MEMORY_MMAP;
    rb.count = 1;

    ret = ioctl(fd, VIDIOC_REQBUFS, &rb);
    if (ret < 0) {
        EAGLEEYE_LOGE("Unable request buffers: %s", strerror(errno));
        return -1;
    }

    /* V4L2: map buffer  */
    memset(&buf, 0, sizeof(struct v4l2_buffer));

    buf.index = 0;
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    ret = ioctl(fd, VIDIOC_QUERYBUF, &buf);
    if (ret < 0) {
        EAGLEEYE_LOGE("Unable query buffer: %s", strerror(errno));
        return -1;
    }

    /* Only map one */
    mem = (unsigned char *)mmap(0, buf.length, PROT_READ | PROT_WRITE, 
				MAP_SHARED, fd, buf.m.offset);
    if (mem == MAP_FAILED) {
        EAGLEEYE_LOGE("Unable map buffer: %s", strerror(errno));
        return -1;
    }

    /* V4L2: queue buffer */
    ret = ioctl(fd, VIDIOC_QBUF, &buf);

    return 0;
}

void V4L2CameraNode::cameraUninit(){
    munmap(mem, buf.length);
    return ;
}

void V4L2CameraNode::cameraClose(){
    close(fd);
}

void V4L2CameraNode::startCameraStreaming(){
    enum v4l2_buf_type type;
    int ret;

    if (start) return;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ret = ioctl(fd, VIDIOC_STREAMON, &type);
    if (ret < 0) {
        EAGLEEYE_LOGE("Unable query buffer: %s", strerror(errno));
        return;
    }
    start = true;
}

void V4L2CameraNode::stopCameraStreaming(){
    enum v4l2_buf_type type;
    int ret;

    if (!start) return;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ret = ioctl(fd, VIDIOC_STREAMOFF, &type);
    if (ret < 0) {
        EAGLEEYE_LOGE("Unable query buffer: %s", strerror(errno));
        return;
    }

    start = false;
}

void V4L2CameraNode::setPreviewSize(int width, int height){
    this->m_preview_width=width;
    this->m_preview_height=height;
}

bool V4L2CameraNode::isSupportPreviewSize(unsigned int& width, unsigned int& height, unsigned int pixelformat){
    struct v4l2_fmtdesc fmtd;	        //存的是摄像头支持的传输格式
    struct v4l2_frmsizeenum  frmsize;	//存的是摄像头对应的图片格式所支持的分辨率
    bool is_found = false;
    for (int i = 0; ; i++){
        fmtd.index = i;
        fmtd.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (ioctl(fd, VIDIOC_ENUM_FMT, &fmtd) < 0)
            break;

        for (int j = 0; ; j++){
            frmsize.index = j;
            frmsize.pixel_format = fmtd.pixelformat;
            if (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) < 0)
                break;

            if(fmtd.pixelformat != pixelformat){
                continue;
            }
    
            EAGLEEYE_LOGD("check v4l2 camera w = %d, h = %d", frmsize.discrete.width, frmsize.discrete.height);
            if(width == 0 || height == 0){
                // 使用默认宽高
                EAGLEEYE_LOGD("Use default camera w = %d, h = %d", frmsize.discrete.width, frmsize.discrete.height);
                width = frmsize.discrete.width;
                height = frmsize.discrete.height;
            }
            if(frmsize.discrete.width == width && frmsize.discrete.height == height){
                is_found = true;
                break;
            }
        }
    }

    return is_found;
}
}
