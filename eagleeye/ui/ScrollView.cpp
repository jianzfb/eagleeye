#include "eagleeye/ui/ScrollView.h"
#include "eagleeye/framework/pipeline/EmptySignal.h"
#include "eagleeye/framework/pipeline/StateSignal.h"

namespace eagleeye
{
ScrollView::ScrollView(int direction){
    // 设置输出端口（拥有1个输出端口）
    this->setNumberOfOutputSignals(1);
    this->setOutputPort(new EmptySignal(), 0);

    // 设置输入端口
    // 端口1：EAGLEEYE_SIGNAL_POS_2D
    // 端口2：EAGLEEYE_SIGNAL_STATE
    this->setNumberOfInputSignals(2);
    this->m_direction = direction;

    this->m_view_num = 5;
    this->m_view_h = 0.3f;
    this->m_view_w = 0.6f;
    this->m_padding_ratio = 0.1f;

    this->m_x_offset = 0.5f;
    this->m_y_offset = 0.0f;

    this->m_default_color[0] = 1.0;
    this->m_default_color[1] = 0.0;
    this->m_default_color[2] = 0.0;
    this->m_default_color[3] = 1.0;

    this->m_select_color[0] = 0.0;
    this->m_select_color[1] = 1.0;
    this->m_select_color[2] = 0.0;
    this->m_select_color[3] = 1.0;

    this->m_lock_color[0] = 0.0;
    this->m_lock_color[1] = 1.0;
    this->m_lock_color[2] = 0.0;
    this->m_lock_color[3] = 1.0;

    this->m_last_x = 0.0f;
    this->m_last_y = 0.0f;

    this->m_last_x_offset = 0.0f;
    this->m_last_y_offset = 0.0f;
}    

ScrollView::~ScrollView(){

}

void ScrollView::executeNodeInfo(){
    if(this->getInputPort(0)->getSignalType() != EAGLEEYE_SIGNAL_POS_2D || this->getInputPort(1)->getSignalType() != EAGLEEYE_SIGNAL_STATE){
        EAGLEEYE_LOGE("SIGNAL TYPE not consitent.");
        return;
    }
    if(this->m_view_num == 0){
        EAGLEEYE_LOGE("View num = 0.");
        return;
    }

    if(m_Program == GL_NONE) return;

    // 位置信号
    ImageSignal<float>* offset_sig = (ImageSignal<float>*)this->getInputPort(0);
    Matrix<float> offset_data = offset_sig->getData();
    float current_x_offset = 0.0f;
    float current_y_offset = 0.0f;
    // 选择状态信号
    // 0:默认，1:锁定状态，2：激活状态，3：丢失状态，4：失活状态
    StateSignal* state_sig = (StateSignal*)this->getInputPort(1);
    int current_state = state_sig->getData();

    switch (current_state)
    {
    case 1:
        // 锁定状态
        current_x_offset = 0.0f;
        current_y_offset = 0.0f;

        this->m_last_x = offset_data.at(0,0);
        this->m_last_y = offset_data.at(0,1);
        break;
    case 2:
        // 激活状态
        current_x_offset = offset_data.at(0,0) - this->m_last_x;
        current_y_offset = offset_data.at(0,1) - this->m_last_y;

        this->m_last_x = offset_data.at(0,0);
        this->m_last_y = offset_data.at(0,1);
        break;
    case 3:
        // 丢失状态
        current_x_offset = 0.0f;
        current_y_offset = 0.0f;
        break;
    case 0:
    case 4:
        // 默认无效状态
        current_x_offset = 0.0f;
        current_y_offset = 0.0f;
        break;
    default:
        current_x_offset = 0.0f;
        current_y_offset = 0.0f;
        break;
    }

    if(this->m_direction == 0){
        // 水平方向
        this->m_last_y_offset = this->m_y_offset;
        this->m_last_x_offset += current_x_offset;

        // 检查最大偏移
        if(this->m_last_x_offset + (this->m_view_num-1)*this->m_view_w + m_padding_ratio*this->m_view_w*(this->m_view_num-1) < 0.0f){
            this->m_last_x_offset = 0.0f - ((this->m_view_num-1)*this->m_view_w + m_padding_ratio*this->m_view_w*(this->m_view_num-1));
        }

        if(this->m_last_x_offset + this->m_view_w + m_padding_ratio*this->m_view_w > 1.0f){
            this->m_last_x_offset = 1.0f - (this->m_view_w + m_padding_ratio*this->m_view_w);
        }
    }
    else{
        // 竖直方向
        this->m_last_x_offset = this->m_x_offset;
        this->m_last_y_offset += current_y_offset;
    }

    float x_offset = this->m_last_x_offset;
    float y_offset = this->m_last_y_offset;
    float view_h = this->m_view_h;
    float view_w = this->m_view_w;

    GLfloat* vVertices = (GLfloat*)malloc(sizeof(float)*3*this->m_view_num*4);
    for(int view_i=0; view_i<this->m_view_num; ++view_i){
        float view_x0 = 0.0f;
        float view_y0 = 0.0f;
        float view_x1 = 0.0f;
        float view_y1 = 0.0f;

        if(this->m_direction == 0){
            // 水平方向
            view_x0 = x_offset + view_i*view_w + m_padding_ratio*view_w*view_i;        
            view_y0 = y_offset;

            view_x1 = view_x0 + view_w;
            view_y1 = y_offset + view_h;

            // 转换坐标
            view_x0 = view_x0*2-1.0f;
            view_y0 = (1.0f-view_y0)*2-1.0f;
            view_x1 = view_x1*2-1.0f;
            view_y1 = (1.0f-view_y1)*2-1.0f;
        }
        else{
            // 竖直方向
            view_x0 = x_offset;
            view_y0 = y_offset + view_i*view_h + m_padding_ratio*view_h*view_i;

            view_x1 = x_offset + view_w;
            view_y1 = view_y0 + view_h;

            // 转换坐标
            view_x0 = view_x0*2-1.0f;
            view_y0 = (1.0f-view_y0)*2-1.0f;
            view_x1 = view_x1*2-1.0f;
            view_y1 = (1.0f-view_y1)*2-1.0f;
        }

        vVertices[(view_i*4)*3] = view_x0;
        vVertices[(view_i*4)*3+1] = view_y0;
        vVertices[(view_i*4)*3+2] = 0.0f;

        vVertices[(view_i*4+1)*3] = view_x1;
        vVertices[(view_i*4+1)*3+1] = view_y0;
        vVertices[(view_i*4+1)*3+2] = 0.0f;

        vVertices[(view_i*4+2)*3] = view_x1;
        vVertices[(view_i*4+2)*3+1] = view_y1;
        vVertices[(view_i*4+2)*3+2] = 0.0f;

        vVertices[(view_i*4+3)*3] = view_x0;
        vVertices[(view_i*4+3)*3+1] = view_y1;
        vVertices[(view_i*4+3)*3+2] = 0.0f;
    }

	// Use the program object
	glUseProgram (m_Program);
	// Load the vertex data
    glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
    // 发现首个位于中心位置的视口
    float screen_center_x = 0.0f;
    float screen_center_y = 0.0f;
    
    int focus_view_i = -1;
    float dist = 1000000.0f;
    for(int view_i=0; view_i<m_view_num; ++view_i){
        float view_center_x = (vVertices[(view_i*4)*3] + vVertices[(view_i*4+1)*3])/2.0f;
        float view_center_y = (vVertices[(view_i*4)*3+1] + vVertices[(view_i*4+2)*3+1])/2.0f;
        if(this->m_direction == 0){
            // 考虑x
            float check_dist = abs(view_center_x - screen_center_x);
            if(check_dist < dist){
                dist = check_dist;
                focus_view_i = view_i;
            }
        }
        else{
            // 考虑y
            float check_dist = abs(view_center_y - screen_center_y);
            if(check_dist < dist){
                dist = check_dist;
                focus_view_i = view_i;
            }
        }
    }

    for(int view_i=0; view_i<m_view_num; ++view_i){
        if((current_state == 2 || current_state == 3) && view_i == focus_view_i){
            // 激活状态
            glUniform4f(1, this->m_select_color[0], this->m_select_color[1], this->m_select_color[2], this->m_select_color[3]);
        }
        else{
            // 默认状态
            glUniform4f(1, this->m_default_color[0], this->m_default_color[1], this->m_default_color[2], this->m_default_color[3]);
        }
        glDrawArrays(GL_TRIANGLE_FAN, view_i*4, 4);
    }

	glUseProgram(GL_NONE);
    free(vVertices);
}

void ScrollView::init(){
    Superclass::init();

	char vShaderStr[] =
			"#version 300 es                          \n"
			"layout(location = 0) in vec4 vPosition;  \n"
            "layout(location = 1) uniform vec4 vColor;     \n"
            "out vec4 v_color;                        \n"
			"void main()                              \n"
			"{                                        \n"
			"   gl_Position = vPosition;              \n"
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

    this->create("scrollview", vShaderStr, fShaderStr);
}

void ScrollView::setViewNum(int num){
    this->m_view_num = num;
}

void ScrollView::setViewHW(float h, float w){
    this->m_view_h = h;
    this->m_view_w = w;
}

void ScrollView::setOffset(float x_v, float y_v){
    this->m_x_offset = x_v;
    this->m_y_offset = y_v;
}
} // namespace eagleeye

