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
    m_decoder = new RKH264Decoder();
}

VideoStreamNode::~VideoStreamNode(){

}

void VideoStreamNode::executeNodeInfo(){
    // do nothing
}

void VideoStreamNode::decode(uint8_t* package_data, int package_size){
    std::vector<Matrix<Array<unsigned char, 3>>> frame_list;
    m_decoder->decode(package_data, package_size, frame_list);
    ImageSignal<Array<unsigned char, 3>>* out_sig = (ImageSignal<Array<unsigned char, 3>>*)this->getOutputPort(0);
    for(int frame_i=0; frame_i<frame_list.size(); ++frame_i){
        MetaData meta;
        out_sig->setData(frame_list[frame_i], meta);
    }

    if(frame_list.size() > 0){
        this->modified();
    }
}

void VideoStreamNode::postexit(){
    ImageSignal<Array<unsigned char, 3>>* out_sig = (ImageSignal<Array<unsigned char, 3>>*)this->getOutputPort(0);
    MetaData meta;
    out_sig->setData(Matrix<Array<unsigned char, 3>>(0,0), meta);
}
}