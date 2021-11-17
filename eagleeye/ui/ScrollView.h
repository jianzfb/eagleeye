#ifndef _EAGLEEYE_SCROLLVIEW_H_
#define _EAGLEEYE_SCROLLVIEW_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/render/RenderNode.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include <GLES3/gl3.h>

namespace eagleeye
{
class ScrollView:public RenderNode{
public:
    typedef ScrollView                   Self;
    typedef RenderNode                  Superclass;

    EAGLEEYE_CLASSIDENTITY(ScrollView);

    ScrollView(int direction=0);
    virtual ~ScrollView();

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

    /**
     * @brief init gl 
     */ 
    virtual void init();

    void setViewNum(int num);
    void setViewHW(float h, float w);

    void setOffset(float x_v, float y_v);

private:
    ScrollView(const ScrollView&);
    void operator=(const ScrollView&);

    int m_direction;
    int m_view_num;
    float m_view_h;
    float m_view_w;
    float m_padding_ratio;
    float m_x_offset;
    float m_y_offset;

    GLfloat m_default_color[4];
    GLfloat m_select_color[4];
    GLfloat m_lock_color[4];

    float m_last_x;
    float m_last_y;

    float m_last_x_offset;
    float m_last_y_offset;
};
} // namespace eagleeye

#endif
