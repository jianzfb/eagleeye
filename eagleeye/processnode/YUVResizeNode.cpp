#include "eagleeye/processnode/YUVResizeNode.h"
#include "eagleeye/framework/pipeline/YUVSignal.h"
#include "eagleeye/basic/blob.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "libyuv.h"


namespace eagleeye
{
YUVResizeNode::YUVResizeNode(int resize_w, int resize_h){
    this->m_resize_w = resize_w;
    this->m_resize_h = resize_h;

    // 设置输出端口（拥有1个输出端口）
    this->setNumberOfOutputSignals(1);
    this->setOutputPort(new YUVSignal(), 0);

    // 设置输入端口（拥有1个输入端口）
	this->setNumberOfInputSignals(1);
}    

YUVResizeNode::~YUVResizeNode(){

}

void YUVResizeNode::executeNodeInfo(){
    YUVSignal* input_sig = (YUVSignal*)(this->getInputPort(0));

    // 输入yuv数据
    Blob input_blob = input_sig->getData();
    unsigned char* input_ptr = (unsigned char*)(input_blob.cpu());
    int height = input_sig->getHeight();
    int width = input_sig->getWidth();

    // 输出yuv数据
    Blob output_blob(sizeof(unsigned char)*(m_resize_h*m_resize_w + (m_resize_h/2)*(m_resize_w/2) + (m_resize_h/2)*(m_resize_w/2)),
            Aligned(64), 
            EagleeyeRuntime(EAGLEEYE_CPU));
    unsigned char* output_ptr = (unsigned char*)(output_blob.cpu());

    if(input_sig->getSignalValueType() == EAGLEEYE_YUV_I420){
        uint8_t* src_i420_y_data = (uint8_t*)input_ptr;
        uint8_t* src_i420_u_data = src_i420_y_data + width * height;
        uint8_t* src_i420_v_data = src_i420_u_data + (int)(width * height * 0.25);

        uint8_t* dst_i420_y_data = (uint8_t*)output_ptr;
        uint8_t* dst_i420_u_data = dst_i420_y_data + m_resize_w * m_resize_h;
        uint8_t* dst_i420_v_data = dst_i420_u_data + (int)(m_resize_w * m_resize_h * 0.25);
        libyuv::I420Scale((const uint8_t *) src_i420_y_data, width,
                    (const uint8_t *) src_i420_u_data, width >> 1,
                    (const uint8_t *) src_i420_v_data, width >> 1,
                    width, height,
                    (uint8_t *) dst_i420_y_data, m_resize_w,
                    (uint8_t *) dst_i420_u_data, m_resize_w >> 1,
                    (uint8_t *) dst_i420_v_data, m_resize_w >> 1,
                    m_resize_w, m_resize_h,
                    libyuv::kFilterBilinear);
    }
    else{
        EAGLEEYE_LOGD("Dont support YUV %d resize.", (int)input_sig->getSignalValueType());
    }

    YUVSignal* output_sig = (YUVSignal*)(this->getOutputPort(0));
    output_sig->setData(output_blob);
}
} // namespace eagleeye
