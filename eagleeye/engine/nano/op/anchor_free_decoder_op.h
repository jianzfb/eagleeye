#ifndef _EAGLEEYE_ANCHOR_FREE_DECODER_OP_
#define _EAGLEEYE_ANCHOR_FREE_DECODER_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
enum AnchorFreeDecoderType{
    ANCHORFREE_LTRB = 0
};

class AnchorFreeDecoderOp: public BaseOp<2, 1>, DynamicCreator<AnchorFreeDecoderOp>{
public:
    AnchorFreeDecoderOp(){};
    AnchorFreeDecoderOp(AnchorFreeDecoderType decoder_type, int max_per, float x_scale, float y_scale, float x_scale_ext=1.0f, float y_scale_ext=1.0f);
    AnchorFreeDecoderOp(const AnchorFreeDecoderOp& op);

    virtual ~AnchorFreeDecoderOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}

    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    void ltrb_decoder(const Tensor& score, const Tensor& ltrb, Tensor& out);

    AnchorFreeDecoderType m_decoder_type;
    int m_max_per;
    float m_x_scale;
    float m_y_scale;
    float m_x_scale_ext;
    float m_y_scale_ext;
};
} // namespace dataflow    
} // namespace eagleeye

#endif
