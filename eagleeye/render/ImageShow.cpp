#include "eagleeye/render/ImageShow.h"
#include "glm/mat4x4.hpp"
#include "glm/ext.hpp"
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <fstream> 
#include "eagleeye/render/GLUtils.h"


namespace eagleeye{
ImageShow::ImageShow(){
    // 设置输出端口（拥有1个输出端口）
    this->setNumberOfOutputSignals(1);
    this->setOutputPort(new ImageSignal<Array<unsigned char, 3>>, 0);

    // 设置输入端口
    this->setNumberOfInputSignals(1);
}   

ImageShow::~ImageShow(){
	this->destroy();
}

void ImageShow::executeNodeInfo(){
    // 渲染过程
    // 获得输入信号
    ImageSignal<Array<unsigned char, 3>>* input_img_sig = (ImageSignal<Array<unsigned char, 3>>*)(this->getInputPort(0));

    // 获得输入信号的图像
    Matrix<Array<unsigned char, 3>> img = input_img_sig->getData();
    if(!img.isContinuous()){
        img = img.clone();
    }

    unsigned char* img_ptr = (unsigned char*)img.dataptr();
    int img_height = img.rows();
    int img_width = img.cols();
	EAGLEEYE_LOGD("Image width %d height %d.", img_width, img_height);

    if(m_Program == GL_NONE || m_TextureId == GL_NONE) return;

	// 计算归一化坐标
	int screen_w = this->getScreenW();
	int screen_h = this->getScreenH();
	int canvas_x = this->getCanvasX();
	int canvas_y = this->getCanvasY();
	int canvas_w = this->getCanvasW();
	int canvas_h = this->getCanvasH();
	if(canvas_w == 0 || canvas_h == 0){
		canvas_x = 0;
		canvas_y = 0;
		canvas_w = this->getScreenW();
		canvas_h = this->getScreenH();
	}
	EAGLEEYE_LOGD("Canvas x %d y %d width %d height %d.", canvas_x, canvas_y, canvas_w, canvas_h);

	// 计算图片显示位置
	bool is_ok = false;
	float scale = float(canvas_w) / float(img_width);
	if(scale * img_height <= canvas_h){
		is_ok = true;
	}
	if(!is_ok){
		scale = float(canvas_h) / float(img_height);
	}
	float scaled_img_width = scale * float(img_width);
	float scaled_img_height = scale * float(img_height);

	float img_x0 = (canvas_w - scaled_img_width)/2.0f + canvas_x;
	float img_y0 = (canvas_h - scaled_img_height)/2.0f + canvas_y;
	float img_x1 = (canvas_w - scaled_img_width)/2.0f + scaled_img_width + canvas_x;
	float img_y1 = (canvas_h - scaled_img_height)/2.0f + scaled_img_height + canvas_y;
	EAGLEEYE_LOGD("Image position x0 %f y0 %f x1 %f y1 %f.", img_x0, img_y0, img_x1, img_y1);

	// 图像中心对齐canvas中心
	GLfloat update_vertices_coords[] = {
			-1.0f,  1.0f, 0.0f,
			-1.0f, -1.0f, 0.0f,
			1.0f,  -1.0f, 0.0f,
			1.0f,   1.0f, 0.0f,
	};

	float x0 = (img_x0 / float(screen_w)) * 2.0f - 1.0f;
	float y0 = (img_y0 / float(screen_h)) * 2.0f - 1.0f;
	float x1 = (img_x1 / float(screen_w)) * 2.0f - 1.0f;
	float y1 = (img_y1 / float(screen_h)) * 2.0f - 1.0f;

	update_vertices_coords[0] = x0;		// left_top x
	update_vertices_coords[1] = y0;		// left_top y

	update_vertices_coords[3] = x0;		// left_bottom x
	update_vertices_coords[4] = y1;		// left_bottom y

	update_vertices_coords[6] = x1;		// right_bottom x
	update_vertices_coords[7] = y1;		// right_bottom y

	update_vertices_coords[9] = x1;		// right_top x
	update_vertices_coords[10] = y0;	// right_top y

	// 重新绑定定点坐标
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(update_vertices_coords), update_vertices_coords, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);
	
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

	glUseProgram(m_Program);
	glBindVertexArray(m_VAO);

	// 重新绑定纹理数据
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_TextureId);
    GLUtils::setInt(m_Program, "u_texture", 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img_width, img_height, 0, GL_RGB, GL_UNSIGNED_BYTE, img_ptr);

	// 绘制图像
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const void *)0);

	// 解除绑定VAO
	glBindVertexArray(GL_NONE);
}

void ImageShow::build(){
	// create RGB texture
	glGenTextures(1, &m_TextureId);
	glBindTexture(GL_TEXTURE_2D, m_TextureId);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, GL_NONE);

	// 创建纹理
	GLfloat verticesCoords[] = {
			-1.0f,  1.0f, 0.0f,  // Position 0
			-1.0f, -1.0f, 0.0f,  // Position 1
			1.0f,  -1.0f, 0.0f,  // Position 2
			1.0f,   1.0f, 0.0f,  // Position 3
	};

	GLfloat textureCoords[] = {
			0.0f,  1.0f,        // TexCoord 0
			0.0f,  0.0f,        // TexCoord 1
			1.0f,  0.0f,        // TexCoord 2		
			1.0f,  1.0f,        // TexCoord 3
	};

	GLushort indices[] = { 0, 1, 2, 0, 2, 3 };

	// Generate VBO Ids and load the VBOs with data
	glGenBuffers(3, m_VboIds);
	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verticesCoords), verticesCoords, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoords), textureCoords, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

	glBindBuffer(GL_ARRAY_BUFFER, m_VboIds[1]);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (const void *)0);
	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VboIds[2]);
}

void ImageShow::init(){
	// 1.step 调用管线初始化
	Superclass::init();

	// 2.step 创建shader, create()函数将调用build()构建渲染对象
	char vShaderStr[] =
            "#version 320 es\n"
            "layout(location = 0) in vec4 a_position;\n"
            "layout(location = 1) in vec2 a_texCoord;\n"
            "out vec2 v_texCoord;\n"
            "void main()\n"
            "{\n"
            "    gl_Position = a_position;\n"
            "    v_texCoord = a_texCoord;\n"
            "}";

	char fShaderStr[] =
            "#version 320 es\n"
            "#extension GL_EXT_texture_buffer : require\n"
			"precision mediump float;\n"
            "in vec2 v_texCoord;\n"
            "layout(location = 0) out vec4 outColor;\n"
            "uniform sampler2D u_texture;\n"
            "void main()\n"
            "{\n"
            "    outColor = texture(u_texture, v_texCoord);\n"
            "}";

	this->create(vShaderStr, fShaderStr);
}

void ImageShow::destroy(){
	glDeleteBuffers(3, m_VboIds);
	glDeleteTextures(1, &m_TextureId);

	m_TextureId = 0;
	Superclass::destroy();
}
} // namespace eagleeye
