#ifndef _EAGLEEYE_POSE_AFFINE_OP_
#define _EAGLEEYE_POSE_AFFINE_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{

class PoseAffineOp:public BaseOp<Tensor, 2, 1>{
public:
    PoseAffineOp() = default;
    PoseAffineOp(float x_scale=1.0f, float y_scale=1.0f, float x_offset=0.0f, float y_offset=0.0f);
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