#ifndef _EAGLEEYE_RENDERNODE_H_
#define _EAGLEEYE_RENDERNODE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/render/RenderBase.h"
#include <GLES3/gl3.h>


namespace eagleeye
{
class RenderNode: public AnyNode, public RenderBase{
public:
    typedef RenderNode                      Self;
    typedef AnyNode                         Superclass;

    EAGLEEYE_CLASSIDENTITY(RenderNode);

    RenderNode();
    virtual ~RenderNode();

    int getScreenW();
    int getScreenH();

    void setCanvas(int x, int y, int w, int h);
    int getCanvasX();
    int getCanvasY();
    int getCanvasW();
    int getCanvasH();

    /**
     * @brief init gl 
     */ 
    virtual void init();

protected:
    int m_screen_w;
    int m_screen_h;

    int m_canvas_x;
    int m_canvas_y;
    int m_canvas_w;
    int m_canvas_h;

private:
    RenderNode(const RenderNode&);
    void operator=(const RenderNode&);
}; 
} // namespace eagleeye

#endif