#include "eagleeye/render/RenderBase.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/render/GLUtils.h"

namespace eagleeye
{
RenderBase::RenderBase(){
    m_VAO = 0;
    m_Program = 0;
    m_VertexShader = 0;
    m_FragmentShader = 0;
}

RenderBase::~RenderBase(){
}

void RenderBase::createProgram(const char *pVertexShaderSource, const char *pFragShaderSource){
    // try to destroy existed shader program
    if(m_Program){
        this->destroy();
        EAGLEEYE_LOGD("Destroy existed program.");
    }

    // create new shader
    m_Program = 
            GLUtils::CreateProgram(pVertexShaderSource, 
                                    pFragShaderSource,
                                    m_VertexShader,
									m_FragmentShader);
	if (!m_Program){
		GLUtils::CheckGLError("Create Program");
		EAGLEEYE_LOGE("CreateProgram Could not create program.");
	}
}

void RenderBase::destroy(){
    if(m_Program){
        // 删除shader资源
        GLUtils::DeleteProgram(m_Program);

        // 删除VAO
        glDeleteVertexArrays(1, &m_VAO);

        // clear 
        this->m_Program = 0;
        this->m_VertexShader = 0;
        this->m_FragmentShader = 0;
        this->m_VAO = 0;
    }
}

void RenderBase::create(const char *pVertexShaderSource, const char *pFragShaderSource){
    // 0.step 尝试删除已有资源
    this->destroy();

    // 1.step 创建shader program
    this->createProgram(pVertexShaderSource, pFragShaderSource);

    // 2.step 创建并绑定VAO
    glGenVertexArrays(1, &m_VAO);
	glBindVertexArray(m_VAO);
    
    // 3.step 初始化绑定到VAO的数据
    this->build();
    
    // 4.step 取消VAO绑定
    glBindVertexArray(GL_NONE);
}
} // namespace eagleeye
