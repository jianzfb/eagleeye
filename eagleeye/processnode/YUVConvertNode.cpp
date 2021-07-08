#include "eagleeye/processnode/YUVConvertNode.h"
#include "eagleeye/framework/pipeline/YUVSignal.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/common/EagleeyeYUV.h"
#include "eagleeye/common/EagleeyeLog.h"

namespace eagleeye
{
YUVConvertNode::YUVConvertNode(YUVConvertType convert_type){
    // 设置输出端口（拥有1个输出端口）
    this->setNumberOfOutputSignals(1);
    this->setOutputPort(new ImageSignal<Array<unsigned char, 3>>, 0);

    // 设置输入端口（拥有1个输入端口）
	this->setNumberOfInputSignals(1);

    this->m_convert_type = convert_type;

    EAGLEEYE_MONITOR_VAR(int, setConvertType, getConvertType, "converttype","0", "6");
}   

YUVConvertNode::~YUVConvertNode(){

}

void YUVConvertNode::executeNodeInfo(){
    if(this->getInputPort(0)->getSignalType() != EAGLEEYE_SIGNAL_YUV_IMAGE){
        EAGLEEYE_LOGD("Dont support signal type.");
        return;
    }

    YUVSignal* input_sig = (YUVSignal*)(this->getInputPort(0));
    MetaData input_meta;
    Blob yuv_data = input_sig->getData(input_meta);
    int width = input_sig->getWidth();
    int height = input_sig->getHeight();

    // I420
    if(input_sig->getValueType() == EAGLEEYE_YUV_I420){
        unsigned char* yuv_ptr = (unsigned char*)yuv_data.cpu();
        if(m_convert_type == I420ToRGB){
            Matrix<Array<unsigned char, 3>> data = eagleeye_I420_to_RGB(yuv_ptr, width, height);
            ImageSignal<Array<unsigned char, 3>>* output_sig = (ImageSignal<Array<unsigned char, 3>>*)this->getOutputPort(0);
            output_sig->setData(data, input_meta);
            output_sig->setSignalType(EAGLEEYE_SIGNAL_RGB_IMAGE);
        }
        else if(m_convert_type == I420ToBGR){
            Matrix<Array<unsigned char, 3>> data = eagleeye_I420_to_BGR(yuv_ptr, width, height);
            ImageSignal<Array<unsigned char, 3>>* output_sig = (ImageSignal<Array<unsigned char, 3>>*)this->getOutputPort(0);
            output_sig->setData(data, input_meta);
            output_sig->setSignalType(EAGLEEYE_SIGNAL_BGR_IMAGE);
        }
        else{
            EAGLEEYE_LOGD("Conver error (EAGLEEYE_YUV_I420).");
        }
    }
    else if(input_sig->getValueType() == EAGLEEYE_YUV_NV21){
        unsigned char* yuv_ptr = (unsigned char*)yuv_data.cpu();
        if(m_convert_type == NV21ToRGB){
            Matrix<Array<unsigned char, 3>> data = eagleeye_NV21_to_RGB(yuv_ptr, width, height);
            ImageSignal<Array<unsigned char, 3>>* output_sig = (ImageSignal<Array<unsigned char, 3>>*)this->getOutputPort(0);
            output_sig->setData(data, input_meta);
            output_sig->setSignalType(EAGLEEYE_SIGNAL_RGB_IMAGE);
        }
        else if(m_convert_type == NV21ToBGR){
            Matrix<Array<unsigned char, 3>> data = eagleeye_NV21_to_BGR(yuv_ptr, width, height);
            ImageSignal<Array<unsigned char, 3>>* output_sig = (ImageSignal<Array<unsigned char, 3>>*)this->getOutputPort(0);
            output_sig->setData(data, input_meta);
            output_sig->setSignalType(EAGLEEYE_SIGNAL_BGR_IMAGE);
        }
        else{
            EAGLEEYE_LOGD("Conver error (EAGLEEYE_YUV_NV21).");
        }

    }
    else if(input_sig->getValueType() == EAGLEEYE_YUV_NV12){
        unsigned char* yuv_ptr = (unsigned char*)yuv_data.cpu();
        if(m_convert_type == NV12ToRGB){
            Matrix<Array<unsigned char, 3>> data = eagleeye_NV12_to_RGB(yuv_ptr, width, height);
            ImageSignal<Array<unsigned char, 3>>* output_sig = (ImageSignal<Array<unsigned char, 3>>*)this->getOutputPort(0);
            output_sig->setData(data, input_meta);
            output_sig->setSignalType(EAGLEEYE_SIGNAL_RGB_IMAGE);
        }
        else if(m_convert_type == NV12ToBGR){
            Matrix<Array<unsigned char, 3>> data = eagleeye_NV12_to_BGR(yuv_ptr, width, height);
            ImageSignal<Array<unsigned char, 3>>* output_sig = (ImageSignal<Array<unsigned char, 3>>*)this->getOutputPort(0);
            output_sig->setData(data, input_meta);
            output_sig->setSignalType(EAGLEEYE_SIGNAL_BGR_IMAGE);
        }
        else{
            EAGLEEYE_LOGD("Conver error (EAGLEEYE_YUV_NV12).");
        }
    }
    else{
        EAGLEEYE_LOGD("Conver error");
    }
}

void YUVConvertNode::setConvertType(int convert_type){
    this->m_convert_type = (YUVConvertType)(convert_type);
    this->modified();
}

void YUVConvertNode::getConvertType(int& convert_type){
    m_t = (int)m_convert_type;
    convert_type = m_t;
}
} // namespace eagleeye
