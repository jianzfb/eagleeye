#include "eagleeye/processnode/ImageResizeNode.h"
#include "eagleeye/basic/blob.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/basic/MatrixMath.h"


namespace eagleeye
{
ImageResizeNode::ImageResizeNode(int resize_w, int resize_h, float resize_scale){
    this->m_resize_w = resize_w;
    this->m_resize_h = resize_h;
    this->m_resize_scale = resize_scale;

    // 设置输出端口（拥有1个输出端口）
    this->setNumberOfOutputSignals(1);
    this->setOutputPort(new ImageSignal<Array<unsigned char,3>>, 0);

    // 设置输入端口（拥有1个输入端口）
	this->setNumberOfInputSignals(1);

    EAGLEEYE_MONITOR_VAR(int, setResizeW, getResizeW, "width","100", "1920");
    EAGLEEYE_MONITOR_VAR(int, setResizeH, getResizeH, "height","100", "1920");
}    

ImageResizeNode::~ImageResizeNode(){

}

void ImageResizeNode::executeNodeInfo(){
    if(this->getInputPort(0)->getSignalType() != EAGLEEYE_SIGNAL_IMAGE &&
        this->getInputPort(0)->getSignalType() != EAGLEEYE_SIGNAL_BGR_IMAGE &&
         this->getInputPort(0)->getSignalType() != EAGLEEYE_SIGNAL_RGB_IMAGE){
        EAGLEEYE_LOGE("Dont support signal type.");
        return;
    }

    ImageSignal<Array<unsigned char,3>>* input_sig = (ImageSignal<Array<unsigned char,3>>*)(this->getInputPort(0));
    Matrix<Array<unsigned char,3>> image = input_sig->getData();
    int height = image.rows();
    int width = image.cols();
    
    int target_resize_h = m_resize_h;
    int target_resize_w = m_resize_w;
    if(target_resize_h <= 0 || target_resize_w <= 0){
        target_resize_h = (int)(height * this->m_resize_scale);
        target_resize_w = (int)(width * this->m_resize_scale);
    }

    Matrix<Array<unsigned char,3>> resized_image = resize(image, target_resize_h, target_resize_w, BILINEAR_INTERPOLATION);
    ImageSignal<Array<unsigned char,3>>* output_sig = (ImageSignal<Array<unsigned char,3>>*)(this->getOutputPort(0));
    output_sig->setSignalType(this->getInputPort(0)->getSignalType());
    output_sig->setData(resized_image);
}

void ImageResizeNode::setResizeW(int w){
    this->m_resize_w = w;
    modified();
}

void ImageResizeNode::getResizeW(int& w){
    w = this->m_resize_w;
}

void ImageResizeNode::setResizeH(int h){
    this->m_resize_h = h;
    modified();
}

void ImageResizeNode::getResizeH(int& h){
    h = this->m_resize_h;
}
} // namespace eagleeye
