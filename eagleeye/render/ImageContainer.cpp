#include "eagleeye/render/ImageContainer.h"
#include "eagleeye/render/GLUtils.h"
#include "glm/mat4x4.hpp"
#include "glm/ext.hpp"
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <fstream> 


namespace eagleeye{
ImageContainer::ImageContainer(){
    // 设置输出端口（拥有1个输出端口）
    this->setNumberOfOutputSignals(1);
    this->setOutputPort(new ImageSignal<Array<unsigned char, 3>>, 0);

    this->m_split_w = 1;
    this->m_split_h = 1;

    this->m_margin_x = 0;
    this->m_margin_y = 0;
    this->m_padding_x = 0;
    this->m_padding_y = 0;
}   

ImageContainer::~ImageContainer(){
    if(m_input_signals_cp.size() != 0){
        for(int signal_i = 0; signal_i<this->getNumberOfInputSignals(); ++signal_i){
            delete m_input_signals_cp[signal_i];
        }
        m_input_signals_cp.clear();
    }
}

void ImageContainer::executeNodeInfo(){
    // 获得绘制区域
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
    EAGLEEYE_LOGD("Container canvas x %d y %d width %d height %d", canvas_x, canvas_y, canvas_w, canvas_h);

    // 复制信号
    int space_w = (canvas_w - 2*this->m_margin_x - (this->m_split_w-1)*this->m_padding_x) / this->m_split_w;
    int space_h = (canvas_h - 2*this->m_margin_y - (this->m_split_h-1)*this->m_padding_y) / this->m_split_h;

    if(m_input_signals_cp.size() != this->getNumberOfInputSignals()){
        if(m_input_signals_cp.size() != 0){
            for(int signal_i = 0; signal_i<this->getNumberOfInputSignals(); ++signal_i){
                delete m_input_signals_cp[signal_i];
            }
            m_input_signals_cp.clear();
        }

        for(int index=0; index<this->m_imageshow_list.size(); ++index){
            AnySignal* signal_cp = this->getInputPort(index)->make();
            // 记录
            m_input_signals_cp.push_back(signal_cp);
            // 注册到绘制节点
            this->m_imageshow_list[index]->setInputPort(signal_cp , 0);
        }
    }

    for(int index=0; index<this->m_imageshow_list.size(); ++index){
        if(index < this->getNumberOfInputSignals() && index < this->m_split_w * this->m_split_h){
            m_input_signals_cp[index]->copy(this->getInputPort(index));

            int r_i = index / this->m_split_w;
            int c_i = index - r_i*this->m_split_w;

            // 切分窗口,并设置画布
            int x_i=0;
            if(c_i == 0){
                x_i = canvas_x + this->m_margin_x + c_i*space_w;
            }
            else{
                x_i = canvas_x + this->m_margin_x + c_i*space_w + (c_i-1)*m_padding_x;
            }

            int y_i=0;
            if(r_i == 0){
                y_i = canvas_y + this->m_margin_y + r_i*space_h;
            }
            else{
                y_i = canvas_y + this->m_margin_y + r_i*space_h + (r_i-1)*m_padding_y;
            }

            this->m_imageshow_list[index]->setCanvas(x_i,y_i,space_w, space_h);
        }
    }

    // 渲染边框信息
    // 
    
    // 渲染图像信息
    for(int index=0; index<this->m_imageshow_list.size(); ++index){
        this->m_imageshow_list[index]->start();
    }
}

void ImageContainer::init(){
    Superclass::init();

    //创建 imageshow
    int num = this->getNumberOfInputSignals();
    if(this->m_imageshow_list.size() == 0){
        for(int i=0; i<num; ++i){
            this->m_imageshow_list.push_back(
                std::shared_ptr<ImageShow>(new ImageShow(), [](ImageShow* x){delete x;})
            );
        }
    }

    // 初始化
    for(int i=0; i<num; ++i){
        this->m_imageshow_list[i]->init();
    }
    if(this->m_split_h < 1 || this->m_split_w < 1){
        this->m_split_h = 1;
        this->m_split_w = 1;
    }

    // 设置画布背景色
    glClearColor(1.0f,1.0f,1.0f, 1.0f);
}

void ImageContainer::setHSplit(int split_h){
    this->m_split_h = split_h;
    this->modified();
}
void ImageContainer::getHSplit(int& split_h){
    split_h = this->m_split_h;
}

void ImageContainer::setWSplit(int split_w){
    this->m_split_w = split_w;
    this->modified();
}
void ImageContainer::getWSplit(int& split_w){
    split_w = this->m_split_w;
}

void ImageContainer::setMarginX(int margin_x){
    this->m_margin_x = margin_x;
    this->modified();
}

void ImageContainer::getMarginX(int& margin_x){
    margin_x = this->m_margin_x;
}

void ImageContainer::setMarginY(int margin_y){
    this->m_margin_y = margin_y;
    this->modified();
}

void ImageContainer::getMarginY(int& margin_y){
    margin_y = this->m_margin_y;
}

void ImageContainer::setPaddingX(int padding_x){
    this->m_padding_x = padding_x;
    this->modified();
}

void ImageContainer::getPaddingX(int& padding_x){
    padding_x = this->m_padding_x;
}

void ImageContainer::setPaddingY(int padding_y){
    this->m_padding_y = padding_y;
    this->modified();
}

void ImageContainer::getPaddingY(int& padding_y){
    padding_y = this->m_padding_y;
}
}