#ifndef _EAGLEEYE_POSE_AFFINE_OP_
#define _EAGLEEYE_POSE_AFFINE_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{

class PoseAffineOp:public BaseOp<2, 1>, DynamicCreator<PoseAffineOp>{
public:
    using BaseOp<2, 1>::init;
    PoseAffineOp() = default;
    PoseAffineOp(float x_scale, float y_scale, float x_offset, float y_offset);
    virtual ~PoseAffineOp() = default;

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    float m_x_scale;
    float m_y_scale;
    float m_x_offset;
    float m_y_offset;
};
} // namespace dataflow
} // namespace eagleeye
#endif