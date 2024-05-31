#include "eagleeye/processnode/AndroidCameraNode.h"
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
#include <libusb/libusb.h>
namespace eagleeye{
USBCameraNode::USBCameraNode(){
    m_is_camera_open = false;
    m_image_format = 1;     // 0: RGB, 1: BGR
    m_is_pull_error = false;
    m_is_init_error = false;

    m_context = NULL;
    m_device_handle = NULL;
}

USBCameraNode::~USBCameraNode(){

}

void USBCameraNode::executeNodeInfo(){
    if(m_context == NULL){
        libusb_context *context = NULL;
        int rc = libusb_init(&context);
        if(rc < 0) {
            EAGLEEYE_LOGE("Init usb error.");
            m_is_init_error = true;
            return;
        }
    }


    libusb_device_handle *dev_handle = NULL;

    libusb_device **devs; //pointer to pointer of device, used to retrieve a list of devices
    ssize_t cnt;
    cnt = libusb_get_device_list(context, &devs); //get the list of devices
    if (cnt < 0) {
        EAGLEEYE_LOGE("Couldnt get device list.");
        m_is_init_error = true;
        return;
    }
    


    // // Assuming we just have one camera connected.
    // libusb_device *dev = devs[0];
 
    // rc = libusb_open(dev, &dev_handle); //open the device
    // if(rc != 0) {
    //     std::cerr << "Open Error " << rc << std::endl;
    //     return -1;
    // }
 
    // // Assume we know the endpoint is 1.
    // unsigned char buffer[640*480*3]; //assuming 640x480 @ 24-bit color
    // rc = libusb_claim_interface(dev_handle, 0);
 
    // if(rc < 0) {
    //     std::cerr << "Claim Interface Error " << rc << std::endl;
    //     return -1;
    // }
 
    // rc = libusb_bulk_transfer(dev_handle, (1 | LIBUSB_ENDPOINT_IN), buffer, sizeof(buffer), NULL, 5000);
    // if(rc < 0) {
    //     std::cerr << "Bulk Transfer Error " << rc << std::endl;
    //     return -1;
    // }
 
    // libusb_release_interface(dev_handle, 0);
 
    // cv::Mat frame(480, 640, CV_8UC3, buffer);
    // cv::imshow("Frame", frame);
    // cv::waitKey(0);
 
    // libusb_free_device_list(devs, 1); //free the list, unref the devices in it
    // libusb_close(dev_handle);
    // libusb_exit(context);
}

void USBCameraNode::processUnitInfo(){
    Superclass::processUnitInfo();
    modified();
}

void USBCameraNode::setCameraId(std::string camera_id){
    m_camera_id = camera_i;
    modified();
}

void USBCameraNode::getCameraId(std::string& camera_id){
    camera_id = m_camera_id; 
}

void USBCameraNode::setImageFormat(int image_format){
    m_image_format = image_format;
    modified();
}

void USBCameraNode::getImageFormat(int& image_format){
    image_format = m_image_format;
}
}