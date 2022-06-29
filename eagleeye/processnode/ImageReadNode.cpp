#include "eagleeye/processnode/ImageReadNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/common/EagleeyeFile.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/framework/pipeline/AnyMonitor.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/3rd/pnglib/png.h"
namespace eagleeye
{
ImageReadNode::ImageReadNode(){
	this->setNumberOfOutputSignals(1);
	this->setOutputPort(new ImageSignal<Array<unsigned char,3>>,0);
    this->getOutputPort(0)->setSignalType(EAGLEEYE_SIGNAL_RGB_IMAGE);

    EAGLEEYE_MONITOR_VAR(std::string, setImagePath, getImagePath, "file","", "");
}

ImageReadNode::~ImageReadNode(){

}

void ImageReadNode::executeNodeInfo(){
    if(this->getNumberOfInputSignals() > 0 && this->getInputPort(0)->getSignalType() == EAGLEEYE_SIGNAL_FILE){
        StringSignal* input_sig = (StringSignal*)(this->getInputPort(0));
        std::string image_path = input_sig->getData();
        this->setImagePath(image_path);
    }

    if(this->m_image_path == ""){
        EAGLEEYE_LOGD("dont set image path");
        return;        
    }

    if(!isfileexist(this->m_image_path.c_str())){
        EAGLEEYE_LOGD("image file %s is not exist", this->m_image_path.c_str());
        return;
    }

    EAGLEEYE_LOGD("read image file %s", this->m_image_path.c_str());
    readPngFile(this->m_image_path.c_str());
}    

void ImageReadNode::setImagePath(std::string image_path){
    this->m_image_path = image_path;
    this->modified();
}

void ImageReadNode::getImagePath(std::string& image_path){
    image_path = this->m_image_path;
}

void ImageReadNode::readPngFile(const char *filename){
    EAGLEEYE_LOGD("parse png file %s", filename);
    FILE *fp = fopen(filename, "rb");

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if(!png) abort();

    png_infop info = png_create_info_struct(png);
    if(!info) abort();

    if(setjmp(png_jmpbuf(png))) abort();

    png_init_io(png, fp);

    png_read_info(png, info);

    int width      = png_get_image_width(png, info);
    int height     = png_get_image_height(png, info);
    int color_type = png_get_color_type(png, info);
    int bit_depth  = png_get_bit_depth(png, info);

    // Read any color_type into 8bit depth, RGBA format.
    // See http://www.libpng.org/pub/png/libpng-manual.txt

    if(bit_depth == 16)
        png_set_strip_16(png);

    if(color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);

    // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
    if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);

    if(png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

    // These color_type don't have an alpha channel then fill it with 0xff.
    if(color_type == PNG_COLOR_TYPE_RGB ||
        color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

    if(color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);

    png_read_update_info(png, info);

    png_bytep* row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
    for(int y = 0; y < height; y++) {
        row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png,info));
    }
    png_read_image(png, row_pointers);

    if(color_type == PNG_COLOR_TYPE_GRAY){
        // gray image
         Matrix<Array<unsigned char,3>> rgb_img(height, width);
        for(int y=0; y<height; ++y){
            Array<unsigned char,3>* rgb_img_ptr = rgb_img.row(y);
            for(int x=0; x<width; ++x){
                rgb_img_ptr[x][0] = *(row_pointers[y] + x);
                rgb_img_ptr[x][1] = *(row_pointers[y] + x);
                rgb_img_ptr[x][2] = *(row_pointers[y] + x);
            }
        }

        ImageSignal<Array<unsigned char,3>>* rgb_sig =  (ImageSignal<Array<unsigned char,3>>*)this->getOutputPort(0);
        rgb_sig->setData(rgb_img);
    }
    else if(color_type == PNG_COLOR_TYPE_RGB){
        // rgb image
        Matrix<Array<unsigned char,3>> rgb_img(height, width);
        for(int y=0; y<height; ++y){
            Array<unsigned char,3>* rgb_img_ptr = rgb_img.row(y);
            for(int x=0; x<width; ++x){
                rgb_img_ptr[x][0] = *(row_pointers[y] + x*4);
                rgb_img_ptr[x][1] = *(row_pointers[y] + x*4 + 1);
                rgb_img_ptr[x][2] = *(row_pointers[y] + x*4 + 2);
            }
        }

        ImageSignal<Array<unsigned char,3>>* rgb_sig =  (ImageSignal<Array<unsigned char,3>>*)this->getOutputPort(0);
        rgb_sig->setData(rgb_img);
    }
    else if(color_type == PNG_COLOR_TYPE_RGBA){
        // rgba image
        Matrix<Array<unsigned char,3>> rgb_img(height, width);
        for(int y=0; y<height; ++y){
            Array<unsigned char,3>* rgb_img_ptr = rgb_img.row(y);
            for(int x=0; x<width; ++x){
                rgb_img_ptr[x][0] = *(row_pointers[y] + x*4);
                rgb_img_ptr[x][1] = *(row_pointers[y] + x*4 + 1);
                rgb_img_ptr[x][2] = *(row_pointers[y] + x*4 + 2);
            }
        }

        ImageSignal<Array<unsigned char,3>>* rgb_sig =  (ImageSignal<Array<unsigned char,3>>*)this->getOutputPort(0);
        rgb_sig->setData(rgb_img);
    }

    for(int y=0; y<height; ++y){
        free(row_pointers[y]);
    }
    free(row_pointers);

    fclose(fp);
    png_destroy_read_struct(&png, &info, NULL);
}
} // namespace eagleeye
