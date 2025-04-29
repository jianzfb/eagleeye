#ifndef _EAGLEEYE_ARCFACEALIGN_OP_
#define _EAGLEEYE_ARCFACEALIGN_OP_

#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

namespace eagleeye{
namespace dataflow{
class ArcFaceAlignOp:public BaseOp<3, 1>, DynamicCreator<ArcFaceAlignOp>{
public:
    using BaseOp<3, 1>::init;
    ArcFaceAlignOp();    
    ArcFaceAlignOp(int target_h, int target_w, int margin=0);
    virtual ~ArcFaceAlignOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}

    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    int m_target_h;
    int m_target_w;
    int m_margin;
    float m_margin_ratio;

    int m_src_handler;
    int m_tgt_handler;

    void* m_src_ptr;
    void* m_tgt_ptr;

    cv::Mat m_dst_points;
};
}
}

#endif