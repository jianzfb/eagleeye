#include "eagleeye/processnode/ImageWriteNode.h"
#include "eagleeye/common/EagleeyeIO.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
#include "eagleeye/common/EagleeyeFile.h"
#include "eagleeye/common/EagleeyeLog.h"
#include "eagleeye/framework/pipeline/AnyMonitor.h"
#include "eagleeye/common/EagleeyeStr.h"
#ifdef EAGLEEYE_PNG
#include "eagleeye/3rd/pnglib/png.h"
#endif

namespace eagleeye
{
ImageWriteNode::ImageWriteNode(){
    // 输出信号
    this->setNumberOfInputSignals(1);
    this->setNumberOfOutputSignals(1);
    this->setOutputPort(new ImageSignal<unsigned char>, 0);

    this->m_folder = "/data/local/tmp/";
    this->m_count = 0;
    EAGLEEYE_MONITOR_VAR(std::string, setWriteFolder, getWriteFolder, "folder","", "");
}

ImageWriteNode::~ImageWriteNode(){

}

void ImageWriteNode::executeNodeInfo(){
    if(!isdirexist(this->m_folder.c_str())){ 
        EAGLEEYE_LOGD("%s dont exsit, try to build", this->m_folder.c_str());
        createdirectory(this->m_folder.c_str());
    }
    EagleeyeType data_type = this->getInputPort(0)->getSignalValueType();
    if(data_type == EAGLEEYE_UCHAR || data_type == EAGLEEYE_RGB || data_type == EAGLEEYE_RGBA){
        if(data_type == EAGLEEYE_UCHAR){
            // gray image
            ImageSignal<unsigned char>* input_uchar_signal = (ImageSignal<unsigned char>*)(this->getInputPort(0));
            Matrix<unsigned char> data_uchar = input_uchar_signal->getData();
            std::string file_prefix = this->getFileNamePrefix(input_uchar_signal->name);
            std::string file_path = this->m_folder + "/" + file_prefix + ".png";
            writePngFile(file_path.c_str(), data_uchar.row(0), data_uchar.rows(), data_uchar.cols(), data_uchar.stride(), 1);
        }
        else if(data_type == EAGLEEYE_RGB){
            // rgb image
            ImageSignal<Array<unsigned char,3>>* input_rgb_signal = (ImageSignal<Array<unsigned char,3>>*)(this->getInputPort(0));
            Matrix<Array<unsigned char,3>> data_rgb = input_rgb_signal->getData();
            std::string file_prefix = this->getFileNamePrefix(input_rgb_signal->name);
            std::string file_path = this->m_folder + "/" + file_prefix + ".png";
            writePngFile(file_path.c_str(), (unsigned char*)data_rgb.row(0), data_rgb.rows(), data_rgb.cols(), data_rgb.stride(), 3);
        }
        else{
            // rgba image
            ImageSignal<Array<unsigned char,4>>* input_rgba_signal = (ImageSignal<Array<unsigned char,4>>*)(this->getInputPort(0));
            Matrix<Array<unsigned char,4>> data_rgba = input_rgba_signal->getData();
            std::string file_prefix = this->getFileNamePrefix(input_rgba_signal->name);
            std::string file_path = this->m_folder + "/" + file_prefix + ".png";
            writePngFile(file_path.c_str(), (unsigned char*)data_rgba.row(0), data_rgba.rows(), data_rgba.cols(), data_rgba.stride(), 4);
        }
    }
    else{
        if(data_type == EAGLEEYE_CHAR){
            ImageSignal<char>* input_char_signal = (ImageSignal<char>*)(this->getInputPort(0));
            Matrix<char> data_char = input_char_signal->getData();
            std::string file_prefix = this->getFileNamePrefix(input_char_signal->name);
            std::string file_path = this->m_folder + "/" + file_prefix + ".bin";
            EagleeyeIO::write(data_char, file_path.c_str(), WRITE_BINARY_MODE);
        }
        else if(data_type == EAGLEEYE_SHORT){
            ImageSignal<short>* input_short_signal = (ImageSignal<short>*)(this->getInputPort(0));
            Matrix<short> data_short = input_short_signal->getData();
            std::string file_prefix = this->getFileNamePrefix(input_short_signal->name);
            std::string file_path = this->m_folder + "/" + file_prefix + ".bin";
            EagleeyeIO::write(data_short, file_path.c_str(), WRITE_BINARY_MODE);
        }
        else if(data_type == EAGLEEYE_USHORT){
            ImageSignal<unsigned short>* input_ushort_signal = (ImageSignal<unsigned short>*)(this->getInputPort(0));
            Matrix<unsigned short> data_ushort = input_ushort_signal->getData();
            std::string file_prefix = this->getFileNamePrefix(input_ushort_signal->name);
            std::string file_path = this->m_folder + "/" + file_prefix + ".bin";
            EagleeyeIO::write(data_ushort, file_path.c_str(), WRITE_BINARY_MODE);
        }
        else if(data_type == EAGLEEYE_INT){
            ImageSignal<int>* input_int_signal = (ImageSignal<int>*)(this->getInputPort(0));
            Matrix<int> data_int = input_int_signal->getData();
            std::string file_prefix = this->getFileNamePrefix(input_int_signal->name);
            std::string file_path = this->m_folder + "/" + file_prefix + ".bin";
            EagleeyeIO::write(data_int, file_path.c_str(), WRITE_BINARY_MODE);
        }
        else if(data_type == EAGLEEYE_UINT){
            ImageSignal<unsigned int>* input_uint_signal = (ImageSignal<unsigned int>*)(this->getInputPort(0));
            Matrix<unsigned int> data_uint = input_uint_signal->getData();
            std::string file_prefix = this->getFileNamePrefix(input_uint_signal->name);
            std::string file_path = this->m_folder + "/" + file_prefix + ".bin";
            EagleeyeIO::write(data_uint, file_path.c_str(), WRITE_BINARY_MODE);
        }
        else if(data_type == EAGLEEYE_FLOAT){
            ImageSignal<float>* input_float_signal = (ImageSignal<float>*)(this->getInputPort(0));
            Matrix<float> data_float = input_float_signal->getData();
            std::string file_prefix = this->getFileNamePrefix(input_float_signal->name);
            std::string file_path = this->m_folder + "/" + file_prefix + ".bin";
            EagleeyeIO::write(data_float, file_path.c_str(), WRITE_BINARY_MODE);
        }
        else if(data_type == EAGLEEYE_DOUBLE){
            ImageSignal<double>* input_double_signal = (ImageSignal<double>*)(this->getInputPort(0));
            Matrix<double> data_double = input_double_signal->getData();
            std::string file_prefix = this->getFileNamePrefix(input_double_signal->name);
            std::string file_path = this->m_folder + "/" + file_prefix + ".bin";
            EagleeyeIO::write(data_double, file_path.c_str(), WRITE_BINARY_MODE);
        }
    }
}

void ImageWriteNode::writePngFile(const char* file_path, unsigned char* data, int height, int width, int stride, int channel){
#ifdef EAGLEEYE_PNG
    int y;
    FILE *fp = fopen(file_path, "wb");
    if(!fp) abort();

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) abort();

    png_infop info = png_create_info_struct(png);
    if (!info) abort();
    
    if (setjmp(png_jmpbuf(png))) abort();
    
    png_init_io(png, fp);

    // Output is 8bit depth, RGBA format.
    int png_color = 0;
    switch (channel)
    {
    case 1:
        png_color = PNG_COLOR_TYPE_GRAY;
        break;
    case 3:
        png_color = PNG_COLOR_TYPE_RGB;
        break;
    default:
        png_color = PNG_COLOR_TYPE_RGBA;
        break;
    }
    png_set_IHDR(
        png,
        info,
        width, height,
        8,
        png_color,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );
    png_write_info(png, info);

    // To remove the alpha channel for PNG_COLOR_TYPE_RGB format,
    // Use png_set_filler().
    //png_set_filler(png, 0, PNG_FILLER_AFTER);
    png_bytep* row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
    for(int i=0; i<height; ++i){
        row_pointers[i] = data + stride*i*channel;
    }

    png_write_image(png, row_pointers);
    png_write_end(png, NULL);

    free(row_pointers);
    fclose(fp);
    png_destroy_write_struct(&png, &info);
#endif
}

void ImageWriteNode::setWriteFolder(std::string folder){
    this->m_folder = folder;
}

void ImageWriteNode::getWriteFolder(std::string& folder){
    folder = this->m_folder;
}

std::string ImageWriteNode::getFileNamePrefix(std::string filename){
    if(filename == ""){
        std::string prefix = this->getUnitName();
        prefix = prefix + "_" + tos(m_count);
        m_count += 1;
        return prefix;
    }

    std::string sperator = ".";
    std::vector<std::string> filename_terms = split(filename,sperator);
    std::string prefix = filename_terms[0];
    prefix = prefix + "_"+tos(m_count);
    m_count += 1;
    return prefix;
}

void ImageWriteNode::reset(){
    // reset count
    this->m_count = 0;
    Superclass::reset();
}
} // namespace eagleeye
