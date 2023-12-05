#ifndef _EAGLEEYE_RENDERBASE_H_
#define _EAGLEEYE_RENDERBASE_H_
#include "eagleeye/common/EagleeyeMacro.h"
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>


namespace eagleeye
{
class RenderBase{
public:
    RenderBase();
    virtual ~RenderBase();

    /**
     * @brief init render context
     */ 
    void create(const char* program_name, const char *pVertexShaderSource, const char *pFragShaderSource);

    /**
     * @brief init
     */ 
    virtual void create(){};

    /**
     * @brief init render data
     */ 
    virtual void build(){};

    /**
     * @brief update render data
     */ 
    virtual void update(){};

    /**
     * @brief draw elements
     */ 
    virtual GLuint draw(){};

    /**
     * @brief destroy GPU resource
     */ 
    virtual void destroy();

protected:
    GLuint m_Program;
    GLuint m_VAO;
    bool m_finish_ini;
}; 
} // namespace eagleeye

#endif