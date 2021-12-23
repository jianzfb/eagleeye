#include "eagleeye/common/EagleeyeImageUti.h"
#include "eagleeye/processnode/ImageReadNode.h"
#include "eagleeye/processnode/ImageWriteNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"

namespace eagleeye
{
void imread(std::string image_path, Matrix<Array<unsigned char,3>>& image){
    ImageReadNode irn;
    irn.setImagePath(image_path);
    irn.start();

    ImageSignal<Array<unsigned char,3>>* out = (ImageSignal<Array<unsigned char,3>>*)irn.getOutputPort(0);
    image = out->getData();
}

void imwrite(std::string folder, std::string name, Matrix<Array<unsigned char,3>> image){
    ImageWriteNode iwn;
    iwn.setWriteFolder(folder);
    iwn.setFileName(name);
    ImageSignal<Array<unsigned char,3>>* s = new ImageSignal<Array<unsigned char,3>>();
    s->setSignalType(EAGLEEYE_SIGNAL_RGB_IMAGE);
    s->setData(image);
    iwn.setInputPort(s, 0);
    iwn.start();
    delete s;
}

void imwrite(std::string folder, std::string name, Matrix<unsigned char> image){
    ImageWriteNode iwn;
    iwn.setWriteFolder(folder);
    iwn.setFileName(name);
    ImageSignal<unsigned char>* s = new ImageSignal<unsigned char>();
    s->setSignalType(EAGLEEYE_SIGNAL_GRAY_IMAGE);
    s->setData(image);
    iwn.setInputPort(s, 0);
    iwn.start();
    delete s;
}
} // namespace eagleeye
