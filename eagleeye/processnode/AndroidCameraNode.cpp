#include "eagleeye/processnode/AndroidCameraNode.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include <string>
#include <fstream>
#include <vector>
#include <thread>
#include "libyuv.h"
#include <queue>
#include <mutex>
#include <condition_variable>

#if defined(__ANDROID__) || defined(ANDROID)
#include <jni.h>
#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraMetadata.h>
#include <camera/NdkCameraDevice.h>
#include <media/NdkImageReader.h>
#include <android/native_window_jni.h>

/**
 * Variables used to initialize and manage native camera
 */

static ACameraManager* cameraManager = nullptr;

static ACameraDevice* cameraDevice = nullptr;

static ACaptureRequest* request = nullptr;

static ACameraCaptureSession* textureSession = nullptr;

static ANativeWindow* imageWindow = nullptr;

static ACameraOutputTarget* imageTarget = nullptr;

static AImageReader* imageReader = nullptr;

static ACaptureSessionOutput* imageOutput = nullptr;
static ACaptureSessionOutputContainer* outputs = nullptr;

static int captureImageWidth = 640;
static int captureImageHeight = 480;
static int captureImageRotate = 0;

static std::string cameraFacing = "back";

static std::mutex androidCameraMu;
static std::condition_variable androidCameraCond;
static std::queue<eagleeye::Matrix<eagleeye::Array<unsigned char, 3>>> androidCameraQueue;

/**
 * Device listeners
 */

static void onDisconnected(void* context, ACameraDevice* device){
    EAGLEEYE_LOGD("Device Disconnected.");
}

static void onError(void* context, ACameraDevice* device, int error){
    EAGLEEYE_LOGD("Device Error %d.", error);
}

/**
 * Session state callbacks
 */

static void onSessionActive(void* context, ACameraCaptureSession *session){
    EAGLEEYE_LOGD("onSessionActive()");
}

static void onSessionReady(void* context, ACameraCaptureSession *session){
    EAGLEEYE_LOGD("onSessionReady()");
}

static void onSessionClosed(void* context, ACameraCaptureSession *session){
    EAGLEEYE_LOGD("onSessionClosed()");
}

/**
 * Image reader setup. If you want to use AImageReader, enable this in CMakeLists.txt.
 */
static void imageCallback(void* context, AImageReader* reader){
    AImage *image = nullptr;
    auto status = AImageReader_acquireNextImage(reader, &image);

    uint8_t *data = nullptr;
    int len = 0;
    uint8_t *src_y = nullptr;
    AImage_getPlaneData(image, 0, &src_y, &len);
    int32_t src_stride_y = 0;
    AImage_getPlaneRowStride(image, 0, &src_stride_y);

    uint8_t *src_u = nullptr;
    AImage_getPlaneData(image, 1, &src_u, &len);
    int32_t src_stride_u = 0;
    AImage_getPlaneRowStride(image, 1, &src_stride_u);

    uint8_t *src_v = nullptr;
    AImage_getPlaneData(image, 2, &src_v, &len);
    int32_t src_stride_v = 0;
    AImage_getPlaneRowStride(image, 2, &src_stride_v);

    // Process data here
    // ...
    // yuv -> bgr -> output queue
    auto *src_i420_data = (uint8_t *) malloc(sizeof(uint8_t) * captureImageWidth * captureImageHeight * 3 / 2);
    jint src_y_size = captureImageWidth * captureImageHeight;
    jint src_u_size = (captureImageWidth >> 1) * (captureImageHeight >> 1);
    uint8_t *src_i420_y_data = src_i420_data;
    uint8_t *src_i420_u_data = src_i420_data + src_y_size;
    uint8_t *src_i420_v_data = src_i420_data + src_y_size + src_u_size;

    // -> I420
    int i420_stride_y;
    int dst_width;
    int dst_height;
    if (captureImageRotate == 0 || captureImageRotate == 180) {
        i420_stride_y = captureImageWidth;
        dst_width = captureImageWidth;
        dst_height = captureImageHeight;
    } else {
        i420_stride_y = captureImageHeight;
        dst_width = captureImageHeight;
        dst_height = captureImageWidth;
    }

    libyuv::Android420ToI420(
            src_y, src_stride_y,
            src_u, src_stride_u,
            src_v, src_stride_v,
            2,
            src_i420_y_data, i420_stride_y,
            src_i420_u_data, i420_stride_y >> 1,
            src_i420_v_data, i420_stride_y >> 1,
            captureImageWidth, captureImageHeight);

    // -> BGR
    eagleeye::Matrix<eagleeye::Array<unsigned char,3>> bgr_data(dst_height, dst_width);
    uint8_t* bgr_data_ptr = bgr_data.cpu<uint8_t>();
    libyuv::I420ToRGB24(
        src_i420_y_data, i420_stride_y, 
        src_i420_u_data, (i420_stride_y>>1),
        src_i420_v_data, (i420_stride_y>>1),
        bgr_data_ptr, dst_width*3,
        dst_width,
        dst_height);

    free(src_i420_data);
    AImage_delete(image);

    // 加入队列
    std::unique_lock<std::mutex> locker(androidCameraMu);
    if(androidCameraQueue.size() > 3){
        androidCameraQueue.pop();
    }
    androidCameraQueue.push(bgr_data);
    locker.unlock();

    // notify
    androidCameraCond.notify_all();
}

AImageReader* createYUVReader(){
    AImageReader* reader = nullptr;
    AImageReader_new(640, 480, AIMAGE_FORMAT_YUV_420_888, 2, &reader);
    AImageReader_ImageListener listener{
            .context = nullptr,
            .onImageAvailable = imageCallback,
    };

    AImageReader_setImageListener(reader, &listener);
    return reader;
}

ANativeWindow* createSurface(AImageReader* reader){
    ANativeWindow *nativeWindow;
    AImageReader_getWindow(reader, &nativeWindow);

    return nativeWindow;
}

/**
 * Capture callbacks
 */

void onCaptureFailed(void* context, ACameraCaptureSession* session,
                     ACaptureRequest* request, ACameraCaptureFailure* failure){
    EAGLEEYE_LOGE("onCaptureFailed ");
}

void onCaptureSequenceCompleted(void* context, ACameraCaptureSession* session,
                                int sequenceId, int64_t frameNumber){}

void onCaptureSequenceAborted(void* context, ACameraCaptureSession* session,
                              int sequenceId){}

void onCaptureCompleted (
        void* context, ACameraCaptureSession* session,
        ACaptureRequest* request, const ACameraMetadata* result){
    EAGLEEYE_LOGD("onCaptureCompleted");
}

static ACameraCaptureSession_captureCallbacks captureCallbacks {
        .context = nullptr,
        .onCaptureStarted = nullptr,
        .onCaptureProgressed = nullptr,
        .onCaptureCompleted = onCaptureCompleted,
        .onCaptureFailed = onCaptureFailed,
        .onCaptureSequenceCompleted = onCaptureSequenceCompleted,
        .onCaptureSequenceAborted = onCaptureSequenceAborted,
        .onCaptureBufferLost = nullptr,
};

static void printCamProps(ACameraManager *cameraManager, const char *id){
    // exposure range
    // std::cout<<"A"<<std::endl;
    // ACameraMetadata *metadataObj;
    // ACameraManager_getCameraCharacteristics(cameraManager, id, &metadataObj);

    // std::cout<<"B"<<std::endl;
    // ACameraMetadata_const_entry entry = {0};
    // ACameraMetadata_getConstEntry(metadataObj,
    //                               ACAMERA_SENSOR_INFO_EXPOSURE_TIME_RANGE, &entry);

    // std::cout<<"C"<<std::endl;
    // int64_t minExposure = entry.data.i64[0];
    // int64_t maxExposure = entry.data.i64[1];
    // EAGLEEYE_LOGD("camProps: minExposure=%lld vs maxExposure=%lld", minExposure, maxExposure);
    // ////////////////////////////////////////////////////////////////

    // // sensitivity
    // std::cout<<"D"<<std::endl;
    // ACameraMetadata_getConstEntry(metadataObj,
    //                               ACAMERA_SENSOR_INFO_SENSITIVITY_RANGE, &entry);

    // std::cout<<"E"<<std::endl;
    // int32_t minSensitivity = entry.data.i32[0];
    // int32_t maxSensitivity = entry.data.i32[1];

    // std::cout<<"F"<<std::endl;
    // EAGLEEYE_LOGD("camProps: minSensitivity=%d vs maxSensitivity=%d", minSensitivity, maxSensitivity);
    // ////////////////////////////////////////////////////////////////

    // std::cout<<"G"<<std::endl;
    // // YUV format
    // ACameraMetadata_getConstEntry(metadataObj,
    //                               ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, &entry);

    // std::cout<<"H"<<std::endl;
    // for (int i = 0; i < entry.count; i += 4){
    //     // We are only interested in output streams, so skip input stream
    //     int32_t input = entry.data.i32[i + 3];
    //     if (input)
    //         continue;

    //     int32_t format = entry.data.i32[i + 0];
    //     if (format == AIMAGE_FORMAT_YUV_420_888){
    //         int32_t width = entry.data.i32[i + 1];
    //         int32_t height = entry.data.i32[i + 2];
    //         EAGLEEYE_LOGD("camProps: maxWidth=%d vs maxHeight=%d", width, height);
    //     }
    // }
    // std::cout<<"I"<<std::endl;

    // // cam facing
    // ACameraMetadata_getConstEntry(metadataObj,
    //                               ACAMERA_SENSOR_ORIENTATION, &entry);

    // std::cout<<"G"<<std::endl;
    // captureImageRotate = entry.data.i32[0];
    // EAGLEEYE_LOGD("camProps: %d", captureImageRotate);
    // std::cout<<"K"<<std::endl;
}


static std::string getBackFacingCamId(ACameraManager *cameraManager){
    ACameraIdList *cameraIds = nullptr;
    ACameraManager_getCameraIdList(cameraManager, &cameraIds);
    EAGLEEYE_LOGD("Found camera count %d.", cameraIds->numCameras);

    std::string backId;
    for (int i = 0; i < cameraIds->numCameras; ++i){
        const char *id = cameraIds->cameraIds[i];

        ACameraMetadata *metadataObj;
        ACameraManager_getCameraCharacteristics(cameraManager, id, &metadataObj);

        ACameraMetadata_const_entry lensInfo = {0};
        ACameraMetadata_getConstEntry(metadataObj, ACAMERA_LENS_FACING, &lensInfo);

        auto facing = static_cast<acamera_metadata_enum_android_lens_facing_t>(
                lensInfo.data.u8[0]);

        // Found a back-facing camera?
        if (facing == ACAMERA_LENS_FACING_BACK){
            backId = id;
            break;
        }
        backId = id;
        break;
    }

    ACameraManager_deleteCameraIdList(cameraIds);
    return backId;
}

static std::string getFrontFacingCamId(ACameraManager *cameraManager){
    ACameraIdList *cameraIds = nullptr;
    ACameraManager_getCameraIdList(cameraManager, &cameraIds);
    EAGLEEYE_LOGD("Found camera count %d.", cameraIds->numCameras);

    std::string frontId;    
    for (int i = 0; i < cameraIds->numCameras; ++i){
        const char *id = cameraIds->cameraIds[i];

        ACameraMetadata *metadataObj;
        ACameraManager_getCameraCharacteristics(cameraManager, id, &metadataObj);

        ACameraMetadata_const_entry lensInfo = {0};
        ACameraMetadata_getConstEntry(metadataObj, ACAMERA_LENS_FACING, &lensInfo);

        auto facing = static_cast<acamera_metadata_enum_android_lens_facing_t>(
                lensInfo.data.u8[0]);

        // Found a back-facing camera?
        if (facing == ACAMERA_LENS_FACING_FRONT){
            frontId = id;
            break;
        }
    }

    ACameraManager_deleteCameraIdList(cameraIds);
    return frontId;
}

static void exitCamera(){
    // clear
    if(textureSession != nullptr){
        ACameraCaptureSession_stopRepeating(textureSession);
        ACameraCaptureSession_close(textureSession);
        textureSession = nullptr;
    }

    if(outputs != nullptr){
        ACaptureSessionOutputContainer_free(outputs);
        outputs = nullptr;
    }
    if(imageOutput != nullptr){
        ACaptureSessionOutput_free(imageOutput);
        imageOutput = nullptr;
    }

    if(request != nullptr){
        ACaptureRequest_removeTarget(request, imageTarget);
        ACaptureRequest_free(request);
        request = nullptr;
    }

    if(imageTarget != nullptr){
        ACameraOutputTarget_free(imageTarget);
        imageTarget = nullptr;
    }

    if(cameraDevice != nullptr){
        ACameraDevice_close(cameraDevice);
        cameraDevice = nullptr;
    }

    if(imageReader != nullptr){
        AImageReader_delete(imageReader);
        imageReader = nullptr;
    }

    if(cameraManager != nullptr){
        ACameraManager_delete(cameraManager);
        cameraManager = nullptr;
    }
}

static bool initCam(){
    ACameraDevice_stateCallbacks cameraDeviceCallbacks = {
            .context = nullptr,
            .onDisconnected = onDisconnected,
            .onError = onError,
    };

    cameraManager = ACameraManager_create();
    if(cameraFacing == "back"){
        auto id = getBackFacingCamId(cameraManager);
        if(id == ""){
            EAGLEEYE_LOGE("Dont found back camera.");
            return false;
        }
        EAGLEEYE_LOGD("Start open back camera ");
        camera_status_t status = ACameraManager_openCamera(cameraManager, id.c_str(), &cameraDeviceCallbacks, &cameraDevice);
        if(status != ACAMERA_OK){
            EAGLEEYE_LOGE("Fail to open back camera %s.", id.c_str());
            return false;
        }
        EAGLEEYE_LOGD("Finish open back camera %s.", id.c_str());
        printCamProps(cameraManager, id.c_str());
    }
    else{
        auto id = getFrontFacingCamId(cameraManager);
        if(id == ""){
            EAGLEEYE_LOGE("Dont found front camera.");
            return false;
        }        
        EAGLEEYE_LOGD("Start open front camera ");
        camera_status_t status = ACameraManager_openCamera(cameraManager, id.c_str(), &cameraDeviceCallbacks, &cameraDevice);
        if(status != ACAMERA_OK){
            EAGLEEYE_LOGE("Fail to open front camera %s.", id.c_str());
            return false;
        }
        EAGLEEYE_LOGD("Finish open front camera %s.", id.c_str());
        printCamProps(cameraManager, id.c_str());
    }

    // Prepare request for texture target
    camera_status_t status = ACameraDevice_createCaptureRequest(cameraDevice, TEMPLATE_PREVIEW, &request);
    if(status != ACAMERA_OK){
        return false;
    }

    // Enable ImageReader example in CMakeLists.txt. This will additionally
    // make image data available in imageCallback().
    imageReader = createYUVReader();
    imageWindow = createSurface(imageReader);
    ANativeWindow_acquire(imageWindow);
    ACameraOutputTarget_create(imageWindow, &imageTarget);
    status = ACaptureRequest_addTarget(request, imageTarget);
    if(status != ACAMERA_OK){
        return false;
    }

    // Prepare outputs for session
    status = ACaptureSessionOutputContainer_create(&outputs);
    if(status != ACAMERA_OK){
        return false;
    }
    status = ACaptureSessionOutput_create(imageWindow, &imageOutput);
    if(status != ACAMERA_OK){
        return false;
    }
    status = ACaptureSessionOutputContainer_add(outputs, imageOutput);
    if(status != ACAMERA_OK){
        return false;
    }

    // Create the session
    static ACameraCaptureSession_stateCallbacks sessionStateCallbacks {
            .context = nullptr,
            .onActive = onSessionActive,
            .onReady = onSessionReady,
            .onClosed = onSessionClosed
    };
    status = ACameraDevice_createCaptureSession(cameraDevice, outputs, &sessionStateCallbacks, &textureSession);
    if(status != ACAMERA_OK){
        return false;
    }

    // Start capturing continuously
    status = ACameraCaptureSession_setRepeatingRequest(textureSession, &captureCallbacks, 1, &request, nullptr);
    if(status != ACAMERA_OK){
        return false;
    }
    return true;
}

namespace eagleeye{
AndroidCameraNode::AndroidCameraNode(){
    m_is_camera_open = false;
    this->setNumberOfOutputSignals(2);  // image signal, timestamp signal
    EAGLEEYE_MONITOR_VAR(std::string, setCameraFacing, getCameraFacing, "camera","","");

    this->setOutputPort(new ImageSignal<Array<unsigned char,3>>(),0);
    this->getOutputPort(0)->setSignalType(EAGLEEYE_SIGNAL_BGR_IMAGE);

    ImageSignal<double>* timestamp_sig = new ImageSignal<double>();
    Matrix<double> timestamp(1,1);
    timestamp_sig->setData(timestamp);
    this->setOutputPort(timestamp_sig,1);
    this->getOutputPort(1)->setSignalType(EAGLEEYE_SIGNAL_TIMESTAMP);

    m_camera_facing = "back";
    cameraFacing = m_camera_facing;
    m_timestamp = 0.0;
}

AndroidCameraNode::~AndroidCameraNode(){
    exitCamera();
}

void AndroidCameraNode::executeNodeInfo(){
    // 执行一次拉去一帧
    if(!m_is_camera_open){
        // 打开摄像头
        m_is_camera_open = initCam();
    }
    if(!m_is_camera_open){
        EAGLEEYE_LOGE("Camera not open.");
        exitCamera();
        return;
    }

    // 等待相机预览流队列
    std::unique_lock<std::mutex> locker(androidCameraMu);
    while(androidCameraQueue.size() == 0){
        androidCameraCond.wait(locker);
        if(androidCameraQueue.size() > 0){
            break;
        }
    }

    Matrix<Array<unsigned char, 3>> data = androidCameraQueue.front();	
    androidCameraQueue.pop();
    locker.unlock();

    ImageSignal<Array<unsigned char,3>>* output_img_signal = (ImageSignal<Array<unsigned char,3>>*)(this->getOutputPort(0));
    output_img_signal->setData(data);
    ImageSignal<double>* output_timestamp_signal = (ImageSignal<double>*)(this->getOutputPort(1));
    output_timestamp_signal->getData().at(0,0) = m_timestamp;

    m_timestamp += 1.0;
}

void AndroidCameraNode::setCameraFacing(std::string facing){
    if(facing != "back" && facing != "front"){
        EAGLEEYE_LOGE("Camera only support back/front");
        return;
    }

    if(m_camera_facing == facing){
        // 切换目标相机于当前激活的相同
        EAGLEEYE_LOGD("Camera same, dont need switch.");
        return;
    }

    if(m_is_camera_open){
        // 关闭相机，并重制状态
        exitCamera();
        m_is_camera_open = false;
    }

    this->m_camera_facing = facing;
    cameraFacing = this->m_camera_facing;
    modified();
}

void AndroidCameraNode::getCameraFacing(std::string& facing){
    facing = this->m_camera_facing;
}

void AndroidCameraNode::processUnitInfo(){
    Superclass::processUnitInfo();
    modified();
}
}

#endif