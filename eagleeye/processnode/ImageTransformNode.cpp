#include "eagleeye/processnode/ImageTransformNode.h"
#include "eagleeye/basic/MatrixMath.h"

namespace eagleeye{
ImageTransformNode::ImageTransformNode(bool is_crop){
    // 设置输出端口（拥有1个输出端口）
    this->setNumberOfOutputSignals(1);
    this->setOutputPort(new OutputPort_OUTPUT_IMAGE_Type(), OUTPUT_PORT_OUTPUT_IMAGE);
    this->m_is_crop = is_crop;
	// 设置输入端口
    if(this->m_is_crop){
        // 需要接收裁切框输入
        this->setNumberOfInputSignals(2);
    }
    else{
        this->setNumberOfInputSignals(1);
    }
	
    EAGLEEYE_MONITOR_VAR(float, setScale, getScale,"scale","0.1","2.0");
    EAGLEEYE_MONITOR_VAR(int, setMinSize, getMinSize, "minsize", "16", "1024");
    this->m_scale = 1.0f;
    this->m_min_size = -1;
}  

ImageTransformNode::~ImageTransformNode(){

}

void ImageTransformNode::executeNodeInfo(){
    ImageSignal<Array<unsigned char, 3>>* input_img_sig = (ImageSignal<Array<unsigned char, 3>>*)(this->getInputPort(0));
	ImageSignal<Array<unsigned char, 3>>* output_img_sig = (ImageSignal<Array<unsigned char, 3>>*)(this->getOutputPort(0));

    EAGLEEYE_LOGD("in image transformnode exe");
    Matrix<Array<unsigned char,3>> input_img = input_img_sig->getData();
    int rows = input_img.rows();
    int cols = input_img.cols();

    Matrix<Array<unsigned char,3>> processed_img = input_img;
    if(this->m_is_crop){
        ImageSignal<int>* crop_sig = dynamic_cast<ImageSignal<int>*>(this->getInputPort(1));
        Matrix<int> crop_bbox = crop_sig->getData();
        int x = crop_bbox.at(0,0);
        x = eagleeye_max(x,0);
        int y = crop_bbox.at(0,1);
        y = eagleeye_max(y,0);
        int width = crop_bbox.at(0,2);
        if(x + width >= cols){
            width = cols - x;
        }
        int height = crop_bbox.at(0,3);
        if(y + height >= rows){
            height = rows - y;
        }
        processed_img = input_img(Range(y,y+height),Range(x,x+width)).clone();
    }

    int processed_rows = processed_img.rows();
    int processed_cols = processed_img.cols();
    EAGLEEYE_LOGD("process crop size %d %d",processed_rows,processed_cols);

    float scale = this->m_scale;
    if(this->m_min_size > 0){
        int small_side = processed_rows < processed_cols ? processed_rows : processed_cols;
        scale = float(this->m_min_size) / float(small_side);
    }

    if(abs(scale - 1.0f) > 0.000000001f){
        int scaled_rows = int(scale*processed_rows + 0.5f);
        int scaled_cols = int(scale*processed_cols + 0.5f);
        processed_img = resize(processed_img,scaled_rows,scaled_cols,BILINEAR_INTERPOLATION);
    }

    EAGLEEYE_LOGD("process scaled size %d %d",processed_img.rows(),processed_img.cols());
    output_img_sig->setData(processed_img);
}

void ImageTransformNode::setScale(float scale){
    EAGLEEYE_LOGD("set scale %f", scale);
    this->m_scale = scale;
}
void ImageTransformNode::getScale(float& scale){
    EAGLEEYE_LOGD("get scale %f", this->m_scale);
    scale = this->m_scale;
}

void ImageTransformNode::setMinSize(int size){
    EAGLEEYE_LOGD("set min size %d", size);
    this->m_min_size = size;
}
void ImageTransformNode::getMinSize(int& size){
    EAGLEEYE_LOGD("get min size %d", this->m_min_size);
    size = this->m_min_size;
}
}