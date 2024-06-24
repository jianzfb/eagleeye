#include "eagleeye/render/RenderRegionNode.h"
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/framework/pipeline/AnyPipeline.h"
#include "eagleeye/basic/MatrixMath.h"
#include "eagleeye/basic/Array.h"
namespace eagleeye
{
RenderRegionNode::RenderRegionNode(){
    // 设置输入端口 image signal
    this->setNumberOfInputSignals(1);

    // 设置输入端口
    // 端口0：形状数据
    this->setNumberOfOutputSignals(1);
	this->setOutputPort(new ImageSignal<float>(), 0);
	this->getOutputPort(0)->setSignalType(EAGLEEYE_SIGNAL_RECT);
}
RenderRegionNode::~RenderRegionNode(){

}

void RenderRegionNode::executeNodeInfo(){
    Matrix<Array<unsigned char, 3>> rgb_img;
    Matrix<unsigned char> gray_img;
    unsigned char* img_ptr = NULL;
    int img_height = 0;
    int img_width = 0;
    
    if(this->getInputPort(0)->getSignalType() == EAGLEEYE_SIGNAL_RGB_IMAGE || this->getInputPort(0)->getSignalType() == EAGLEEYE_SIGNAL_BGR_IMAGE){
        // 获得输入信号
        ImageSignal<Array<unsigned char, 3>>* input_img_sig = (ImageSignal<Array<unsigned char, 3>>*)(this->getInputPort(0));
        rgb_img = input_img_sig->getData();
        if(!rgb_img.isContinuous()){
            rgb_img = rgb_img.clone();
        }

        img_ptr = (unsigned char*)rgb_img.dataptr();
        img_height = rgb_img.rows();
        img_width = rgb_img.cols();
    }	
    else if(this->getInputPort(0)->getSignalType() ==  EAGLEEYE_SIGNAL_GRAY_IMAGE){
        // 获得输入信号
        ImageSignal<unsigned char>* input_img_sig = (ImageSignal<unsigned char>*)(this->getInputPort(0));
        gray_img = input_img_sig->getData();
        if(!gray_img.isContinuous()){
            gray_img = gray_img.clone();
        }

        img_ptr = (unsigned char*)gray_img.dataptr();
        img_height = gray_img.rows();
        img_width = gray_img.cols();
    }
    else{
        EAGLEEYE_LOGE("Dont support input port type");
        return;
    }
    EAGLEEYE_LOGD("Image width %d height %d.", img_width, img_height);
    // 计算归一化坐标
	int screen_w = AnyPipeline::getRenderSurfaceW();
	int screen_h = AnyPipeline::getRenderSurfaceH();
	int canvas_x = 0;
	int canvas_y = 0;
	int canvas_w = screen_w;
	int canvas_h = screen_h;
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

	// 输出信号1：渲染区域
	ImageSignal<float>* render_region_sig = (ImageSignal<float>*)(this->getOutputPort(0));
	Matrix<float> render_region(1,4);	// x,y,w,h
	render_region.at(0,0) = img_x0;
	render_region.at(0,1) = img_y0;
	render_region.at(0,2) = img_x1-img_x0;
	render_region.at(0,3) = img_y1-img_y0;
	render_region_sig->setData(render_region);
    return;
}

}//eagleeye