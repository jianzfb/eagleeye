#include "eagleeye/render/RenderContext.h"
#include "eagleeye/common/EagleeyeLog.h"

namespace eagleeye
{
RenderContext::RenderContext(){
    this->m_ScreenW = 0;
    this->m_ScreenH = 0;
}
RenderContext::~RenderContext(){

}

void RenderContext::onCreated(){
	glClearColor(1.0f,1.0f,1.0f, 1.0f);
}

void RenderContext::onChanged(int width, int height){
    EAGLEEYE_LOGD("set screen width %d height %d", width, height);
	glViewport(0, 0, width, height);
	m_ScreenW = width;
	m_ScreenH = height;
}

void RenderContext::onMouse(int mouse_x, int mouse_y, int mouse_flag){
    // 回调控制
}


int RenderContext::getScreenW(){
    return this->m_ScreenW;
}

int RenderContext::getScreenH(){
    return this->m_ScreenH;
}    
} // namespace eagleeye
