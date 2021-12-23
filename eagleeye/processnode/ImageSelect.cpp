#include "eagleeye/processnode/ImageSelect.h"
#include "eagleeye/framework/pipeline/AnyPipeline.h"
#include "eagleeye/common/EagleeyeStr.h"
#include "eagleeye/common/EagleeyeFile.h"
namespace eagleeye{
ImageSelect::ImageSelect(std::vector<std::string> image_list){
    this->m_background_index = 0;
    this->background_img_list = image_list;
    EAGLEEYE_MONITOR_VAR(int, setIndex, getIndex, "background", "0", "100");
}   

ImageSelect::~ImageSelect(){

}

void ImageSelect::executeNodeInfo(){
    // 读取背景图片
    Superclass::executeNodeInfo();
}

void ImageSelect::setIndex(int index){
    std::string background_image_path;
    std::string resource_folder = this->getPipeline()->resourceFolder();
    std::string node_name = this->getUnitName();
    if(node_name == ""){
        node_name = "ImageSelect";
    }
    m_background_index = index;    

    if(this->background_img_list.size() > 0){
        index = index % this->background_img_list.size();
        background_image_path = resource_folder + "/images/"+node_name+"/"+this->background_img_list[m_background_index];
    }
    else{
        std::string index_str = tos(index);
        std::string image_name_str = index_str + ".png";
        background_image_path = resource_folder + "/images/"+node_name+"/"+image_name_str;
    }
    if(!isfileexist(background_image_path.c_str())){
        EAGLEEYE_LOGD("image file %s is not exist", background_image_path.c_str());
        return;
    }

    this->setImagePath(background_image_path);
}

void ImageSelect::getIndex(int& index){
    index = m_background_index;
}

void ImageSelect::init(){
    // 获得背景图片个数
    std::string resource_folder = this->getPipeline()->resourceFolder();
    std::string node_name = this->getUnitName();
    if(node_name == ""){
        node_name = "ImageSelect";
    }    
    std::string background_image_path;    
    
    if(this->background_img_list.size() > 0){
        background_image_path = resource_folder + "/images/"+node_name+"/"+this->background_img_list[0];
    }
    else{
        std::string image_name_str = "0.png";
        background_image_path = resource_folder + "/images/"+node_name+"/"+image_name_str;
    }
    if(!isfileexist(background_image_path.c_str())){
        EAGLEEYE_LOGD("image file %s is not exist", background_image_path.c_str());
        return;
    }

    this->setImagePath(background_image_path);
}
}