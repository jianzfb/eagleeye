#include "eagleeye/render/RenderContext.h"
#include "eagleeye/common/EagleeyeLog.h"
#include <math.h>

namespace eagleeye
{
RenderContext::RenderContext(){
    this->m_ScreenW = 0;
    this->m_ScreenH = 0;

    this->m_rotate = 0;
    this->m_mirror = false;
}
RenderContext::~RenderContext(){

}

void RenderContext::onCreated(){
	glClearColor(1.0f,1.0f,1.0f, 1.0f);
}

void RenderContext::onChanged(int width, int height, int rotate, bool mirror){
    EAGLEEYE_LOGD("set screen width %d height %d (rotate %d, mirror %d)", width, height, rotate, (int)mirror);
    if(width > 0){
        m_ScreenW = width;
    }
    if(height > 0){
        m_ScreenH = height;
    }
    if(m_ScreenW > 0 && m_ScreenH > 0){
        glViewport(0, 0, m_ScreenW, m_ScreenH);
    }
	
    if(rotate >= 0){
        this->m_rotate = rotate;    // 顺时针
    }

    this->m_mirror = mirror;
}

void RenderContext::onMouse(int mouse_x, int mouse_y, int mouse_flag){
    // 回调控制
}

int RenderContext::getScreenW(){
    int screen_w = 0;
    switch (m_rotate)
    {
    case 0:
    case 180:
        screen_w = this->m_ScreenW;
        break;
    case 90:
    case 270:
        screen_w = this->m_ScreenH;
        break; 
    default:
        screen_w = this->m_ScreenW; 
        break;
    }
    return screen_w;
}

int RenderContext::getScreenH(){
    int screen_h = 0;
    switch (m_rotate)
    {
    case 0:
    case 180:
        screen_h = this->m_ScreenH;
        break;
    case 90:
    case 270:
        screen_h = this->m_ScreenW;
        break; 
    default:
        screen_h = this->m_ScreenH; 
        break;
    }
    return screen_h;
}

void RenderContext::getXY(float& x, float& y){
    // mirror
    if(this->m_mirror){
        x = -x;
    }
    
    // rotate
    float r = ((360.0f-m_rotate) / 360.0f) * 2 * 3.1415926f;
    float xn = x * cos(r) + y * sin(r);
    float yn = -x * sin(r) + y * cos(r);

    x = xn;
    y = yn;
}
} // namespace eagleeye
