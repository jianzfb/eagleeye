#include "eagleeye/render/ImageBlend.h"
#include "eagleeye/render/GLUtils.h"
#include "glm/mat4x4.hpp"
#include "glm/ext.hpp"
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <fstream> 


namespace eagleeye{
ImageBlend::ImageBlend(){
    // 设置输出端口（拥有1个输出端口）
    this->setNumberOfOutputSignals(1);
	this->setOutputPort(new ImageSignal<float>(), 0);
	this->getOutputPort(0)->setSignalType(EAGLEEYE_SIGNAL_RECT);

    // 设置输入端口
    // port 0: foreground RGB Image
    // port 1: foreground Mask 
    // port 2: background RGB Image
    this->setNumberOfInputSignals(3);

	this->m_low_thres = 0.4f;
	this->m_high_thres = 1.0f;
	this->m_using_nonlinear_map = 1;
	EAGLEEYE_MONITOR_VAR(float, setLowThresh, getLowThresh, "low_thres", "0.0", "1.0");
	EAGLEEYE_MONITOR_VAR(float, setHighThresh, getHighThresh, "high_thres", "0.0", "1.0");
    EAGLEEYE_MONITOR_VAR(int, setNonlinearMap, getNonelinearMap, "nonlinear", "0", "2");
}   

ImageBlend::~ImageBlend(){

}

void ImageBlend::executeNodeInfo(){
	unsigned char* img_ptr = NULL;
	int img_height = 0;
	int img_width = 0;
    // 前景图像
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
		EAGLEEYE_LOGE("Dont support input port type.");
		return;
	}

    // 前景Mask图像
    if(this->getInputPort(1)->getSignalType() != EAGLEEYE_SIGNAL_MASK){
        EAGLEEYE_LOGE("Dont support input port type.");
        return;
    }
    ImageSignal<unsigned char>* mask_sig =  (ImageSignal<unsigned char>*)(this->getInputPort(1));
    Matrix<unsigned char> mask = mask_sig->getData();
    if(mask.rows() == 0 || mask.cols() == 0){
        EAGLEEYE_LOGE("Mask is emtpy.");
        return;
    }
    int mask_height = mask.rows();
    int mask_width = mask.cols();
    unsigned char* mask_ptr = mask.dataptr();

    // 背景图像
    if(this->getInputPort(2)->getSignalType() != EAGLEEYE_SIGNAL_RGB_IMAGE && this->getInputPort(2)->getSignalType() != EAGLEEYE_SIGNAL_BGR_IMAGE){
        EAGLEEYE_LOGE("Dont support input port type.");
        return;
    }
    ImageSignal<Array<unsigned char,3>>* background_img_sig = (ImageSignal<Array<unsigned char,3>>*)(this->getInputPort(2));;
    Matrix<Array<unsigned char,3>> background_img = background_img_sig->getData();
    if(background_img.rows() == 0 || background_img.cols() == 0){
        EAGLEEYE_LOGE("Background image is empty.");
        return;
    }

    if(background_img.isContinuous()){
        background_img = background_img.clone();
    }

	// 渲染过程
	EAGLEEYE_LOGD("Image width %d height %d.", img_width, img_height);
    if(m_Program == GL_NONE || m_TextureId == GL_NONE || m_MaskTextureId == GL_NONE || m_BackgrounndTextureId == GL_NONE) return;

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

	GLUtils::setInt(m_Program, "using_nonlinear", m_using_nonlinear_map);
	GLUtils::setFloat(m_Program, "low_thres", m_low_thres);
	GLUtils::setFloat(m_Program, "high_thres", m_high_thres);

	// 重新绑定纹理数据
    // 前景纹理绑定
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_TextureId);
    GLUtils::setInt(m_Program, "u_texture", 0);
	if(img_width % 4 != 0){
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);    
	}
	if(this->getInputPort(0)->getSignalType() == EAGLEEYE_SIGNAL_RGB_IMAGE){
		GLUtils::setInt(m_Program, "foreground_is_rgb", 1);
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img_width, img_height, 0, GL_RGB, GL_UNSIGNED_BYTE, img_ptr);
	}
	else if(this->getInputPort(0)->getSignalType() == EAGLEEYE_SIGNAL_BGR_IMAGE){
		// TODO 对BGR格式渲染存在问题，需要修改shader进行兼容
		GLUtils::setInt(m_Program, "foreground_is_rgb", 0);
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, img_width, img_height, 0, GL_RGB, GL_UNSIGNED_BYTE, img_ptr);
	}
	else if(this->getInputPort(0)->getSignalType() == EAGLEEYE_SIGNAL_GRAY_IMAGE){
		GLUtils::setInt(m_Program, "foreground_is_rgb", 1);
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, img_width, img_height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, img_ptr);
	}
	if(img_width % 4 != 0){
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);    
	}

    // Mask纹理绑定
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_MaskTextureId);
    GLUtils::setInt(m_Program, "m_texture", 1);
	if(mask_width % 4 != 0){
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);    
	}
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, mask_width, mask_height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, mask_ptr);
	if(mask_width % 4 != 0){
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);    
	}

    // 背景纹理绑定
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_BackgrounndTextureId);
    GLUtils::setInt(m_Program, "background_texture", 2);
	int background_width = background_img.cols();
	int background_height = background_img.rows();
	if(background_width % 4 != 0){
		glPixelStorei(GL_UNPACK_ALIGNMENT, 0);    
	}
 	if(this->getInputPort(2)->getSignalType() == EAGLEEYE_SIGNAL_RGB_IMAGE){
		GLUtils::setInt(m_Program, "background_is_rgb", 1);
		unsigned char* background_img_ptr = (unsigned char*)background_img.dataptr();
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, background_width, background_height, 0, GL_RGB, GL_UNSIGNED_BYTE, background_img_ptr);
	}
	else if(this->getInputPort(2)->getSignalType() == EAGLEEYE_SIGNAL_BGR_IMAGE){
		// TODO 对BGR格式渲染存在问题，需要修改shader进行兼容
		GLUtils::setInt(m_Program, "background_is_rgb", 0);
		unsigned char* background_img_ptr = (unsigned char*)background_img.dataptr();
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, background_width, background_height, 0, GL_RGB, GL_UNSIGNED_BYTE, background_img_ptr);
	}   
	if(background_width % 4 != 0){
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);    
	}

	// 绘制图像
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (const void *)0);

	// 解除绑定VAO
	glBindVertexArray(GL_NONE);
}

void ImageBlend::build(){
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
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, GL_NONE);

    glGenTextures(1, &m_BackgrounndTextureId);
	glBindTexture(GL_TEXTURE_2D, m_BackgrounndTextureId);
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

void ImageBlend::init(){
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
			"uniform int foreground_is_rgb;\n"
            "uniform int background_is_rgb;\n"
			"uniform float low_thres; \n"
			"uniform float high_thres; \n"
			"uniform int using_nonlinear; \n"
            "layout(location = 0) out vec4 outColor;\n"
            "uniform sampler2D u_texture;\n"
            "uniform sampler2D m_texture;\n"
            "uniform sampler2D background_texture;\n"
            "void main()\n"
            "{\n"
            "    vec4 forground_color;\n"
			"    if(foreground_is_rgb == 1){ \n"
            "        forground_color = texture(u_texture, v_texCoord); \n"
			"    } \n"
			"    else{ \n"
			"        vec4 c = texture(u_texture, v_texCoord);\n"
			"        forground_color = c; \n"
			"        forground_color.r = c.b; \n"
			"        forground_color.b = c.r; \n"
			"    }\n"
            "    vec4 background_color;\n"
            "    if(background_is_rgb == 1){\n"
            "        background_color = texture(background_texture, v_texCoord); \n"
            "    }\n"
            "    else{\n"
            "        vec4 c = texture(background_texture, v_texCoord);\n"
            "        background_color = c; \n"
			"        background_color.r = c.b; \n"
			"        background_color.b = c.r; \n"
            "    }\n"
            "    vec4 mask_val = texture(m_texture, v_texCoord);\n"
			"    if(using_nonlinear == 1){ \n"
			"        mask_val = (mask_val - low_thres)/(high_thres-low_thres); \n"
			"        mask_val = clamp(mask_val, 0.0, 1.0); \n"
			"        mask_val = mask_val*mask_val*(3.0-2.0*mask_val); \n"
			"    }\n"
			"    else{\n"
            "        if(mask_val.r < low_thres){ \n"
			"            mask_val = vec4(0.0); \n"
			"        }\n"
			"        else if(mask_val.r > high_thres){\n"
			"            mask_val = vec4(1.0);\n"
			"        } \n"
			"    }\n"
            "    outColor = mask_val*forground_color + (1.0-mask_val)*background_color;\n"
            "}";

	this->create("ImageBlend", vShaderStr, fShaderStr);
}

void ImageBlend::destroy(){
	glDeleteBuffers(3, m_VboIds);
	glDeleteTextures(1, &m_TextureId);
    glDeleteTextures(1, &m_MaskTextureId);
    glDeleteTextures(1, &m_BackgrounndTextureId);
	m_TextureId = 0;
	Superclass::destroy();
}

void ImageBlend::setLowThresh(float thres){
	this->m_low_thres = thres;
	this->modified();
}
void ImageBlend::getLowThresh(float& thres){
	thres = this->m_low_thres;
}

void ImageBlend::setHighThresh(float thres){
	if(thres < this->m_low_thres){
		EAGLEEYE_LOGE("HighThres Invalid set.");
		return;
	}

	this->m_high_thres = thres;
	this->modified();
}
void ImageBlend::getHighThresh(float& thres){
	thres = this->m_high_thres;
}

void ImageBlend::setNonlinearMap(int ok){
	this->m_using_nonlinear_map = ok;
	this->modified();
}
void ImageBlend::getNonelinearMap(int& ok){
	ok = this->m_using_nonlinear_map;
}
} // namespace eagleeye
