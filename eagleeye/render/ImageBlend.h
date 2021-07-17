#ifndef _EAGLEEYE_IMAGEBLEND_H_
#define _EAGLEEYE_IMAGEBLEND_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include "eagleeye/framework/pipeline/AnyNode.h"
#include "eagleeye/render/RenderNode.h"
#include "eagleeye/basic/Array.h"
#include "eagleeye/basic/Matrix.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include <GLES3/gl3.h>


namespace eagleeye{
class ImageBlend:public RenderNode{
public:
    typedef ImageBlend                      Self;
    typedef RenderNode                      Superclass;

    EAGLEEYE_CLASSIDENTITY(ImageBlend);

    ImageBlend();
    virtual ~ImageBlend();

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

    void setLowThresh(float thres);
    void getLowThresh(float& thres);

    void setHighThresh(float thres);
    void getHighThresh(float& thres);

    void setNonlinearMap(int ok);
    void getNonelinearMap(int& ok);

private:
    ImageBlend(const ImageBlend&);
    void operator=(const ImageBlend&);
    
    GLuint m_TextureId;
    GLuint m_MaskTextureId;
    GLuint m_BackgrounndTextureId;
	GLuint m_VboIds[3];

    float m_low_thres;
    float m_high_thres;
    int m_using_nonlinear_map;
};    
} // namespace eagleeye


#endif