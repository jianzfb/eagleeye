#include "eagleeye/processnode/YUVResizeNode.h"
#include "eagleeye/framework/pipeline/YUVSignal.h"
#include "eagleeye/basic/blob.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "libyuv.h"


namespace eagleeye
{
YUVResizeNode::YUVResizeNode(int resize_w, int resize_h, float resize_scale){
    this->m_resize_w = resize_w;
    this->m_resize_h = resize_h;
    this->m_resize_scale = resize_scale;

    // 设置输出端口（拥有1个输出端口）
    this->setNumberOfOutputSignals(1);
    this->setOutputPort(new YUVSignal(), 0);

    // 设置输入端口（拥有1个输入端口）
	this->setNumberOfInputSignals(1);

    EAGLEEYE_MONITOR_VAR(int, setResizeW, getResizeW, "width","100", "1920");
    EAGLEEYE_MONITOR_VAR(int, setResizeH, getResizeH, "height","100", "1920");
}    

YUVResizeNode::~YUVResizeNode(){

}

void YUVResizeNode::executeNodeInfo(){
    if(this->getInputPort(0)->getSignalType() != EAGLEEYE_SIGNAL_YUV_IMAGE){
        EAGLEEYE_LOGE("Dont support signal type.");
        return;
    }

    YUVSignal* input_sig = (YUVSignal*)(this->getInputPort(0));

    // 输入yuv数据
    MetaData input_meta;
    Blob input_blob = input_sig->getData(input_meta);
    unsigned char* input_ptr = (unsigned char*)(input_blob.cpu());
    int height = input_sig->getHeight();
    int width = input_sig->getWidth();
    int target_resize_h = m_resize_h;
    int target_resize_w = m_resize_w;
    if(target_resize_h <= 0 || target_resize_w <= 0){
        target_resize_h = (int)(height * this->m_resize_scale);
        target_resize_w = (int)(width * this->m_resize_scale);
    }

    // 输出yuv数据
    Blob output_blob((target_resize_h*target_resize_w + (target_resize_h/2)*(target_resize_w/2) + (target_resize_h/2)*(target_resize_w/2)),
            TypeTrait<unsigned char>::type,
            CPU_BUFFER,
            Aligned(64));
    unsigned char* output_ptr = output_blob.cpu<unsigned char>();

    if(input_sig->getValueType() == EAGLEEYE_YUV_I420){
        uint8_t* src_i420_y_data = (uint8_t*)input_ptr;
        uint8_t* src_i420_u_data = src_i420_y_data + width * height;
        uint8_t* src_i420_v_data = src_i420_u_data + (int)(width * height * 0.25);

        uint8_t* dst_i420_y_data = (uint8_t*)output_ptr;
        uint8_t* dst_i420_u_data = dst_i420_y_data + target_resize_w * target_resize_h;
        uint8_t* dst_i420_v_data = dst_i420_u_data + (int)(target_resize_w * target_resize_h * 0.25);
        libyuv::I420Scale((const uint8_t *) src_i420_y_data, width,
                    (const uint8_t *) src_i420_u_data, width >> 1,
                    (const uint8_t *) src_i420_v_data, width >> 1,
                    width, height,
                    (uint8_t *) dst_i420_y_data, target_resize_w,
                    (uint8_t *) dst_i420_u_data, target_resize_w >> 1,
                    (uint8_t *) dst_i420_v_data, target_resize_w >> 1,
                    target_resize_w, target_resize_h,
                    libyuv::kFilterBilinear);
    }
    else{
        EAGLEEYE_LOGD("Dont support YUV %d resize.", (int)input_sig->getValueType());
    }

    YUVSignal* output_sig = (YUVSignal*)(this->getOutputPort(0));
    MetaData output_meta = input_meta;
    output_meta.rows = target_resize_h;
    output_meta.cols = target_resize_w;
    output_sig->setData(output_blob, output_meta);
}

void YUVResizeNode::setResizeW(int w){
    this->m_resize_w = w;
    modified();
}

void YUVResizeNode::getResizeW(int& w){
    w = this->m_resize_w;
}

void YUVResizeNode::setResizeH(int h){
    this->m_resize_h = h;
    modified();
}

void YUVResizeNode::getResizeH(int& h){
    h = this->m_resize_h;
}
} // namespace eagleeye
