#ifndef _EAGLEEYE_POSE_DECODER_OP_
#define _EAGLEEYE_POSE_DECODER_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>


namespace eagleeye{
namespace dataflow{
class PoseDecoderOp:public BaseOp<2, 1>,DynamicCreator<PoseDecoderOp>{
public:
    using BaseOp<2, 1>::init;
    PoseDecoderOp() = default;
    PoseDecoderOp(float score_thre);
    PoseDecoderOp(const PoseDecoderOp& op);
    virtual ~PoseDecoderOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    void decoderByHeatmap(const Tensor& heatmap, float score_thre, Tensor& joints_pres, float x_offset, float y_offset, float x_scale, float y_scale);

    float m_score_thre;
};
} // namespace dataflow
} // namespace eagleeye


#endif



