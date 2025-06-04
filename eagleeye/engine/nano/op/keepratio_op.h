#ifndef _EAGLEEYE_KEEPRATIO_OP_
#define _EAGLEEYE_KEEPRATIO_OP_

#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/dynamiccreater.h"
#include "eagleeye/engine/nano/op/interpolate_op.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class KeepRatioOp:public BaseOp<1, 2>, DynamicCreator<KeepRatioOp>{
public:
    using BaseOp<1, 2>::init;
    KeepRatioOp();
    KeepRatioOp(float ratio);
    KeepRatioOp(const KeepRatioOp& op);
    virtual ~KeepRatioOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}

    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    float m_ratio;
};

class ResizeKeepRatioOp:public BaseOp<1,2>, DynamicCreator<ResizeKeepRatioOp>{
public:
    using BaseOp<1, 2>::init;
    ResizeKeepRatioOp();
    ResizeKeepRatioOp(std::vector<int64_t> out_size, InterpolateOpType op_type);
    ResizeKeepRatioOp(const ResizeKeepRatioOp& op);
    virtual ~ResizeKeepRatioOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}

    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    std::vector<int64_t> m_out_size;
    InterpolateOpType m_op_type;
};
}
}

#endif