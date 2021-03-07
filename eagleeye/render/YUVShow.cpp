#include "eagleeye/render/YUVShow.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/framework/pipeline/YUVSignal.h"
#include "eagleeye/render/GLUtils.h"

namespace eagleeye
{
YUVShow::YUVShow(){
    // 设置输出端口（拥有1个输出端口）
    this->setNumberOfOutputSignals(1);
    this->setOutputPort(new ImageSignal<Array<unsigned char, 4>>, 0);

    // 设置输入端口
    this->setNumberOfInputSignals(1);

    m_YTextureId = 0;
    m_UTextureId = 0;
    m_VTextureId = 0;
}

YUVShow::~YUVShow(){
    this->destroy();
}

void YUVShow::executeNodeInfo(){
    // 获得输入YUV
    YUVSignal* yuv_sig = (YUVSignal*)(this->getInputPort(0));
    m_render_frame_yuv = (unsigned char*)(yuv_sig->getData().cpu());
    m_render_frame_width = yuv_sig->getWidth();
    m_render_frame_height = yuv_sig->getHeight();
    EAGLEEYE_LOGD("render width %d height %d", m_render_frame_width, m_render_frame_height);

    // 渲染
    this->draw();
}

void YUVShow::destroy(){
    if(m_YTextureId){
        glDeleteBuffers(3, m_VboIds);
        glDeleteTextures(1, &m_YTextureId);
        glDeleteTextures(1, &m_UTextureId);
        glDeleteTextures(1, &m_VTextureId);

        m_YTextureId = 0;
        m_UTextureId = 0;
        m_VTextureId = 0;
    }

    Superclass::destroy();
}

void YUVShow::build(){
    // 创建纹理
    glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &m_YTextureId);
	glBindTexture(GL_TEXTURE_2D, m_YTextureId);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (!m_YTextureId){
		GLUtils::CheckGLError("Create Y texture");
        return;
	}

	glActiveTexture(GL_TEXTURE1);
	glGenTextures(1, &m_UTextureId);
	glBindTexture(GL_TEXTURE_2D, m_UTextureId);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (!m_UTextureId){
		GLUtils::CheckGLError("Create U texture");
        return;
	}

	glActiveTexture(GL_TEXTURE2);
	glGenTextures(1, &m_VTextureId);
	glBindTexture(GL_TEXTURE_2D, m_VTextureId);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	if (!m_VTextureId){
		GLUtils::CheckGLError("Create V texture");
        return;
	}

    // 创建VAO内容
	GLfloat verticesCoords[] = {
			-1.0f,  1.0f, 0.0f,  // Position 0
			-1.0f, -1.0f, 0.0f,  // Position 1
			1.0f,  -1.0f, 0.0f,  // Position 2
			1.0f,   1.0f, 0.0f,  // Position 3
	};

	GLfloat textureCoords[] = {
			0.0f,  0.0f,        // TexCoord 0
			0.0f,  1.0f,        // TexCoord 1
			1.0f,  1.0f,        // TexCoord 2
			1.0f,  0.0f         // TexCoord 3
	};

	GLushort indices[] = { 0, 1, 2, 0, 2, 3 };
    glGenBuffers(3, m_VboIds);
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verticesCoords), verticesCoords, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoords), textureCoords, GL_STATIC_DRAW);
	glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_NONE);
}

void YUVShow::update(){
    // 重新绑定纹理数据
    unsigned char* y_data = this->m_render_frame_yuv;
    glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_YTextureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, (GLsizei) this->m_render_frame_width,
				 (GLsizei) this->m_render_frame_height, 0,
				 GL_LUMINANCE, GL_UNSIGNED_BYTE, y_data);

    unsigned char* u_data = y_data + this->m_render_frame_width*this->m_render_frame_height;
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_UTextureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, (GLsizei) this->m_render_frame_width >> 1,
				 (GLsizei) this->m_render_frame_height >> 1, 0,
				 GL_LUMINANCE, GL_UNSIGNED_BYTE, u_data);

    unsigned char* v_data = u_data + (this->m_render_frame_width >> 1)*(this->m_render_frame_height >> 1);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_VTextureId);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, (GLsizei) this->m_render_frame_width >> 1,
				 (GLsizei) this->m_render_frame_height >> 1, 0,
				 GL_LUMINANCE, GL_UNSIGNED_BYTE, v_data);
}

GLuint YUVShow::draw(){
    if (!m_Program){
		EAGLEEYE_LOGD("UseProgram Could not use program.");
		return 0;
	}

    // use program
    glUseProgram(m_Program);

    // update render data
    this->update();

    // render elements
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const void *)0);
    glBindVertexArray(GL_NONE);

    return 0;
}

void YUVShow::init(){
    Superclass::init();

    char vShaderStr[] =
        "#version 320 es\n"
        "layout(location = 0) in vec4 position;\n"
        "layout(location = 1) in vec2 texcoord;\n"
        "out vec2 v_texcoord;\n"
        "void main() {\n"
        "    v_texcoord = texcoord;\n"
        "    gl_Position = position;\n"
        "}";

    char fShaderStr[] =
        "#version 320 es\n"
        "precision highp float;\n"
        "in vec2 v_texcoord;\n"
        "uniform lowp sampler2D s_textureY;\n"
        "uniform lowp sampler2D s_textureU;\n"
        "uniform lowp sampler2D s_textureV;\n"
        "void main() {\n"
        "    float y, u, v, r, g, b;\n"
        "    y = texture2D(s_textureY, v_texcoord).r;\n"
        "    u = texture2D(s_textureU, v_texcoord).r;\n"
        "    v = texture2D(s_textureV, v_texcoord).r;\n"
        "    u = u - 0.5;\n"
        "    v = v - 0.5;\n"
        "    r = y + 1.403 * v;\n"
        "    g = y - 0.344 * u - 0.714 * v;\n"
        "    b = y + 1.770 * u;\n"
        "    gl_FragColor = vec4(r, g, b, 1.0);\n"
        "}";

    this->Superclass::create(vShaderStr, fShaderStr);
}
} // namespace eagleeye
