#ifndef _EAGLEEYE_KEEPRATIO_BY_SCALE_OP_
#define _EAGLEEYE_KEEPRATIO_BY_SCALE_OP_

#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class KeepRatioByScaleOp:public BaseOp<1, 2>, DynamicCreator<KeepRatioByScaleOp>{
public:
    using BaseOp<1, 2>::init;
    KeepRatioByScaleOp();
    KeepRatioByScaleOp(const KeepRatioByScaleOp& op);
    virtual ~KeepRatioByScaleOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}

    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    float m_ratio;
    std::vector<int> m_out_size;    // width, height
    Tensor m_temp;
};
}
}

#endif