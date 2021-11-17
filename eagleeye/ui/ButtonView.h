#ifndef _EAGLEEYE_BUTTONVIEW_H_
#define _EAGLEEYE_BUTTONVIEW_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/render/RenderNode.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include <GLES3/gl3.h>

namespace eagleeye
{
class ButtongView:public RenderNode{
public:
    typedef ButtongView                   Self;
    typedef RenderNode                  Superclass;

    EAGLEEYE_CLASSIDENTITY(ButtongView);

    ButtongView();
    virtual ~ButtongView();

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

    /**
     * @brief init gl 
     */ 
    virtual void init();    

private:
    ButtongView(const ButtongView&);
    void operator=(const ButtongView&);

    float m_button_h;
    float m_button_w;

    float m_button_x;
    float m_button_y;
};    
} // namespace eagleeye

#endif