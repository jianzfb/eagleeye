#ifndef _EAGLEEYE_CLIP_OP_
#define _EAGLEEYE_CLIP_OP_

#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class ClipOp:public BaseOp<1, 1>, DynamicCreator<ClipOp>{
public:
    using BaseOp<1, 1>::init;
    ClipOp(float min_v, float max_v);
    ClipOp(const ClipOp& op);
    virtual ~ClipOp();

    ClipOp() = default;

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    float m_min_v;
    float m_max_v;
};
}
}

#endif