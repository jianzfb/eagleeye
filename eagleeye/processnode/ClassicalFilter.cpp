#include "eagleeye/processnode/ClassicalFilter.h"
#include "eagleeye/algorithm/filter.h"
namespace eagleeye
{
ClassicalFilter::ClassicalFilter(){
    // 设置输入端口
    this->setNumberOfInputSignals(1);

    // 设置输出端口（拥有1个输出端口）
    this->setNumberOfOutputSignals(1);
	this->setOutputPort(new ImageSignal<Array<unsigned char, 3>>(), 0);

    this->m_filter_category = 0;    // 默认无效果

	EAGLEEYE_MONITOR_VAR(int, setFilterCategory, getFilterCategory, "filter", "0", "4");
}   

ClassicalFilter::~ClassicalFilter(){

} 

void ClassicalFilter::executeNodeInfo(){
    if(this->getInputPort(0)->getSignalType() != EAGLEEYE_SIGNAL_RGB_IMAGE && this->getInputPort(0)->getSignalType() != EAGLEEYE_SIGNAL_BGR_IMAGE){
        EAGLEEYE_LOGE("Dont support input port type.");
        return;
    }

    Matrix<Array<unsigned char, 3>> rgb_img;
    // 获得输入信号
    ImageSignal<Array<unsigned char, 3>>* input_img_sig = (ImageSignal<Array<unsigned char, 3>>*)(this->getInputPort(0));
    rgb_img = input_img_sig->getData();
    if(!rgb_img.isContinuous()){
        rgb_img = rgb_img.clone();
    }

    unsigned char* img_ptr = (unsigned char*)rgb_img.dataptr();
    int img_height = rgb_img.rows();
    int img_width = rgb_img.cols();

    bool is_rgb = (this->getInputPort(0)->getSignalType() == EAGLEEYE_SIGNAL_RGB_IMAGE);
    switch (this->m_filter_category)
    {
    case 0:
        break;
    case 1:
        rgb_img = this->filterByNostalgic(rgb_img, is_rgb);
        break;    
    case 2:
        rgb_img = this->filterBySketch(rgb_img, is_rgb);
        break;
    case 3:
        rgb_img = this->filterByIce(rgb_img, is_rgb);
        break;
    default:
        break;
    }

    // 更新输出端口信号类型
    this->getOutputPort(0)->setSignalType(this->getInputPort(0)->getSignalType());
    ImageSignal<Array<unsigned char, 3>>* output_sig = (ImageSignal<Array<unsigned char, 3>>*)(this->getOutputPort(0));
    output_sig->setData(rgb_img);
}

void ClassicalFilter::setFilterCategory(int category){
    if(category < 0 || category > 3){
        return;
    }
    this->m_filter_category = category;
    this->modified();
}

void ClassicalFilter::getFilterCategory(int& category){
    category = this->m_filter_category;
}

Matrix<Array<unsigned char, 3>> ClassicalFilter::filterByNostalgic(Matrix<Array<unsigned char, 3>> img, bool is_rgb){
    int rows = img.rows();
    int cols = img.cols();
    Matrix<Array<unsigned char, 3>> result(rows, cols);

    if(is_rgb){
        for (size_t i = 0; i < rows; i++){
            unsigned char* img_ptr = (unsigned char*)img.row(i);
            unsigned char* result_ptr = (unsigned char*)result.row(i);
            for (size_t j = 0; j < cols; j++){
                size_t offset = j*3;
                float t = 0.393*img_ptr[offset] + 0.769*img_ptr[offset+1] + 0.189*img_ptr[offset+2];
                result_ptr[offset] = eagleeye_clip(t, 0, 255);
                t = 0.349*img_ptr[offset] + 0.686*img_ptr[offset+1] + 0.168*img_ptr[offset+2];
                result_ptr[offset+1] = eagleeye_clip(t, 0, 255);
                t = 0.272*img_ptr[offset] + 0.534*img_ptr[offset+1] + 0.131*img_ptr[offset+2];
                result_ptr[offset+2] = eagleeye_clip(t, 0, 255);
            }
        }
    }
    else{
        for (size_t i = 0; i < rows; i++){
            unsigned char* img_ptr = (unsigned char*)img.row(i);
            unsigned char* result_ptr = (unsigned char*)result.row(i);
            for (size_t j = 0; j < cols; j++){
                size_t offset = j*3;
                float t = 0.272*img_ptr[offset+2] + 0.534*img_ptr[offset+1] + 0.131*img_ptr[offset];
                result_ptr[offset] = eagleeye_clip(t, 0, 255);
                t = 0.349*img_ptr[offset+2] + 0.686*img_ptr[offset+1] + 0.168*img_ptr[offset];
                result_ptr[offset+1] = eagleeye_clip(t, 0, 255);
                t = 0.393*img_ptr[offset+2] + 0.769*img_ptr[offset+1] + 0.189*img_ptr[offset];
                result_ptr[offset+2] = eagleeye_clip(t, 0, 255);
            }
        }
    }

    return result;
}


Matrix<Array<unsigned char, 3>> ClassicalFilter::filterBySketch(Matrix<Array<unsigned char, 3>> img, bool is_rgb){
    int rows = img.rows();
    int cols = img.cols();
    Matrix<Array<unsigned char, 3>> result(rows, cols);

    // 1、去色
    Matrix<Array<unsigned char,3>> gray(rows, cols);
    for (size_t i = 0; i < rows; i++){
        unsigned char* img_ptr = (unsigned char*)img.row(i);
        unsigned char* gray_ptr = (unsigned char*)gray.row(i);
        for (size_t j = 0; j < cols; j++){
            int offset = j * 3;
            int max = std::max(
                std::max(img_ptr[offset], img_ptr[offset+1]),img_ptr[offset+2]
            );

            int min = std::min(
                std::min(img_ptr[offset], img_ptr[offset+1]),img_ptr[offset+2]
            );

            gray_ptr[offset] = (max + min) / 2.0f + 0.5f;
            gray_ptr[offset+1] = (max + min) / 2.0f + 0.5f;
            gray_ptr[offset+2] = (max + min) / 2.0f + 0.5f;
        }
    }

    // 2、复制去色图层，并且反色
    Matrix<float> gray_f_revesal(rows, cols);
    for (size_t i = 0; i < rows; i++){
        unsigned char* gray_ptr = (unsigned char*)gray.row(i);
        float* gray_f_revesal_ptr = gray_f_revesal.row(i);
        
        for (size_t j = 0; j < cols; j++){
            int offset = j * 3;
            gray_f_revesal_ptr[j] = 255 - gray_ptr[offset];
        }
    }

    // 3、对反色图像进行高斯模糊；
    // cv::GaussianBlur(gray_revesal, gray_revesal, cv::Size(7, 7), 0);
    Matrix<Array<unsigned char,3>> gray_revesal(rows, cols);
    Matrix<float> blur_gray_f_revesal;
    Filter bf_filter;
    bf_filter.setSigmaI(80.0);
    bf_filter.setSigmaS(7.0);
    bf_filter.fastBF(gray_f_revesal, blur_gray_f_revesal);
    for (size_t i = 0; i < rows; i++){
        unsigned char* gray_revesal_ptr = (unsigned char*)gray_revesal.row(i);
        float* blur_gray_f_revesal_ptr = blur_gray_f_revesal.row(i);
        
        for (size_t j = 0; j < cols; j++){
            int offset = j * 3;
            gray_revesal_ptr[offset] = eagleeye_clip(blur_gray_f_revesal_ptr[j],0,255);
            gray_revesal_ptr[offset+1] = gray_revesal_ptr[offset];
            gray_revesal_ptr[offset+2] = gray_revesal_ptr[offset];
        }
    }

    // 4、模糊后的图像叠加模式选择颜色减淡效果。
    // 减淡公式：C =MIN( A +（A×B）/（255-B）,255)，其中C为混合结果，A为去色后的像素点，B为高斯模糊后的像素点。
    for (size_t i = 0; i < rows; i++){
        unsigned char* gray_ptr = (unsigned char*)gray.row(i);
        unsigned char* gray_revesal_ptr = (unsigned char*)gray_revesal.row(i);
        unsigned char* result_ptr = (unsigned char*)result.row(i);

        for (size_t j = 0; j < cols; j++){
            int offset = j * 3;
            int a = gray_ptr[offset];
            int b = gray_revesal_ptr[offset];
            int c = std::min(a + (a * b) / (255 - b), 255);
            result_ptr[offset] = c;
            result_ptr[offset+1] = c;
            result_ptr[offset+2] = c;
        }
    }

    return result;
}


Matrix<Array<unsigned char, 3>> ClassicalFilter::filterByIce(Matrix<Array<unsigned char, 3>> img, bool is_rgb){
    int rows = img.rows();
    int cols = img.cols();
    Matrix<Array<unsigned char, 3>> result(rows, cols);

    for (size_t i = 0; i < rows; i++){
        unsigned char* img_ptr = (unsigned char*)img.row(i);
        unsigned char* result_ptr = (unsigned char*)result.row(i);
        for (size_t j = 0; j < cols; j++){
            size_t offset = j*3;
            int t = (std::abs(img_ptr[offset] - img_ptr[offset+1] - img_ptr[offset+2]) * 3) >> 2;
            result_ptr[offset] = eagleeye_clip(t, 0, 255);

            t = (std::abs(img_ptr[offset+1] - img_ptr[offset] - img_ptr[offset+2]) * 3) >> 2;
            result_ptr[offset+1] = eagleeye_clip(t, 0, 255);

            t = (std::abs(img_ptr[offset+2] - img_ptr[offset] - img_ptr[offset+1]) * 3) >> 2;
            result_ptr[offset+2] = eagleeye_clip(t, 0, 255);
        }
    }
    return result;
}

} // namespace eagleeye
