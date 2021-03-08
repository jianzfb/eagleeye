#include "eagleeye/processnode/YUVConvertNode.h"
#include "eagleeye/framework/pipeline/YUVSignal.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/common/EagleeyeYUV.h"

namespace eagleeye
{
YUVConvertNode::YUVConvertNode(YUVConvertType convert_type){
    // 设置输出端口（拥有1个输出端口）
    this->setNumberOfOutputSignals(1);
    this->setOutputPort(new ImageSignal<Array<unsigned char, 3>>, 0);

    // 设置输入端口（拥有1个输入端口）
	this->setNumberOfInputSignals(1);

    this->m_convert_type = convert_type;
}   

YUVConvertNode::~YUVConvertNode(){

}

void YUVConvertNode::executeNodeInfo(){
    YUVSignal* input_sig = (YUVSignal*)(this->getInputPort(0));
    Blob yuv_data = input_sig->getData();
    int width = input_sig->getWidth();
    int height = input_sig->getHeight();

    // I420
    if(input_sig->getSignalValueType() == EAGLEEYE_YUV_I420){
        unsigned char* yuv_ptr = (unsigned char*)yuv_data.cpu();
        if(m_convert_type == YUV2RGB){
            Matrix<Array<unsigned char, 3>> data = eagleeye_I420_to_RGB(yuv_ptr, width, height);
            ImageSignal<Array<unsigned char, 3>>* output_sig = (ImageSignal<Array<unsigned char, 3>>*)this->getOutputPort(0);
            output_sig->setData(data);
        }
        else{
            Matrix<Array<unsigned char, 3>> data = eagleeye_I420_to_BGR(yuv_ptr, width, height);
            ImageSignal<Array<unsigned char, 3>>* output_sig = (ImageSignal<Array<unsigned char, 3>>*)this->getOutputPort(0);
            output_sig->setData(data);
        }
    }
    else{
        EAGLEEYE_LOGD("Dont support YUV %d convert.", (int)input_sig->getSignalValueType());
    }

}

} // namespace eagleeye
