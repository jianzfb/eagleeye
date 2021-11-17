#include "eagleeye/processnode/CropNode.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
namespace eagleeye
{
CropNode::CropNode(){
    // 设置输入端口
    // 0: 输入图像
    // 1: 输入图像人脸检测
	this->setNumberOfInputSignals(2);

    // 设置输出端口（拥有1个输出端口）
    this->setNumberOfOutputSignals(1);
}   

CropNode::~CropNode(){

}

void CropNode::executeNodeInfo(){
    if(this->getInputPort(0)->getSignalType() != EAGLEEYE_SIGNAL_RGB_IMAGE && 
        this->getInputPort(0)->getSignalType() != EAGLEEYE_SIGNAL_RGBA_IMAGE && 
        this->getInputPort(0)->getSignalType() != EAGLEEYE_SIGNAL_BGR_IMAGE && 
        this->getInputPort(0)->getSignalType() != EAGLEEYE_SIGNAL_BGRA_IMAGE && 
        this->getInputPort(0)->getSignalType() != EAGLEEYE_SIGNAL_GRAY_IMAGE){
            EAGLEEYE_LOGE("Input signal 0 type not correct.");
            return;
    }
    if(this->getInputPort(1)->getSignalType() != EAGLEEYE_SIGNAL_DET){
        EAGLEEYE_LOGE("Input signal 1 type not correct.");
        return;
    }

    ImageSignal<float>* det_sig = (ImageSignal<float>*)this->getInputPort(1);
    Matrix<float> det_result = det_sig->getData();
    float norm_x = det_result.at(0,0);
    float norm_y = det_result.at(0,1);
    float norm_w = det_result.at(0,2);
    float norm_h = det_result.at(0,3);

    if(this->getInputPort(0)->getSignalType() == EAGLEEYE_SIGNAL_RGB_IMAGE || 
        this->getInputPort(0)->getSignalType() == EAGLEEYE_SIGNAL_BGR_IMAGE){
        ImageSignal<Array<unsigned char,3>>* input_sig = (ImageSignal<Array<unsigned char,3>>*)this->getInputPort(0);
        Matrix<Array<unsigned char,3>> image = input_sig->getData();
        int img_h = image.rows();
        int img_w = image.cols();

        int x0 = (int)(norm_x * img_w + 0.5f);
        int y0 = (int)(norm_y * img_h + 0.5f);
        int x1 = (int)((norm_x+norm_w) * img_w + 0.5f);
        int y1 = (int)((norm_y+norm_h) * img_h + 0.5f);

        x0 = eagleeye_clip(x0, 0, img_w);
        y0 = eagleeye_clip(y0, 0, img_h);
        x1 = eagleeye_clip(x1, 0, img_w);
        y1 = eagleeye_clip(y1, 0, img_h);
        if(x0 >= x1 || y0 >= y1){
            EAGLEEYE_LOGE("Region incorrect(x0 >= x1 or y0 >= y1).");
            return;
        }

        Matrix<Array<unsigned char,3>> region_image = image(Range(y0, y1), Range(x0,x1)).clone();
        ImageSignal<Array<unsigned char,3>>* output_sig = (ImageSignal<Array<unsigned char,3>>*)this->getOutputPort(0);
        output_sig->setData(region_image);
    }
    else if(this->getInputPort(0)->getSignalType() == EAGLEEYE_SIGNAL_RGBA_IMAGE || 
        this->getInputPort(0)->getSignalType() == EAGLEEYE_SIGNAL_BGRA_IMAGE){
        ImageSignal<Array<unsigned char,4>>* input_sig = (ImageSignal<Array<unsigned char,4>>*)this->getInputPort(0);
        Matrix<Array<unsigned char,4>> image = input_sig->getData();
        int img_h = image.rows();
        int img_w = image.cols();

        int x0 = (int)(norm_x * img_w + 0.5f);
        int y0 = (int)(norm_y * img_h + 0.5f);
        int x1 = (int)((norm_x+norm_w) * img_w + 0.5f);
        int y1 = (int)((norm_y+norm_h) * img_h + 0.5f);

        x0 = eagleeye_clip(x0, 0, img_w);
        y0 = eagleeye_clip(y0, 0, img_h);
        x1 = eagleeye_clip(x1, 0, img_w);
        y1 = eagleeye_clip(y1, 0, img_h);

        if(x0 >= x1 || y0 >= y1){
            EAGLEEYE_LOGE("Region incorrect(x0 >= x1 or y0 >= y1).");
            return;
        }

        Matrix<Array<unsigned char,4>> region_image = image(Range(y0, y1), Range(x0,x1)).clone();
        ImageSignal<Array<unsigned char,4>>* output_sig = (ImageSignal<Array<unsigned char,4>>*)this->getOutputPort(0);
        output_sig->setData(region_image);
    }
    else{
        ImageSignal<unsigned char>* input_sig = (ImageSignal<unsigned char>*)this->getInputPort(0);
        Matrix<unsigned char> image = input_sig->getData();
        int img_h = image.rows();
        int img_w = image.cols();

        int x0 = (int)(norm_x * img_w + 0.5f);
        int y0 = (int)(norm_y * img_h + 0.5f);
        int x1 = (int)((norm_x+norm_w) * img_w + 0.5f);
        int y1 = (int)((norm_y+norm_h) * img_h + 0.5f);

        x0 = eagleeye_clip(x0, 0, img_w);
        y0 = eagleeye_clip(y0, 0, img_h);
        x1 = eagleeye_clip(x1, 0, img_w);
        y1 = eagleeye_clip(y1, 0, img_h);

        if(x0 >= x1 || y0 >= y1){
            EAGLEEYE_LOGE("Region incorrect(x0 >= x1 or y0 >= y1).");
            return;
        }

        Matrix<unsigned char> region_image = image(Range(y0, y1), Range(x0,x1)).clone();
        ImageSignal<unsigned char>* output_sig = (ImageSignal<unsigned char>*)this->getOutputPort(0);
        output_sig->setData(region_image);
    }
}

void CropNode::addInputPort(AnySignal* sig){
    int signal_num = this->getNumberOfInputSignals();
    this->setInputPort(sig, signal_num);
}

void CropNode::setInputPort(AnySignal* sig,int index){
    if(index >= 2){
        EAGLEEYE_LOGE("Only support input signal 2");
        return;
    }
    Superclass::setInputPort(sig, index);

    if(index == 0){
        this->setOutputPort(sig->make(), index);
        this->getOutputPort(0)->setSignalType(sig->getSignalType());
    }
}
} // namespace eagleeye
