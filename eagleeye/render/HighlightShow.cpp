#include "eagleeye/render/HighlightShow.h"
#include "eagleeye/render/GLUtils.h"
#include "glm/mat4x4.hpp"
#include "glm/ext.hpp"
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <fstream> 


namespace eagleeye{
HighlightShow::HighlightShow(){
    // 设置输出端口（拥有1个输出端口）
    this->setNumberOfOutputSignals(1);
	this->setOutputPort(new ImageSignal<float>(), 0);
	this->getOutputPort(0)->setSignalType(EAGLEEYE_SIGNAL_RECT);

    // 设置输入端口
    // 0: RGB 
    // 1: MASK 高亮着色 
    this->setNumberOfInputSignals(2);

	this->m_TextureId = 0;
	this->m_MaskTextureId = 0;
}   

HighlightShow::~HighlightShow(){

}

void HighlightShow::executeNodeInfo(){
	unsigned char* img_ptr = NULL;
	int img_height = 0;
	int img_width = 0;
	if(this->getInputPort(0)->getSignalType() == EAGLEEYE_SIGNAL_RGB_IMAGE || this->getInputPort(0)->getSignalType() == EAGLEEYE_SIGNAL_BGR_IMAGE){
		// 获得输入信号
		ImageSignal<Array<unsigned char, 3>>* input_img_sig = (ImageSignal<Array<unsigned char, 3>>*)(this->getInputPort(0));
		Matrix<Array<unsigned char, 3>> img = input_img_sig->getData();
		if(!img.isContinuous()){
			img = img.clone();
		}

		img_ptr = (unsigned char*)img.dataptr();
		img_height = img.rows();
		img_width = img.cols();
	}	
	else if(this->getInputPort(0)->getSignalType() ==  EAGLEEYE_SIGNAL_GRAY_IMAGE){
		// 获得输入信号
		ImageSignal<unsigned char>* input_img_sig = (ImageSignal<unsigned char>*)(this->getInputPort(0));
		Matrix<unsigned char> img = input_img_sig->getData();
		if(!img.isContinuous()){
			img = img.clone();
		}

		img_ptr = (unsigned char*)img.dataptr();
		img_height = img.rows();
		img_width = img.cols();
	}
	else{
		EAGLEEYE_LOGE("Dont support input port-1 type");
		return;
	}

    if(this->getInputPort(1)->getSignalType() != EAGLEEYE_SIGNAL_MASK){
        EAGLEEYE_LOGE("Dont support port-2 type (needed EAGLEEYE_SIGNAL_MASK)");
        return;
    }

    // 高亮区域
    ImageSignal<unsigned char>* mask_sig = (ImageSignal<unsigned char>*)(this->getInputPort(1));
    Matrix<unsigned char> mask = mask_sig->getData();
    if(!mask.isContinuous()){
        mask = mask.clone();
    }
	int mask_rows = mask.rows();
	int mask_cols = mask.cols();
    unsigned char* mask_ptr = mask.dataptr();
	EAGLEEYE_LOGD("hightlight mask width %d height %d", mask_cols, mask_rows);

	// 渲染过程
	EAGLEEYE_LOGD("Image width %d height %d.", img_width, img_height);
    if(m_Program == GL_NONE || m_TextureId == GL_NONE || m_MaskTextureId == GL_NONE) return;

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
	EAGLEEYE_LOGD("Screen width %d height %d", screen_w, screen_h);

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

	// 输出信号1：渲染区域
	ImageSignal<float>* render_region_sig = (ImageSignal<float>*)(this->getOutputPort(0));
	Matrix<float> render_region(1,4);	// x,y,w,h
	render_region.at(0,0) = img_x0;
	render_region.at(0,1) = img_y0;
	render_region.at(0,2) = img_x1-img_x0;
	render_region.at(0,3) = img_y1-img_y0;
	render_region_sig->setData(render_region);

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
	this->getRenderContext()->getXY(update_vertices_coords[0], update_vertices_coords[1]);

	update_vertices_coords[3] = x0;		// left_bottom x
	update_vertices_coords[4] = y1;		// left_bottom y
	this->getRenderContext()->getXY(update_vertices_coords[3], update_vertices_coords[4]);

	update_vertices_coords[6] = x1;		// right_bottom x
	update_vertices_coords[7] = y1;		// right_bottom y
	this->getRenderContext()->getXY(update_vertices_coords[6], update_vertices_coords[7]);

	update_vertices_coords[9] = x1;		// right_top x
	update_vertices_coords[10] = y0;	// right_top y
	this->getRenderContext()->getXY(update_vertices_coords[9], update_vertices_coords[10]);

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

	// 绑定RGB纹理数据
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_TextureId);
    GLUtils::setInt(m_Program, "u_texture", 0);
	if(img_width % 4 != 0){
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);    
	}
	if(this->getInputPort(0)->getSignalType() == EAGLEEYE_SIGNAL_RGB_IMAGE){
		GLUtils::setInt(m_Program, "is_rgb", 1);
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img_width, img_height, 0, GL_RGB, GL_UNSIGNED_BYTE, img_ptr);
	}
	else if(this->getInputPort(0)->getSignalType() == EAGLEEYE_SIGNAL_BGR_IMAGE){
		// TODO 对BGR格式渲染存在问题，需要修改shader进行兼容
		GLUtils::setInt(m_Program, "is_rgb", 0);
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img_width, img_height, 0, GL_RGB, GL_UNSIGNED_BYTE, img_ptr);
	}
	else if(this->getInputPort(0)->getSignalType() == EAGLEEYE_SIGNAL_GRAY_IMAGE){
		GLUtils::setInt(m_Program, "is_rgb", 1);
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, img_width, img_height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, img_ptr);
	}
	if(img_width % 4 != 0){
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);    
	}

    // 绑定MASK纹理数据
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_MaskTextureId);
    GLUtils::setInt(m_Program, "m_texture", 1);
	if(mask_cols % 4 != 0){
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);    
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, mask_cols, mask_rows, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, mask_ptr);
	if(mask_cols % 4 != 0){
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);    
	}

	// 绘制图像
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const void *)0);

	// 解除绑定VAO
	glBindVertexArray(GL_NONE);
}

void HighlightShow::build(){
	// create RGB texture
	glGenTextures(1, &m_TextureId);
	glBindTexture(GL_TEXTURE_2D, m_TextureId);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, GL_NONE);

    glGenTextures(1, &m_MaskTextureId);
	glBindTexture(GL_TEXTURE_2D, m_MaskTextureId);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
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

void HighlightShow::init(){
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
			"uniform int is_rgb;\n"
			"vec3 colormap[10]=vec3[10](vec3(0.2081, 0.1663, 0.5292),vec3(0.1802, 0.2832, 0.7634),vec3(0.0116, 0.4203, 0.8805),vec3(0.0761, 0.4974, 0.8418),vec3(0.0408, 0.5874, 0.8217),vec3(0.0238, 0.6585, 0.7696),vec3(0.1258, 0.7049, 0.6775),vec3(0.3177, 0.7394, 0.563),vec3(0.5418, 0.749, 0.4613),vec3(0.7261, 0.7405, 0.3874)); \n"
            "layout(location = 0) out vec4 outColor;\n"
            "uniform sampler2D u_texture;\n"
            "uniform sampler2D m_texture;\n"
            "void main()\n"
            "{\n"
            "    vec4 rgb_v = texture(u_texture, v_texCoord); \n"
            "    vec4 mask_v = texture(m_texture, v_texCoord); \n"
			"    int label = int(mask_v.r * 255.0); \n"
			"    if(is_rgb != 1){ \n"
			"        vec4 c = texture(u_texture, v_texCoord);\n"
			"        rgb_v = c; \n"
			"        rgb_v.r = c.b; \n"
			"        rgb_v.b = c.r; \n"
			"    }\n"
            "    if(label > 0){ \n"
			"        vec3 label_color = colormap[label%10]; \n"
            "        rgb_v.r = clamp(rgb_v.r*0.2 + label_color.r*0.8,0.0,1.0); \n"
			"		 rgb_v.g = clamp(rgb_v.g*0.2 + label_color.g*0.8,0.0,1.0); \n"
			"        rgb_v.b = clamp(rgb_v.b*0.2 + label_color.b*0.8,0.0,1.0); \n"
            "    } \n"
			"    outColor = rgb_v; 	\n"
            "}";

	this->create("HighlightShow", vShaderStr, fShaderStr);
}

void HighlightShow::destroy(){
	glDeleteBuffers(3, m_VboIds);
	glDeleteTextures(1, &m_TextureId);
    glDeleteTextures(1, &m_MaskTextureId);

	m_TextureId = 0;
    m_MaskTextureId = 0;
	Superclass::destroy();
}
} // namespace eagleeye
