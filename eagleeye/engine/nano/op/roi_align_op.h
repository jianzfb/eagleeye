#ifndef _EAGLEEYE_ROIALIGN_OP_
#define _EAGLEEYE_ROIALIGN_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class RoiAlignOp:public BaseOp<Tensor, 3, 1>{
public:
    RoiAlignOp(int64_t pooled_h, int64_t pooled_w, float spatial_scale, bool align);
    virtual ~RoiAlignOp();

    RoiAlignOp() = default;
    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    int64_t m_pooled_h;
    int64_t m_pooled_w;
    float m_spatial_scale;
    bool m_align;
};
} // namespace dataflow
} // namespace eagleeye

#endif