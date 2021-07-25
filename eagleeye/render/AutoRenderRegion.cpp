#include "eagleeye/render/AutoRenderRegion.h"
namespace eagleeye
{
AutoRenderRegion::AutoRenderRegion(){
    // 设置输出端口（拥有1个输出端口）
    this->setNumberOfOutputSignals(1);
	this->setOutputPort(new ImageSignal<float>(), 0);
	this->getOutputPort(0)->setSignalType(EAGLEEYE_SIGNAL_RECT);

    // 设置输入端口
    this->setNumberOfInputSignals(1);
}   
AutoRenderRegion::~AutoRenderRegion(){

} 

void AutoRenderRegion::executeNodeInfo(){
    if(this->getInputPort(0)->getSignalType() != EAGLEEYE_SIGNAL_RGB_IMAGE && 
        this->getInputPort(0)->getSignalType() != EAGLEEYE_SIGNAL_BGR_IMAGE && 
        this->getInputPort(0)->getSignalType() != EAGLEEYE_SIGNAL_RGBA_IMAGE && 
        this->getInputPort(0)->getSignalType() != EAGLEEYE_SIGNAL_BGRA_IMAGE && 
        this->getInputPort(0)->getSignalType() != EAGLEEYE_SIGNAL_GRAY_IMAGE){
        EAGLEEYE_LOGE("Dont support input port type");
        return;
    }

	int img_height = 0;
	int img_width = 0;
	if(this->getInputPort(0)->getSignalType() == EAGLEEYE_SIGNAL_RGB_IMAGE || this->getInputPort(0)->getSignalType() == EAGLEEYE_SIGNAL_BGR_IMAGE){
		// 获得输入信号
		ImageSignal<Array<unsigned char, 3>>* input_img_sig = (ImageSignal<Array<unsigned char, 3>>*)(this->getInputPort(0));
		img_height = input_img_sig->getData().rows();
		img_width = input_img_sig->getData().cols();
	}	
    else if(this->getInputPort(0)->getSignalType() == EAGLEEYE_SIGNAL_RGBA_IMAGE || this->getInputPort(0)->getSignalType() == EAGLEEYE_SIGNAL_BGRA_IMAGE){
		ImageSignal<Array<unsigned char, 4>>* input_img_sig = (ImageSignal<Array<unsigned char, 4>>*)(this->getInputPort(0));
		img_height = input_img_sig->getData().rows();
		img_width = input_img_sig->getData().cols();
    }
    else{
        ImageSignal<unsigned char>* input_img_sig = (ImageSignal<unsigned char>*)(this->getInputPort(0));
		img_height = input_img_sig->getData().rows();
		img_width = input_img_sig->getData().cols();
    }

	// 计算归一化坐标
	int screen_w = this->getScreenW();
	int screen_h = this->getScreenH();
	int canvas_x = this->getCanvasX();
	int canvas_y = this->getCanvasY();
	int canvas_w = this->getCanvasW();
	int canvas_h = this->getCanvasH();
	if(canvas_w == 0 || canvas_h == 0){
		canvas_x = 0;
		canvas_y = 0;
		canvas_w = this->getScreenW();
		canvas_h = this->getScreenH();
	}
	EAGLEEYE_LOGD("Canvas x %d y %d width %d height %d.", canvas_x, canvas_y, canvas_w, canvas_h);
	EAGLEEYE_LOGD("Screen width %d height %d.", screen_w, screen_h);

	// 计算图片显示位置
	bool is_ok = false;
	float scale = float(canvas_w) / float(img_width);
	if(scale * img_height <= canvas_h){
		is_ok = true;
	}
	if(!is_ok){
		scale = float(canvas_h) / float(img_height);
	}
	float scaled_img_width = scale * float(img_width);
	float scaled_img_height = scale * float(img_height);

	float img_x0 = (canvas_w - scaled_img_width)/2.0f + canvas_x;
	float img_y0 = (canvas_h - scaled_img_height)/2.0f + canvas_y;
	float img_x1 = (canvas_w - scaled_img_width)/2.0f + scaled_img_width + canvas_x;
	float img_y1 = (canvas_h - scaled_img_height)/2.0f + scaled_img_height + canvas_y;

	// 渲染区域
	ImageSignal<float>* render_region_sig = (ImageSignal<float>*)(this->getOutputPort(0));
	Matrix<float> render_region(1,4);	// x,y,w,h
	render_region.at(0,0) = img_x0;
	render_region.at(0,1) = img_y0;
	render_region.at(0,2) = img_x1-img_x0;
	render_region.at(0,3) = img_y1-img_y0;
	render_region_sig->setData(render_region);
}

} // namespace eagleeye
