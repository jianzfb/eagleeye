#include "eagleeye/render/RenderNode.h"
#include "eagleeye/framework/pipeline/AnyPipeline.h"
namespace eagleeye
{ 
RenderNode::RenderNode(){
    this->m_screen_w = 0;
    this->m_screen_h = 0;

    this->m_canvas_x = 0;
    this->m_canvas_y = 0;
    this->m_canvas_w = 0;
    this->m_canvas_h = 0;

    this->setNodeCategory(RENDER);
}   

RenderNode::~RenderNode(){

} 

void RenderNode::init(){
    //EAGLEEYE_LOGD("RenderNode init");
    if(AnyPipeline::getRenderContext()->getInit() == false)
    {
        Superclass::resetInitOnce();
    }
    Superclass::init();
    glClearColor(1.0f,1.0f,1.0f, 1.0f);
}

bool RenderNode::start(){
    Superclass::start();
    AnyPipeline::getRenderContext()->setInit(true);
    return true;
}

int RenderNode::getScreenW(){
    return AnyPipeline::getRenderSurfaceW();
}

int RenderNode::getScreenH(){
    return AnyPipeline::getRenderSurfaceH();
}

int RenderNode::getCanvasW(){
    return this->m_canvas_w;
}

int RenderNode::getCanvasH(){
    return this->m_canvas_h;
}

int RenderNode::getCanvasX(){
    return this->m_canvas_x;
}

int RenderNode::getCanvasY(){
    return this->m_canvas_y;
}

void RenderNode::setCanvas(int x, int y, int w, int h){
    this->m_canvas_x = x;
    this->m_canvas_y = y;
    this->m_canvas_w = w;
    this->m_canvas_h = h;
}

RenderContext* RenderNode::getRenderContext(){
    return AnyPipeline::getRenderContext();
}

void RenderNode::getMouse(int& mouse_x, int& mouse_y, int& mouse_action){
    AnyPipeline::getRenderContext()->getMouse(mouse_x, mouse_y, mouse_action);
}

} // namespace eagleeye
