#ifndef _EAGLEEYE_BLAZE_DECODER_OP_
#define _EAGLEEYE_BLAZE_DECODER_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/basic/Matrix.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{

template<std::size_t IN>
class BlazeDecoderOp: public BaseOp<Tensor, IN, 1>{
public:
    BlazeDecoderOp() = default;
    BlazeDecoderOp(Matrix<float> anchors, int landmark_num);
    virtual ~BlazeDecoderOp() = default;

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    Matrix<float> m_anchors;
    int m_landmark_num;
    int m_pad_w;
    int m_pad_h;
    float m_scale_x;
    float m_scale_y;
    std::vector<int> m_anchor_num;
};
} // namespace dataflow
} // namespace eagleeye

#include "eagleeye/engine/nano/op/blaze_decoder_op.hpp"
#endif