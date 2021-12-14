#include "eagleeye/processnode/ImageTransformNode.h"
#include "eagleeye/basic/MatrixMath.h"

namespace eagleeye{
ImageTransformNode::ImageTransformNode(std::vector<float> region, std::vector<int> size)
    :m_default_region(region),m_default_size(size){
    // 设置输出端口（拥有1个输出端口）
    this->setNumberOfOutputSignals(1);
    // this->setOutputPort(new OutputPort_OUTPUT_IMAGE_Type(), OUTPUT_PORT_OUTPUT_IMAGE);
    // this->m_is_crop = is_crop;
	// // 设置输入端口
    // if(this->m_is_crop){
    //     // 需要接收裁切框输入
    //     this->setNumberOfInputSignals(2);
    // }
    // else{
    //     this->setNumberOfInputSignals(1);
    // }
	
    EAGLEEYE_MONITOR_VAR(int, setMinSize, getMinSize, "minsize", "16", "1024");
    this->m_min_size = -1;
}  

ImageTransformNode::~ImageTransformNode(){

}

void ImageTransformNode::executeNodeInfo(){
    if(this->getInputPort(0)->getSignalType() != EAGLEEYE_SIGNAL_RGB_IMAGE && 
        this->getInputPort(0)->getSignalType() != EAGLEEYE_SIGNAL_BGR_IMAGE){
            EAGLEEYE_LOGE("Input signal 0 type not correct.");
            return;
    }

    if(this->getNumberOfInputSignals() > 1){
        if(this->getInputPort(1)->getSignalType() != EAGLEEYE_SIGNAL_DET &&
            this->getInputPort(1)->getSignalType() != EAGLEEYE_SIGNAL_RECT){
            EAGLEEYE_LOGE("Input signal 1 type not correct.");
            return;
        }
    }

    ImageSignal<Array<unsigned char, 3>>* input_img_sig = (ImageSignal<Array<unsigned char, 3>>*)(this->getInputPort(0));
	ImageSignal<Array<unsigned char, 3>>* output_img_sig = (ImageSignal<Array<unsigned char, 3>>*)(this->getOutputPort(0));

    Matrix<Array<unsigned char,3>> input_img = input_img_sig->getData();
    int img_h = input_img.rows();
    int img_w = input_img.cols();
    Matrix<float> det_result(1,4);
    det_result.at(0,0) = m_default_region[0];
    det_result.at(0,1) = m_default_region[1];
    det_result.at(0,2) = m_default_region[2];
    det_result.at(0,3) = m_default_region[3];
    if(this->getNumberOfInputSignals() > 1){
        ImageSignal<float>* det_sig = (ImageSignal<float>*)this->getInputPort(1);
        det_result = det_sig->getData();
    }
    
    float norm_x = det_result.at(0,0);
    float norm_y = det_result.at(0,1);
    float norm_w = det_result.at(0,2);
    float norm_h = det_result.at(0,3);

    int x0 = (int)(norm_x * img_w + 0.5f);
    int y0 = (int)(norm_y * img_h + 0.5f);
    int x1 = (int)((norm_x+norm_w) * img_w + 0.5f);
    int y1 = (int)((norm_y+norm_h) * img_h + 0.5f);

    x0 = eagleeye_clip(x0, 0, img_w);
    y0 = eagleeye_clip(y0, 0, img_h);
    x1 = eagleeye_clip(x1, 0, img_w);
    y1 = eagleeye_clip(y1, 0, img_h);

    if(x1-x0 <= m_min_size){
        if(x0 <= img_w-m_min_size){
            x1 = x0+m_min_size;
        }
        else{
            x1 = img_w;
            x0 = img_w - m_min_size;
        }
    }
    if(y1-y0 <= m_min_size){
        if(y0 <= img_h-m_min_size){
            y1 = y0+m_min_size;
        }
        else{
            y1 = img_h;
            y0 = img_h - m_min_size;
        }
    }

    // 1.step crop
    Matrix<Array<unsigned char,3>> region_image = input_img(Range(y0, y1), Range(x0,x1)).clone();

    // 2.step resize
    region_image = resize(region_image,m_default_size[0],m_default_size[1],BILINEAR_INTERPOLATION);

    // output
    output_img_sig->setData(region_image);
}

void ImageTransformNode::setMinSize(int size){
    EAGLEEYE_LOGD("set min size %d", size);
    this->m_min_size = size;
}
void ImageTransformNode::getMinSize(int& size){
    EAGLEEYE_LOGD("get min size %d", this->m_min_size);
    size = this->m_min_size;
}

void ImageTransformNode::addInputPort(AnySignal* sig){
    int signal_num = this->getNumberOfInputSignals();
    this->setInputPort(sig, signal_num);
}

void ImageTransformNode::setInputPort(AnySignal* sig,int index){
    if(index >= 2){
        EAGLEEYE_LOGE("Only support input signal 2");
        return;
    }
    if(this->getNumberOfInputSignals() < index+1){
        this->setNumberOfInputSignals(index+1);
    }

    Superclass::setInputPort(sig, index);
    if(index == 0){
        this->setOutputPort(sig->make(), index);
        this->getOutputPort(0)->setSignalType(sig->getSignalType());
    }
}
}