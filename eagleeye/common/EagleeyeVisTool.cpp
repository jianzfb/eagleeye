#include "eagleeye/common/EagleeyeVisTool.h"
#include "eagleeye/basic/MetaOperation.h"
#include<iostream>
namespace eagleeye
{
void drawMaskOnImage(Matrix<Array<unsigned char,3>>& rgb, 
                     Matrix<unsigned char> mask, 
                     int label_num){
    Matrix<Array<float,3>> hsv = rgb.transform(RGB2HSV<Array<unsigned char,3>>());
    label_num += 1;
    std::vector<float> label_color(label_num);
    for(int i=0; i<label_num; ++i){
        label_color[i] = int(360/label_num) * i;
    }

    int h = rgb.rows(); int w = rgb.cols();
    for(int i=0; i<h; ++i){
        unsigned char* mask_ptr = mask.row(i);
        for(int j=0; j<w; ++j){
            if(mask_ptr[j] > 0){
                hsv.at(i,j)[0] = label_color[mask_ptr[j]];
                hsv.at(i,j)[1] = 1.0;
            }
        }
    }

    rgb = hsv.transform(HSV2RGB<Array<unsigned char,3>>(), rgb);
}  

void drawMaskOnImage(Matrix<Array<unsigned char,4>>& rgb, 
                     Matrix<unsigned char> mask, 
                     int label_num){
    Matrix<Array<float,3>> hsv = rgb.transform(RGB2HSV<Array<unsigned char,4>>());
    std::vector<float> label_color(label_num);
    for(int i=0; i<label_num; ++i){
        label_color[i] = int(360/label_num) * i;
    }

    int h = rgb.rows(); int w = rgb.cols();
    for(int i=0; i<h; ++i){
        unsigned char* mask_ptr = mask.row(i);
        for(int j=0; j<w; ++j){
            if(mask_ptr[j] > 0){
                hsv.at(i,j)[0] = label_color[mask_ptr[j]];
                hsv.at(i,j)[1] = 1.0;
            }
        }
    }

    rgb = hsv.transform(HSV2RGB<Array<unsigned char,4>>(), rgb);
}  
} // namespace eagleeye
