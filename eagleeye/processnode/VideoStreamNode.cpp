#include "eagleeye/processnode/VideoStreamNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
namespace eagleeye{
VideoStreamNode::VideoStreamNode(int queue_size){
    // 设置输出节点
    this->m_queue_size = queue_size;
    AnySignal* sig = new ImageSignal<Array<unsigned char, 3>>();
    sig->transformCategoryToQ(this->m_queue_size);
    this->setNumberOfOutputSignals(1);
    this->setOutputPort(sig, 0);
}

VideoStreamNode::~VideoStreamNode(){

}

void VideoStreamNode::executeNodeInfo(){
    // do nothing
}

void VideoStreamNode::decode(uint8_t* package_data, int package_size){
    std::vector<Matrix<Array<unsigned char, 3>>> frame_list;
    m_decoder->decode(package_data, package_size, frame_list);

    std::cout<<"AA get decode frames "<<frame_list.size()<<std::endl;

    ImageSignal<Array<unsigned char, 3>>* out_sig = (ImageSignal<Array<unsigned char, 3>>*)this->getOutputPort(0);
    std::cout<<"out_sig "<<(void*)out_sig<<std::endl;
    for(int frame_i=0; frame_i<frame_list.size(); ++frame_i){
        std::cout<<"set data frame "<<frame_i<<std::endl;
        MetaData meta;
        out_sig->setData(frame_list[frame_i], meta);
    }

    if(frame_list.size() > 0){
        this->modified();
    }
}
}