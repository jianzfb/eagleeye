#ifndef _EAGLEEYE_YUVSHOW_H_
#define _EAGLEEYE_YUVSHOW_H_
#include "eagleeye/render/RenderNode.h"


namespace eagleeye
{
class YUVShow: public RenderNode{
public:
    typedef YUVShow             Self;
    typedef RenderNode          Superclass; 

    EAGLEEYE_CLASSIDENTITY(YUVShow);

    YUVShow();
    virtual ~YUVShow();

    /**
	 *	@brief execute Node
     *  @note user must finish this function
	 */
	virtual void executeNodeInfo();

    /**
     * @brief init render data(VBO, TEXTURE, ...)
     */ 
    virtual void build();

    /**
     * @brief update render data
     */ 
    virtual void update();

    /**
     * @brief render
     */ 
    virtual GLuint draw();

    /**
     * @brief destroy render resource
     */ 
    virtual void destroy();

    /**
     * @brief init gl 
     */ 
    void init();

private:
    YUVShow(const YUVShow&);
    void operator=(const YUVShow&);

    GLuint m_YTextureId;
    GLuint m_UTextureId;
    GLuint m_VTextureId;
    GLuint m_VboIds[3];

    int m_render_frame_width;
    int m_render_frame_height;
    unsigned char* m_render_frame_yuv;
}; 
} // namespace eagleeye


#endif