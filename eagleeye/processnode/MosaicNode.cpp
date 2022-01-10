#include "eagleeye/processnode/MosaicNode.h"
#include "eagleeye/framework/pipeline/AnyPipeline.h"
#include "eagleeye/basic/Variable.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/render/RenderContext.h"


namespace eagleeye
{
MosaicNode::MosaicNode(){
    // 设置输出端口（拥有1个输出端口）
    this->setNumberOfOutputSignals(1);
	this->setOutputPort(new ImageSignal<Array<unsigned char, 3>>(), 0);

    // 设置输入端口数
    // port 0: EAGLEEYE_SIGNAL_RECT
    // port 1: RGB/BGR
    this->setNumberOfInputSignals(2);

    this->m_radius = 20;
    this->m_category = 0;
    this->m_new_run = true;
    this->m_is_start = false;
    EAGLEEYE_MONITOR_VAR(int, setRadius, getRadius, "radius", "10", "80");
	EAGLEEYE_MONITOR_VAR(int, setMosaicCategory, getMosaicCategory, "category", "0", "4");

    EAGLEEYE_MOUSE_MONINTOR(
        [this](int mouse_x, int mouse_y, int mouse_action){
                this->modified();
            }
    );
}   

MosaicNode::~MosaicNode(){

}

void MosaicNode::executeNodeInfo(){
    if(this->getInputPort(0)->getSignalType() != EAGLEEYE_SIGNAL_RECT){
        EAGLEEYE_LOGD("Dont support input port type.");
        return;
    }

	if(this->getInputPort(1)->getSignalType() != EAGLEEYE_SIGNAL_RGB_IMAGE && this->getInputPort(1)->getSignalType() != EAGLEEYE_SIGNAL_BGR_IMAGE){
        EAGLEEYE_LOGD("Dont support input port type.");
        return;
    }
    if(this->m_category == 0){
        // 不做马赛克处理
        ImageSignal<Array<unsigned char, 3>>* image_sig = (ImageSignal<Array<unsigned char, 3>>*)this->getInputPort(1);
        Matrix<Array<unsigned char, 3>> image = image_sig->getData();

        ImageSignal<Array<unsigned char, 3>>* output_sig = (ImageSignal<Array<unsigned char, 3>>*)this->getOutputPort(0);
        Matrix<Array<unsigned char, 3>> mosaic_image = output_sig->getData();
        if(mosaic_image.rows() != image.rows() || mosaic_image.cols() != image.cols()){
            mosaic_image = image.clone();
        }
        else{
            mosaic_image.copy(image);
        }
        output_sig->setData(mosaic_image);
        output_sig->setSignalType(image_sig->getSignalType());
        return;
    }

    int mouse_x = 0;
    int mouse_y = 0;
    int mouse_action = -1;
    AnyPipeline::getRenderContext()->getMouse(mouse_x, mouse_y, mouse_action);
    if(mouse_action == 0){
        this->m_is_start = true;
    }
    else if(mouse_action == 2){
        this->m_is_start = false;
    }

    ImageSignal<float>* constrained_region_sig = (ImageSignal<float>*)this->getInputPort(0);
    Matrix<float> constrained_region = constrained_region_sig->getData();

    int x0 = (int)(constrained_region.at(0,0)+0.5f);
    int y0 = (int)(constrained_region.at(0,1)+0.5f);
    int w = (int)(constrained_region.at(0,2)+0.5f);
    int h = (int)(constrained_region.at(0,3)+0.5f);
    int x1 = x0 + w;
    int y1 = y0 + h;

    if(mouse_x < x0 || mouse_x > x1 || mouse_y < y0 || mouse_y > y1){
        return;
    }

    mouse_x = mouse_x - x0;
    mouse_x = eagleeye_max(mouse_x, 0);
    mouse_y = mouse_y - y0;
    mouse_y = eagleeye_max(mouse_y, 0);

    ImageSignal<Array<unsigned char, 3>>* image_sig = (ImageSignal<Array<unsigned char, 3>>*)this->getInputPort(1);
    Matrix<Array<unsigned char, 3>> image = image_sig->getData();
    int image_rows = image.rows();
    int image_cols = image.cols();

    ImageSignal<Array<unsigned char, 3>>* output_sig = (ImageSignal<Array<unsigned char, 3>>*)this->getOutputPort(0);
    Matrix<Array<unsigned char, 3>> mosaic_image = output_sig->getData();
    if(mosaic_image.rows() != image_rows || mosaic_image.cols() != image_cols || this->m_new_run){
        mosaic_image = image.clone();
        this->m_new_run = false;
    }

    if(!m_is_start){
        output_sig->setData(mosaic_image);
        output_sig->setSignalType(this->getInputPort(1)->getSignalType());
        return;
    }

    // 
    float scale_x = (float)(image_cols)/(float)(w);
    float scale_y = (float)(image_rows)/(float)(h);

    int in_img_cx = (int)(mouse_x * scale_x + 0.5f);
    int in_img_cy = (int)(mouse_y * scale_y + 0.5f);

    switch (this->m_category)
    {
    case 1:
        squareMosaic(image, mosaic_image, in_img_cx, in_img_cy);
        break;
    case 2:
        hexagonMosaic(image, mosaic_image, in_img_cx, in_img_cy);
        break;
    case 3:
        triangleMosaic(image, mosaic_image, in_img_cx, in_img_cy);
        break;
    default:
        squareMosaic(image, mosaic_image, in_img_cx, in_img_cy);
        break;
    }

    output_sig->setData(mosaic_image);
    output_sig->setSignalType(this->getInputPort(1)->getSignalType());
}

void MosaicNode::setRadius(int radius){
    this->m_radius = radius;
    this->modified();
}

void MosaicNode::getRadius(int& radius){
    radius = this->m_radius;
}

void MosaicNode::setMosaicCategory(int category){
    if(category < 0 || category > 3){
        return;
    }
    this->m_category = category;
    this->modified();

    m_new_run = true;
    m_is_start = false;
}

void MosaicNode::getMosaicCategory(int& category){
    category = this->m_category;
}

void MosaicNode::squareMosaic(Matrix<Array<unsigned char, 3>> source,
        Matrix<Array<unsigned char, 3>> result,int cx, int cy){
    int image_rows = source.rows();
    int image_cols = source.cols();

    int in_img_lx = (cx / m_radius) * m_radius;
    int in_img_rx = in_img_lx + m_radius;
    in_img_rx = eagleeye_min(in_img_rx, image_cols);

    int in_img_ty = (cy / m_radius) * m_radius;
    int in_img_by = in_img_ty + m_radius;
    in_img_by = eagleeye_min(in_img_by, image_rows);

    // int random_x = Variable<int>::uniform(in_img_lx, in_img_rx).var();
    // int random_y = Variable<int>::uniform(in_img_ty, in_img_by).var();
    int random_x = (int)((in_img_lx + in_img_rx)/2.0f);
    int random_y = (int)((in_img_ty + in_img_by)/2.0f);
    unsigned char random_r = source.at(random_y, random_x)[0];
    unsigned char random_g = source.at(random_y, random_x)[1];
    unsigned char random_b = source.at(random_y, random_x)[2];

    for(int i=in_img_ty; i<in_img_by; ++i){
        unsigned char* mosaic_ptr = (unsigned char*)result.row(i);
        for(int j=in_img_lx; j<in_img_rx; ++j){
            int offset = j*3;
            mosaic_ptr[offset] = random_r;
            mosaic_ptr[offset + 1] = random_g;
            mosaic_ptr[offset + 2] = random_b;
        }
    }
}
void MosaicNode::hexagonMosaic(Matrix<Array<unsigned char, 3>> source,
        Matrix<Array<unsigned char, 3>> result,int cx, int cy){
    int image_rows = source.rows();
    int image_cols = source.cols();

    // 确定cx,cy所在的六边形
    int hex_cx, hex_cy;
    getHexagonCenter(cx, cy, image_cols, image_rows, hex_cx, hex_cy);

    float TR = 0.866026f;
    float TB = 1.5f;

    int lx = hex_cx - (TB*this->m_radius);
    lx = eagleeye_max(lx, 0);
    int rx = hex_cx + (TB*this->m_radius);
    rx = eagleeye_min(rx, image_cols);

    int ty = hex_cy - (TR*this->m_radius);
    ty = eagleeye_max(ty, 0);
    int by = hex_cy + (TR*this->m_radius);
    by = eagleeye_min(by, image_rows);

    for(int y=ty; y<by; ++y){
        unsigned char* result_ptr = (unsigned char*)result.row(y);
        for(int x=lx; x<rx; ++x){
            int wx = int(float(x)/TB/(float)(m_radius));
            int wy = int(float(y)/TR/(float)(m_radius));

            int v1_x, v1_y, v2_x, v2_y;
            if (wx/2 * 2 == wx) {
                // 偶数列
                if (wy/2*2 == wy) {
                    // 偶数行
                    v1_x = this->m_radius * TB * float(wx);
                    v1_y = this->m_radius * TR *float(wy);

                    v2_x = this->m_radius * TB * float(wx + 1);
                    v2_y = this->m_radius * TR *float(wy + 1);
                } else {
                    // 奇数行
                    v1_x = this->m_radius * TB * float(wx);
                    v1_y = this->m_radius * TR *float(wy + 1);

                    v2_x = this->m_radius * TB * float(wx + 1);
                    v2_y = this->m_radius * TR *float(wy);
                }
            } else {
                // 奇数列
                if (wy/2*2 == wy) {
                    // 偶数行
                    v1_x = this->m_radius * TB * float(wx);
                    v1_y = this->m_radius * TR *float(wy + 1);

                    v2_x = this->m_radius * TB * float(wx + 1);
                    v2_y = this->m_radius * TR *float(wy);
                } else {
                    // 奇数行
                    v1_x = this->m_radius * TB * float(wx);
                    v1_y = this->m_radius * TR *float(wy);

                    v2_x = this->m_radius * TB * float(wx + 1);
                    v2_y = this->m_radius * TR *float(wy + 1);                  
                }
            }

            float s1 = pow(v1_x-x,2.0) + pow(v1_y-y,2.0);
            float s2 = pow(v2_x-x,2.0) + pow(v2_y-y,2.0);

            int xx, yy;
            if (s1 < s2) {
                xx = v1_x;
                yy = v1_y;
            } else {
                xx = v2_x;
                yy = v2_y;
            }
            if(xx != hex_cx || yy != hex_cy){
                continue;
            }

            xx = eagleeye_clip(xx, 0, image_cols-1);
            yy = eagleeye_clip(yy, 0, image_rows-1);

            int offset = x*3;
            result_ptr[offset] = source.at(yy,xx)[0];
            result_ptr[offset+1] = source.at(yy,xx)[1];
            result_ptr[offset+2] = source.at(yy,xx)[2];
        }
    }
}

void MosaicNode::getHexagonCenter(int cx, int cy, int image_cols, int image_rows, int& hex_cx, int& hex_cy){
    float TR = 0.866026f;
    float TB = 1.5f;
    int wx = int(float(cx)/TB/(float)(m_radius));
    int wy = int(float(cy)/TR/(float)(m_radius));

    int v1_x,v1_y,v2_x,v2_y;
    if (wx/2 * 2 == wx) {
        // 偶数列
        if (wy/2*2 == wy) {
            // 偶数行
            v1_x = this->m_radius * TB * float(wx);
            v1_y = this->m_radius * TR *float(wy);

            v2_x = this->m_radius * TB * float(wx + 1);
            v2_y = this->m_radius * TR *float(wy + 1);
        } else {
            // 奇数行
            v1_x = this->m_radius * TB * float(wx);
            v1_y = this->m_radius * TR *float(wy + 1);

            v2_x = this->m_radius * TB * float(wx + 1);
            v2_y = this->m_radius * TR *float(wy);
        }
    } else {
        // 奇数列
        if (wy/2*2 == wy) {
            // 偶数行
            v1_x = this->m_radius * TB * float(wx);
            v1_y = this->m_radius * TR *float(wy + 1);

            v2_x = this->m_radius * TB * float(wx + 1);
            v2_y = this->m_radius * TR *float(wy);
        } else {
            // 奇数行
            v1_x = this->m_radius * TB * float(wx);
            v1_y = this->m_radius * TR *float(wy);

            v2_x = this->m_radius * TB * float(wx + 1);
            v2_y = this->m_radius * TR *float(wy + 1);                  
        }
    }

    float s1 = pow(v1_x-cx,2.0) + pow(v1_y-cy,2.0);
    float s2 = pow(v2_x-cx,2.0) + pow(v2_y-cy,2.0);

    if (s1 < s2) {
        hex_cx = v1_x;
        hex_cy = v1_y;
    } else {
        hex_cx = v2_x;
        hex_cy = v2_y;
    }
    hex_cx = eagleeye_clip(hex_cx, 0, image_cols-1);
    hex_cy = eagleeye_clip(hex_cy, 0, image_rows-1);
}

void MosaicNode::triangleMosaic(Matrix<Array<unsigned char, 3>> source, 
        Matrix<Array<unsigned char, 3>> result,int cx, int cy){
    int image_rows = source.rows();
    int image_cols = source.cols();

    // 确定cx,cy所在的六边形
    int hex_cx, hex_cy;
    getHexagonCenter(cx, cy, image_cols, image_rows, hex_cx, hex_cy);

    float TR = 0.866026f;
    float TB = 1.5f;
    float PI6 = 3.1415926/6.0f;

    int lx = hex_cx - (TB*this->m_radius);
    lx = eagleeye_max(lx, 0);
    int rx = hex_cx + (TB*this->m_radius);
    rx = eagleeye_min(rx, image_cols);

    int ty = hex_cy - (TR*this->m_radius);
    ty = eagleeye_max(ty, 0);
    int by = hex_cy + (TR*this->m_radius);
    by = eagleeye_min(by, image_rows);

    for(int y=ty; y<by; ++y){
        unsigned char* result_ptr = (unsigned char*)result.row(y);
        for(int x=lx; x<rx; ++x){
            int wx = int(float(x)/TB/(float)(m_radius));
            int wy = int(float(y)/TR/(float)(m_radius));

            int v1_x,v1_y,v2_x,v2_y;
            if (wx/2 * 2 == wx) {
                // 偶数列
                if (wy/2*2 == wy) {
                    // 偶数行
                    v1_x = this->m_radius * TB * float(wx);
                    v1_y = this->m_radius * TR *float(wy);

                    v2_x = this->m_radius * TB * float(wx + 1);
                    v2_y = this->m_radius * TR *float(wy + 1);
                } else {
                    // 奇数行
                    v1_x = this->m_radius * TB * float(wx);
                    v1_y = this->m_radius * TR *float(wy + 1);

                    v2_x = this->m_radius * TB * float(wx + 1);
                    v2_y = this->m_radius * TR *float(wy);
                }
            } else {
                // 奇数列
                if (wy/2*2==wy) {
                    // 偶数行
                    v1_x = this->m_radius * TB * float(wx);
                    v1_y = this->m_radius * TR *float(wy+1);

                    v2_x = this->m_radius * TB * float(wx + 1);
                    v2_y = this->m_radius * TR *float(wy);
                } else {
                    // 奇数行
                    v1_x = this->m_radius * TB * float(wx);
                    v1_y = this->m_radius * TR *float(wy);

                    v2_x = this->m_radius * TB * float(wx + 1);
                    v2_y = this->m_radius * TR *float(wy + 1);                  
                }
            }

            float s1 = pow(v1_x-x,2.0) + pow(v1_y-y,2.0);
            float s2 = pow(v2_x-x,2.0) + pow(v2_y-y,2.0);

            int xx,yy;
            if (s1 < s2) {
                xx = v1_x;
                yy = v1_y;
            } else {
                xx = v2_x;
                yy = v2_y;
            }
            xx = eagleeye_clip(xx, 0, image_cols-1);
            yy = eagleeye_clip(yy, 0, image_rows-1);

            if(xx != hex_cx || yy != hex_cy){
                continue;
            }

            float a = atan((x-xx)/(y- yy));
            float area1_x = xx;
            float area1_y = yy - this->m_radius * TR / 2.0;

            float area2_x = xx + this->m_radius / 2.0;
            float area2_y = yy - this->m_radius * TR / 2.0;

            float area3_x = xx + this->m_radius / 2.0;
            float area3_y = yy + this->m_radius * TR / 2.0;

            float area4_x = xx;
            float area4_y = yy + this->m_radius * TR / 2.0;

            float area5_x = xx - this->m_radius / 2.0;
            float area5_y = yy + this->m_radius * TR / 2.0;

            float area6_x = xx - this->m_radius / 2.0;
            float area6_y = yy - this->m_radius * TR / 2.0;

            if (a >= PI6 && a < PI6 * 3.0) {
                xx = area1_x;
                yy = area1_y;
            } else if (a >= PI6 * 3.0 && a < PI6 * 5.0) {
                xx = area2_x;
                yy = area2_y;
            } else if ((a >= PI6 * 5.0 && a <= PI6 * 6.0)|| (a<-PI6 * 5.0 && a>- PI6 * 6.0)) {
                xx = area3_x;
                yy = area3_y;
            } else if (a < -PI6 * 3.0 && a >= -PI6 * 5.0) {
                xx = area4_x;
                yy = area4_y;
            } else if(a <= -PI6 && a> -PI6 * 3.0) {
                xx = area5_x;
                yy = area5_y;
            } else if (a > -PI6 && a < PI6) {
                xx = area6_x;
                yy = area6_y;
            }

            int offset = x*3;
            result_ptr[offset] = source.at(yy,xx)[0];
            result_ptr[offset+1] = source.at(yy,xx)[1];
            result_ptr[offset+2] = source.at(yy,xx)[2];
        }
    }
}

} // namespace eagleeye
