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

#ifndef MAX
#define MAX(a, b) ({__typeof__(a) _a = (a); __typeof__(b) _b = (b); _a > _b ? _a : _b; })
#define MIN(a, b) ({__typeof__(a) _a = (a); __typeof__(b) _b = (b); _a < _b ? _a : _b; })
#endif

// This value is 2 ^ 18 - 1, and is used to clamp the RGB values before their ranges
// are normalized to eight bits.
static const int kMaxChannelValue = 262143;

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
}

V4L2CameraNode::~V4L2CameraNode(){
    this->cameraStopStreaming();
    this->cameraClose();
}

static inline uint32_t YUV2RGB(int nY, int nU, int nV) {
    nY -= 16;
    nU -= 128;
    nV -= 128;
    if (nY < 0) nY = 0;

    // This is the floating point equivalent. We do the conversion in integer
    // because some Android devices do not have floating point in hardware.
    // nR = (int)(1.164 * nY + 2.018 * nU);
    // nG = (int)(1.164 * nY - 0.813 * nV - 0.391 * nU);
    // nB = (int)(1.164 * nY + 1.596 * nV);

    int nR = 1192 * nY + 1634 * nV;
    int nG = 1192 * nY - 833 * nV - 400 * nU;
    int nB = 1192 * nY + 2066 * nU;

    nR = MIN(kMaxChannelValue, MAX(0, nR));
    nG = MIN(kMaxChannelValue, MAX(0, nG));
    nB = MIN(kMaxChannelValue, MAX(0, nB));

    nR = (nR >> 10) & 0xff;
    nG = (nG >> 10) & 0xff;
    nB = (nB >> 10) & 0xff;

    return 0xff000000 | (nR << 16) | (nG << 8) | nB;
}


void V4L2CameraNode::convert(void *r, void *p, unsigned int rSize)
{
    unsigned char *raw = (unsigned char *)r;
    unsigned char *preview = (unsigned char *)p;

    /* TODO: Convert YUYV to ARGB. */
    if (pixelformat == V4L2_PIX_FMT_YUYV) {
        int size = width * height * 2;
        int in;
        int out;

        unsigned char y1;
        unsigned char u;
        unsigned char y2;
        unsigned char v;

        uint32_t argb;
        for(in = 0, out = 0; in < size; in += 4, out += 8) {
            y1 = raw[in];
            u = raw[in + 1];
            y2 = raw[in + 2];
            v = raw[in + 3];

            //android　ARGB_8888 像素数据在内存中其实是以R G B A R G B A …的顺序排布的
            argb = YUV2RGB(y1,u,v);
            preview[out] = (argb >> 16) & 0xff;;
            preview[out + 1] = (argb >> 8) & 0xff;
            preview[out + 2] = argb & 0xff;
            // preview[out + 3] = 0xff;

            argb = YUV2RGB(y2,u,v);
            preview[out + 3] = (argb >> 16) & 0xff;
            preview[out + 4] = (argb >> 8) & 0xff;
            preview[out + 5] = argb & 0xff;
            // preview[out + 7] = 0xff;
        }
    }

    return;
}


int V4L2CameraNode::cameraGrabRawFrame(void *raw_base)
{
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

// void V4L2CameraNode::receiveAndProcess(){
//     unsigned char *raw = new unsigned char[buf.length];
//     EAGLEEYE_LOGE("_start raw buf.length %d", buf.length);
//     Matrix<Array<unsigned char,3>> bgr_data(dst_height, dst_width);

//     int size;
//     int format = -1;
//     while (start) {
//         size = cameraGrabRawFrame(raw);

//         if (size < 0) {
//             usleep(1000);
//             continue;
//         }
//         convert(raw, bgr_data.cpu<unsigned char>(), size);

//         std::unique_lock<std::mutex> locker(gCameraFrameMu);
//         if(gCameraFrameQueue.size() > 3){
//             gCameraFrameQueue.pop();
//         }
//         gCameraFrameQueue.push(bgr_data);
//         locker.unlock();
//         // notify
//         androidCameraCond.notify_all();        
//     }

//     delete[] raw;
// }

void V4L2CameraNode::executeNodeInfo(){
    unsigned char *raw = new unsigned char[buf.length];
    EAGLEEYE_LOGE("_start raw buf.length %d", buf.length);
    Matrix<Array<unsigned char,3>> data(height, width);

    int size;
    int format = -1;
    while (start) {
        size = cameraGrabRawFrame(raw);
        if (size < 0) {
            usleep(1000);
            this->m_is_pull_error = true;
            continue;
        }
        convert(raw, data.cpu<unsigned char>(), size);
        break;    
    }
    this->m_is_pull_error = false;
    delete[] raw;

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
    int ret = this->cameraOpen(camera_id.c_str(), 640, 480, V4L2_PIX_FMT_YUYV);
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

    this->cameraStartStreaming();
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


int V4L2CameraNode::cameraOpen(const char *filename,
                      unsigned int w,
                      unsigned int h,
                      unsigned int p){
    int ret;
    struct v4l2_format format;

    fd = open(filename, O_RDWR, 0);
    if (fd < 0) {
        EAGLEEYE_LOGE("Error opening device: %s", filename);
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

void V4L2CameraNode::cameraStartStreaming(){
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

void V4L2CameraNode::cameraStopStreaming(){
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

// int V4L2CameraNode::cameraSetPreviewSize(int width, int height, int pixformat){
//     int ret;
//     struct v4l2_format format;

//     EAGLEEYE_LOGD("setPreviewSize %d, %d, %d", width, height, pixformat);
//     this->width = width;
//     this->height = height;
//     this->pixelformat = pixformat;

//     format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
//     format.fmt.pix.width = width;
//     format.fmt.pix.height = height;
//     format.fmt.pix.pixelformat = pixelformat;

//     // MUST set
//     format.fmt.pix.field = V4L2_FIELD_ANY;

//     ret = ioctl(fd, VIDIOC_S_FMT, &format);
//     if (ret < 0) {
//         EAGLEEYE_LOGE("Unable to set format: %s", strerror(errno));
//         return -1;
//     }

//     return 0;
// }

}
