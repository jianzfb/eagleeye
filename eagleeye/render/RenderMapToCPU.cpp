#include "eagleeye/render/RenderMapToCPU.h"
#include "eagleeye/common/EagleeyeBGRARotateHWC.h"
namespace eagleeye
{
RenderMapToCPU::RenderMapToCPU(){
    // EAGLEEYE_SIGNAL_RECT
    this->setNumberOfInputSignals(1);

    // 1个输出端口
    this->setNumberOfOutputSignals(1);
    this->setOutputPort(new ImageSignal<Array<unsigned char, 4>>(), 0);
	this->getOutputPort(0)->setSignalType(EAGLEEYE_SIGNAL_RGBA_IMAGE);
}   

RenderMapToCPU::~RenderMapToCPU(){

} 

void RenderMapToCPU::executeNodeInfo(){
    // 获取输入信号，获得实际渲染区域
    ImageSignal<float>* input_sig = (ImageSignal<float>*)(this->getInputPort(0));
    Matrix<float> render_region = input_sig->getData();

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

    bool enable_flip_up = true;
    int offset_x = 0;
    int offset_y = 0;
    int render_w = canvas_w;
    int render_h = canvas_h;
    if(render_region.rows() > 0 && render_region.cols() > 0){
        // 基于渲染区域，获取
        float x0 = render_region.at(0,0);
        float y0 = render_region.at(0,1);
        float w = render_region.at(0,2);
        float h = render_region.at(0,3);

        offset_x = (int)(x0 + 0.5f);
        offset_x = eagleeye_max(offset_x, 0);
        offset_y = (int)(y0 + 0.5f);
        offset_y = eagleeye_max(offset_y, 0);
        
        render_w = (int)(w + 0.5f);
        int t = render_w + offset_x;
        t = eagleeye_min(t, canvas_w);
        render_w = t - offset_x;

        render_h = (int)(h + 0.5f);
        t = render_h + offset_y;
        t = eagleeye_min(t, canvas_h);
        render_h = t - offset_y;
    }

    EAGLEEYE_LOGD("Screen render x=%d y=%d width=%d height=%d.", offset_x, offset_y, render_w, render_h);

    ImageSignal<Array<unsigned char, 4>>* output_sig = (ImageSignal<Array<unsigned char, 4>>*)(this->getOutputPort(0));
    Matrix<Array<unsigned char, 4>> data = output_sig->getData();
    if(data.rows() != render_h || data.cols() != render_w){
        data = Matrix<Array<unsigned char,4>>(render_h, render_w);
    }
    if(m_temp.rows() != render_h || m_temp.cols() != render_w){
        m_temp = Matrix<Array<unsigned char,4>>(render_h, render_w);
    }

    if(enable_flip_up){
        unsigned char* temp_ptr = (unsigned char*)m_temp.dataptr();
        glReadPixels(offset_x, offset_y, render_w, render_h, GL_RGBA, GL_UNSIGNED_BYTE, temp_ptr);

        unsigned char* ptr = (unsigned char*)(data.dataptr());
        bgra_rotate_hwc(temp_ptr, ptr, render_w, render_h, 180);
    }
    else{
        unsigned char* ptr = (unsigned char*)data.dataptr();
        glReadPixels(offset_x, offset_y, render_w, render_h, GL_RGBA, GL_UNSIGNED_BYTE, ptr);
    }

    output_sig->setData(data);
}
} // namespace eagleeye
