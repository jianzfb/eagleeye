#include "eagleeye/render/ShapeNode.h"
#include "eagleeye/framework/pipeline/LandmarkSignal.h"


namespace eagleeye
{
ShapeNode::ShapeNode(){
    // 设置输出端口（拥有1个输出端口）
    this->setNumberOfOutputSignals(1);
    this->setOutputPort(new ImageSignal<Array<unsigned char, 3>>, 0);

    // 设置输入端口
    // 端口0：形状数据
    // 端口1：渲染区域
    this->setNumberOfInputSignals(2);

    // 支持的信号类型
    //  EAGLEEYE_SIGNAL_TRACKING,
    //  EAGLEEYE_SIGNAL_POS_2D,
    //  EAGLEEYE_SIGNAL_DET,
    //  EAGLEEYE_SIGNAL_POINT,
    //  EAGLEEYE_SIGNAL_LINE,
    //  EAGLEEYE_SIGNAL_LANDMARK

    m_color_map[0] = 1.0; m_color_map[1] = 0.0; m_color_map[2] = 0.0; m_color_map[3] = 1.0; 
    m_color_map[4] = 0.0; m_color_map[5] = 1.0; m_color_map[6] = 0.0; m_color_map[7] = 1.0; 
    m_color_map[8] = 0.0; m_color_map[9] = 0.0; m_color_map[10] = 1.0; m_color_map[11] = 1.0; 
    m_color_map_num = 3;
}   

ShapeNode::~ShapeNode(){

}

void ShapeNode::processDetSignal(AnySignal* sig_0, ImageSignal<float>* sig_1){
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

    Matrix<float> render_region = sig_1->getData();
    float render_x0 = 0;
    float render_y0 = 0;
    float render_x1 = canvas_w;
    float render_y1 = canvas_h;
    if(render_region.size() > 0){
        // 区域内渲染
        render_x0 = render_region.at(0,0);
        render_y0 = render_region.at(0,1);
        render_x1 = render_x0 + render_region.at(0,2);
        render_y1 = render_y0 + render_region.at(0,3);
    }

    EAGLEEYE_LOGD("render x0 %f y0 %f x1 %f y1 %f .", render_x0, render_y0, render_x1, render_y1);

    ImageSignal<float>* det_sig = (ImageSignal<float>*)sig_0;
    Matrix<float> det_result = det_sig->getData();
    int det_obj_num = det_result.rows();
    if(det_obj_num == 0){
        return;
    }
    Matrix<float> render_rects(det_obj_num, 6);
    for(int obj_i=0; obj_i<det_obj_num; ++obj_i){
        float* det_result_ptr = det_result.row(obj_i);
        float* render_rect_ptr = render_rects.row(obj_i);
        float norm_x = det_result_ptr[0];    // 归一化坐标
        float norm_y = det_result_ptr[1];    // 归一化坐标
        float norm_w = det_result_ptr[2];    // 归一化坐标
        float norm_h = det_result_ptr[3];    // 归一化坐标

        // draw
        render_rect_ptr[0] = ((render_x0 + norm_x * (render_x1 - render_x0)) / float(screen_w)) * 2.0f - 1.0f;          // x0
        render_rect_ptr[1] = ((render_y0 + (1.0f-norm_y) * (render_y1 - render_y0)) / float(screen_h)) * 2.0f - 1.0f;          // y0
        render_rect_ptr[2] = ((render_x0 + (norm_x+norm_w) * (render_x1 - render_x0)) / float(screen_w)) * 2.0f - 1.0f; // x1
        render_rect_ptr[3] = ((render_y0 + (1.0f-(norm_y+norm_h)) * (render_y1 - render_y0)) / float(screen_h)) * 2.0f - 1.0f; // y1

        render_rect_ptr[4] = det_result_ptr[4];
        render_rect_ptr[5] = det_result_ptr[5];
    }

    drawDet(render_rects);
}

void ShapeNode::executeNodeInfo(){
    AnySignal* input_sig = this->getInputPort(0);
    ImageSignal<float>* render_region_sig = (ImageSignal<float>*)this->getInputPort(1);
    switch (input_sig->getSignalType())
    {
    case EAGLEEYE_SIGNAL_DET:
        processDetSignal(input_sig, render_region_sig);
        break;
    case EAGLEEYE_SIGNAL_TRACKING:
        processTrackingSignal(input_sig, render_region_sig);
        break;
    case EAGLEEYE_SIGNAL_LANDMARK:
        processLandmarkSignal(input_sig, render_region_sig);
        break;
    case EAGLEEYE_SIGNAL_POS_2D:
    case EAGLEEYE_SIGNAL_POINT:
        processPointSignal(input_sig, render_region_sig);
        break;
    case EAGLEEYE_SIGNAL_LINE:
        processLineSignal(input_sig, render_region_sig);
        break;
    case EAGLEEYE_SIGNAL_RECT:
        processRectSignal(input_sig, render_region_sig);
        break;
    default:
        break;
    }
}

void ShapeNode::init(){
    Superclass::init();

	char vShaderStr[] =
			"#version 300 es                          \n"
			"layout(location = 0) in vec4 vPosition;  \n"
            "layout(location = 1) uniform vec4 vColor;\n"
            "out vec4 v_color;                        \n"
			"void main()                              \n"
			"{                                        \n"
			"   gl_Position = vPosition;              \n"
            "   gl_PointSize = 10.0;                  \n"
            "   v_color = vColor;                     \n"
			"}                                        \n";

	char fShaderStr[] =
			"#version 300 es                              \n"
			"precision mediump float;                     \n"
            "in vec4 v_color;                             \n"
			"out vec4 fragColor;                          \n"
			"void main()                                  \n"
			"{                                            \n"
			"   fragColor = v_color;                      \n"
			"}                                            \n";

    this->create("shape", vShaderStr, fShaderStr);
}

void ShapeNode::drawRect(Matrix<float> rects){   
    if(m_Program == GL_NONE) return;
    int rect_num = rects.rows();
    if(rect_num == 0){
        return;
    }

    // rects
    // x0,y0,x1,y1
    GLfloat* vVertices = (GLfloat*)malloc(sizeof(float)*3*rect_num*4);
    for(int rect_i=0; rect_i<rect_num; ++rect_i){
        float* rect_ptr = rects.row(rect_i);
        // 定点1
        vVertices[(rect_i*4)*3] = rect_ptr[0];              // x
        vVertices[(rect_i*4)*3+1] = rect_ptr[1];            // y
        vVertices[(rect_i*4)*3+2] = 0.0f;                   // z

        // 定点2
        vVertices[(rect_i*4+1)*3] = rect_ptr[2];            // x
        vVertices[(rect_i*4+1)*3+1] = rect_ptr[1];          // y
        vVertices[(rect_i*4+1)*3+2] = 0.0f;                 // z

        // 定点3
        vVertices[(rect_i*4+2)*3] = rect_ptr[2];            // x
        vVertices[(rect_i*4+2)*3+1] = rect_ptr[3];          // y
        vVertices[(rect_i*4+2)*3+2] = 0.0f;                 // z

        // 定点4
        vVertices[(rect_i*4+3)*3] = rect_ptr[0];            // x
        vVertices[(rect_i*4+3)*3+1] = rect_ptr[3];          // y
        vVertices[(rect_i*4+3)*3+2] = 0.0f;                 // z
    }
	// Use the program object
	glUseProgram (m_Program);

	// Load the vertex data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
	glEnableVertexAttribArray(0);
    glLineWidth(4);
    glUniform4f(1, this->m_color_map[0], this->m_color_map[1], this->m_color_map[2], this->m_color_map[3]);

    // 顶点颜色
    for(int rect_i=0; rect_i<rect_num; ++rect_i){
        glDrawArrays(GL_LINE_LOOP, rect_i*4, 4);
    }

	glUseProgram(GL_NONE);
    free(vVertices);
}

void ShapeNode::drawDet(Matrix<float> dets){    
    // 绘制检测结果
    if(m_Program == GL_NONE) return;
    int det_num = dets.rows();
    if(det_num == 0){
        return;
    }

    // dets
    // x0,y0,x1,y1,category,propability
    GLfloat* vVertices = (GLfloat*)malloc(sizeof(float)*3*det_num*4);
    for(int rect_i=0; rect_i<det_num; ++rect_i){
        float* det_ptr = dets.row(rect_i);
        // 定点1
        vVertices[(rect_i*4)*3] = det_ptr[0];              // x
        vVertices[(rect_i*4)*3+1] = det_ptr[1];            // y
        vVertices[(rect_i*4)*3+2] = 0.0f;                  // z
        this->getRenderContext()->getXY(vVertices[(rect_i*4)*3], vVertices[(rect_i*4)*3+1]);

        // 定点2
        vVertices[(rect_i*4+1)*3] = det_ptr[2];            // x
        vVertices[(rect_i*4+1)*3+1] = det_ptr[1];          // y
        vVertices[(rect_i*4+1)*3+2] = 0.0f;                // z
        this->getRenderContext()->getXY(vVertices[(rect_i*4+1)*3], vVertices[(rect_i*4+1)*3+1]);


        // 定点3
        vVertices[(rect_i*4+2)*3] = det_ptr[2];            // x
        vVertices[(rect_i*4+2)*3+1] = det_ptr[3];          // y
        vVertices[(rect_i*4+2)*3+2] = 0.0f;                // z
        this->getRenderContext()->getXY(vVertices[(rect_i*4+2)*3], vVertices[(rect_i*4+2)*3+1]);

        // 定点4
        vVertices[(rect_i*4+3)*3] = det_ptr[0];            // x
        vVertices[(rect_i*4+3)*3+1] = det_ptr[3];          // y
        vVertices[(rect_i*4+3)*3+2] = 0.0f;                // z
        this->getRenderContext()->getXY(vVertices[(rect_i*4+3)*3], vVertices[(rect_i*4+3)*3+1]);
    }
	// Use the program object
	glUseProgram (m_Program);

	// Load the vertex data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
	glEnableVertexAttribArray(0);
    glLineWidth(4);

    for(int rect_i=0; rect_i<det_num; ++rect_i){
        float* det_ptr = dets.row(rect_i);
        int category = (int)(det_ptr[4]);
        
        // 根据类别设置定点颜色
        int color_map_index = category % this->m_color_map_num;
        int color_map_offset = color_map_index * 4;
        // color
        glUniform4f(1, 
                    this->m_color_map[color_map_offset], 
                    this->m_color_map[color_map_offset+1], 
                    this->m_color_map[color_map_offset+2], 
                    this->m_color_map[color_map_offset+3]);

        // 绘制形状
        glDrawArrays(GL_LINE_LOOP, rect_i*4, 4);
    }

	glUseProgram(GL_NONE);
    free(vVertices);
}

void ShapeNode::drawLine(Matrix<float> lines){
    if(m_Program == GL_NONE) return;
    int lines_num = lines.rows();
    if(lines_num == 0){
        return;
    }

    // lines
    // x0,y0,x1,y1
    GLfloat* vVertices = (GLfloat*)malloc(sizeof(float)*3*lines_num*2);
    for(int line_i=0; line_i<lines_num; ++line_i){
        float* line_ptr = lines.row(line_i);
        // 定点1
        vVertices[(line_i*2)*3] = line_ptr[0];              // x0
        vVertices[(line_i*2)*3+1] = line_ptr[1];            // y0
        vVertices[(line_i*2)*3+2] = 0.0f;                   // z
        this->getRenderContext()->getXY(vVertices[(line_i*2)*3], vVertices[(line_i*2)*3+1]);


        // 定点2
        vVertices[(line_i*2+1)*3] = line_ptr[2];            // x1
        vVertices[(line_i*2+1)*3+1] = line_ptr[3];          // y1
        vVertices[(line_i*2+1)*3+2] = 0.0f;                 // z
        this->getRenderContext()->getXY(vVertices[(line_i*2+1)*3], vVertices[(line_i*2+1)*3+1]);
    }
	// Use the program object
	glUseProgram (m_Program);

	// Load the vertex data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
	glEnableVertexAttribArray(0);

    glLineWidth(4);
    glUniform4f(1, this->m_color_map[0], this->m_color_map[1], this->m_color_map[2], this->m_color_map[3]);
    glDrawArrays(GL_LINES, 0, lines_num*2);

	glUseProgram(GL_NONE);
    free(vVertices);
}

void ShapeNode::drawPoint(Matrix<float> points){
    if(m_Program == GL_NONE) return;
    int point_num = points.rows();
    if(point_num == 0){
        return;
    }

    // rects
    // x0,y0,x1,y1
    GLfloat* vVertices = (GLfloat*)malloc(sizeof(float)*3*point_num);
    for(int point_i=0; point_i<point_num; ++point_i){
        float* point_ptr = points.row(point_i);
        vVertices[point_i*3] = point_ptr[0];                // x
        vVertices[point_i*3+1] = point_ptr[1];              // y
        vVertices[point_i*3+2] = 0.0f;                      // z

        this->getRenderContext()->getXY(vVertices[point_i*3], vVertices[point_i*3+1]);
    }
	// Use the program object
	glUseProgram (m_Program);

	// Load the vertex data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
	glEnableVertexAttribArray(0);
    glUniform4f(1, this->m_color_map[0], this->m_color_map[1], this->m_color_map[2], this->m_color_map[3]);

    glDrawArrays(GL_POINTS, 0, point_num);

	glUseProgram(GL_NONE);
    free(vVertices);
}

void ShapeNode::drawLandmarks(Matrix<float> landmarks){

}

void ShapeNode::processTrackingSignal(AnySignal* sig_0, ImageSignal<float>* sig_1){
}

void ShapeNode::processLandmarkSignal(AnySignal* sig_0, ImageSignal<float>* sig_1){
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

    Matrix<float> render_region = sig_1->getData();
    float render_x0 = 0;
    float render_y0 = 0;
    float render_x1 = canvas_w;
    float render_y1 = canvas_h;
    if(render_region.size() > 0){
        // 区域内渲染
        render_x0 = render_region.at(0,0);
        render_y0 = render_region.at(0,1);
        render_x1 = render_x0 + render_region.at(0,2);
        render_y1 = render_y0 + render_region.at(0,3);
    }

    EAGLEEYE_LOGD("render x0 %f y0 %f x1 %f y1 %f .", render_x0, render_y0, render_x1, render_y1);

    LandmarkSignal* landmark_sig = (LandmarkSignal*)sig_0;
    Matrix<float> landmarks = landmark_sig->getData();
    int landmark_num = landmarks.rows();
    int group_num = landmarks.cols() / 3;
    if(landmark_num == 0 || group_num == 0){
        return;
    }
    
    Matrix<float> render_points(landmark_num*group_num, 4);
    for(int group_i = 0; group_i < group_num; ++group_i){
        for(int i = 0; i < landmark_num; ++i){
            float* landmark_ptr = landmarks.row(i);
            float* render_point_ptr = render_points.row(group_i*landmark_num + i);

            float norm_x0 = landmark_ptr[group_i*3 + 0];    // 归一化坐标
            float norm_y0 = landmark_ptr[group_i*3 + 1];    // 归一化坐标
            
            // draw
            render_point_ptr[0] = ((render_x0 + norm_x0 * (render_x1 - render_x0)) / float(screen_w)) * 2.0f - 1.0f;             // x0
            render_point_ptr[1] = ((render_y0 + (1.0f-norm_y0) * (render_y1 - render_y0)) / float(screen_h)) * 2.0f - 1.0f;      // y0
        }
    }

    // 绘制点位置
    drawPoint(render_points);

    // 绘制连接线
    Matrix<int> joints = landmark_sig->getJoints();
    int joints_num = joints.rows();
    if(joints_num == 0){
        return;
    }
    Matrix<float> render_lines(joints_num * group_num, 4);
    for(int obj_i = 0; obj_i < joints_num; ++obj_i){
        int from_i = joints.at(obj_i, 0);
        int to_i = joints.at(obj_i, 1);
        
        float* from_landmark_ptr = landmarks.row(from_i);
        float* to_landmark_ptr = landmarks.row(to_i);

        for(int group_i = 0; group_i < group_num; ++group_i){
            float group_i_norm_x0 = from_landmark_ptr[group_i*3 + 0];    // 归一化坐标
            float group_i_norm_y0 = from_landmark_ptr[group_i*3 + 1];    // 归一化坐标

            float group_i_norm_x1 = to_landmark_ptr[group_i*3 + 0];     // 归一化坐标
            float group_i_norm_y1 = to_landmark_ptr[group_i*3 + 1];     // 归一化坐标

            render_lines.at(obj_i*group_num + group_i, 0) = ((render_x0 + group_i_norm_x0 * (render_x1 - render_x0)) / float(screen_w)) * 2.0f - 1.0f;             // x0
            render_lines.at(obj_i*group_num + group_i, 1) = ((render_y0 + (1.0f-group_i_norm_y0) * (render_y1 - render_y0)) / float(screen_h)) * 2.0f - 1.0f;      // y0
            render_lines.at(obj_i*group_num + group_i, 2) = ((render_x0 + group_i_norm_x1 * (render_x1 - render_x0)) / float(screen_w)) * 2.0f - 1.0f;             // x1
            render_lines.at(obj_i*group_num + group_i, 3) = ((render_y0 + (1.0f-group_i_norm_y1) * (render_y1 - render_y0)) / float(screen_h)) * 2.0f - 1.0f;      // y1
        }
    }

    drawLine(render_lines);
}

void ShapeNode::processPointSignal(AnySignal* sig_0, ImageSignal<float>* sig_1){
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

    Matrix<float> render_region = sig_1->getData();
    float render_x0 = 0;
    float render_y0 = 0;
    float render_x1 = canvas_w;
    float render_y1 = canvas_h;
    if(render_region.size() > 0){
        // 区域内渲染
        render_x0 = render_region.at(0,0);
        render_y0 = render_region.at(0,1);
        render_x1 = render_x0 + render_region.at(0,2);
        render_y1 = render_y0 + render_region.at(0,3);
    }

    EAGLEEYE_LOGD("render x0 %f y0 %f x1 %f y1 %f .", render_x0, render_y0, render_x1, render_y1);

    ImageSignal<float>* point_sig = (ImageSignal<float>*)sig_0;
    Matrix<float> points = point_sig->getData();
    int point_num = points.rows();
    if(point_num == 0){
        return;
    }
    Matrix<float> render_points(point_num, 4);
    for(int obj_i=0; obj_i<point_num; ++obj_i){
        float* point_ptr = points.row(obj_i);
        float* render_point_ptr = render_points.row(obj_i);
        float norm_x0 = point_ptr[0];    // 归一化坐标
        float norm_y0 = point_ptr[1];    // 归一化坐标

        // draw
        render_point_ptr[0] = ((render_x0 + norm_x0 * (render_x1 - render_x0)) / float(screen_w)) * 2.0f - 1.0f;             // x0
        render_point_ptr[1] = ((render_y0 + (1.0f-norm_y0) * (render_y1 - render_y0)) / float(screen_h)) * 2.0f - 1.0f;      // y0
    }

    drawPoint(render_points);
}

void ShapeNode::processLineSignal(AnySignal* sig_0, ImageSignal<float>* sig_1){
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

    Matrix<float> render_region = sig_1->getData();
    float render_x0 = 0;
    float render_y0 = 0;
    float render_x1 = canvas_w;
    float render_y1 = canvas_h;
    if(render_region.size() > 0){
        // 区域内渲染
        render_x0 = render_region.at(0,0);
        render_y0 = render_region.at(0,1);
        render_x1 = render_x0 + render_region.at(0,2);
        render_y1 = render_y0 + render_region.at(0,3);
    }

    EAGLEEYE_LOGD("render x0 %f y0 %f x1 %f y1 %f .", render_x0, render_y0, render_x1, render_y1);

    ImageSignal<float>* line_sig = (ImageSignal<float>*)sig_0;
    Matrix<float> lines = line_sig->getData();
    int line_num = lines.rows();
    if(line_num == 0){
        return;
    }
    Matrix<float> render_lines(line_num, 4);
    for(int obj_i=0; obj_i<line_num; ++obj_i){
        float* line_ptr = lines.row(obj_i);
        float* render_rect_ptr = render_lines.row(obj_i);
        float norm_x0 = line_ptr[0];    // 归一化坐标
        float norm_y0 = line_ptr[1];    // 归一化坐标
        float norm_x1 = line_ptr[2];    // 归一化坐标
        float norm_y1 = line_ptr[3];    // 归一化坐标

        // draw
        render_rect_ptr[0] = ((render_x0 + norm_x0 * (render_x1 - render_x0)) / float(screen_w)) * 2.0f - 1.0f;             // x0
        render_rect_ptr[1] = ((render_y0 + (1.0f-norm_y0) * (render_y1 - render_y0)) / float(screen_h)) * 2.0f - 1.0f;      // y0
        render_rect_ptr[2] = ((render_x0 + norm_x1 * (render_x1 - render_x0)) / float(screen_w)) * 2.0f - 1.0f;             // x1
        render_rect_ptr[3] = ((render_y0 + (1.0f-norm_y1) * (render_y1 - render_y0)) / float(screen_h)) * 2.0f - 1.0f;      // y1
    }

    drawLine(render_lines);
}

void ShapeNode::processRectSignal(AnySignal* sig_0, ImageSignal<float>* sig_1){
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

    Matrix<float> render_region = sig_1->getData();
    float render_x0 = 0;
    float render_y0 = 0;
    float render_x1 = canvas_w;
    float render_y1 = canvas_h;
    if(render_region.size() > 0){
        // 区域内渲染
        render_x0 = render_region.at(0,0);
        render_y0 = render_region.at(0,1);
        render_x1 = render_x0 + render_region.at(0,2);
        render_y1 = render_y0 + render_region.at(0,3);
    }

    EAGLEEYE_LOGD("render x0 %f y0 %f x1 %f y1 %f .", render_x0, render_y0, render_x1, render_y1);

    ImageSignal<float>* rect_sig = (ImageSignal<float>*)sig_0;
    Matrix<float> rects = rect_sig->getData();
    int rect_num = rects.rows();
    if(rect_num == 0){
        return;
    }
    Matrix<float> render_rects(rect_num, 4);
    for(int obj_i=0; obj_i<rect_num; ++obj_i){
        float* rect_ptr = rects.row(obj_i);
        float* render_rect_ptr = render_rects.row(obj_i);
        float norm_x = rect_ptr[0];    // 归一化坐标
        float norm_y = rect_ptr[1];    // 归一化坐标
        float norm_w = rect_ptr[2];    // 归一化坐标
        float norm_h = rect_ptr[3];    // 归一化坐标

        // draw
        render_rect_ptr[0] = ((render_x0 + norm_x * (render_x1 - render_x0)) / float(screen_w)) * 2.0f - 1.0f;          // x0
        render_rect_ptr[1] = ((render_y0 + (1.0f-norm_y) * (render_y1 - render_y0)) / float(screen_h)) * 2.0f - 1.0f;          // y0
        render_rect_ptr[2] = ((render_x0 + (norm_x+norm_w) * (render_x1 - render_x0)) / float(screen_w)) * 2.0f - 1.0f; // x1
        render_rect_ptr[3] = ((render_y0 + (1.0f-(norm_y+norm_h)) * (render_y1 - render_y0)) / float(screen_h)) * 2.0f - 1.0f; // y1
    }

    drawRect(render_rects);
}
} // namespace eagleeye
