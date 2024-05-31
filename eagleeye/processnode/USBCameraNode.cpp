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
}
USBCameraNode::~USBCameraNode(){

}

void USBCameraNode::executeNodeInfo(){

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