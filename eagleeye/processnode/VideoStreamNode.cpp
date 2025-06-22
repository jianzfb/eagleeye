#include "eagleeye/processnode/VideoStreamNode.h"
#include "eagleeye/framework/pipeline/SignalFactory.h"
namespace eagleeye{
VideoStreamNode::VideoStreamNode(int queue_size, bool get_then_auto_remove, bool set_then_auto_remove){
    // 设置输出节点
    this->m_queue_size = queue_size;
    this->m_set_then_auto_remove = set_then_auto_remove;
    this->m_get_then_auto_remove = get_then_auto_remove;
    AnySignal* sig = new ImageSignal<Array<unsigned char, 3>>();
    sig->transformCategoryToQ(this->m_queue_size, m_get_then_auto_remove, m_set_then_auto_remove);
    this->setNumberOfOutputSignals(1);
    this->setOutputPort(sig, 0);
    m_decoder = new RKH264Decoder();
}

VideoStreamNode::~VideoStreamNode(){
    delete m_decoder;
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