#ifndef _EAGLEEYE_IMAGESHOW_H_
#define _EAGLEEYE_IMAGESHOW_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/render/RenderNode.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include <GLES3/gl3.h>


namespace eagleeye{
class ImageShow:public RenderNode{
public:
    typedef ImageShow                   Self;
    typedef RenderNode                  Superclass;

    EAGLEEYE_CLASSIDENTITY(ImageShow);

    ImageShow();
    virtual ~ImageShow();

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

    /**
     * @brief init gl and pipeline
     */ 
    virtual void init();

    /**
     * @brief init render data(VBO, TEXTURE, ...)
     */ 
    virtual void build();

    /**
     * @brief destroy render resource
     */ 
    virtual void destroy();

private:
    ImageShow(const ImageShow&);
    void operator=(const ImageShow&);
    
    GLuint m_TextureId;
	GLuint m_VboIds[3];
};    
} // namespace eagleeye


#endif