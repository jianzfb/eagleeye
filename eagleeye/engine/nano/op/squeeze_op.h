#ifndef _EAGLEEYE_SQUEEZE_OP_
#define _EAGLEEYE_SQUEEZE_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include "eagleeye/engine/nano/op/interpolate_op.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{

class SqueezeOp: public BaseOp<Tensor, 1, 1>{
public:
    SqueezeOp() = default;
    SqueezeOp(size_t axis);
    SqueezeOp(const SqueezeOp& op);
    virtual ~SqueezeOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

private:
    size_t m_axis;
}; 

class UnSqueezeOp: public BaseOp<Tensor, 1, 1>{
public:
    UnSqueezeOp() = default;
    UnSqueezeOp(size_t axis);
    UnSqueezeOp(const UnSqueezeOp& op);
    virtual ~UnSqueezeOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

private:
    size_t m_axis;
}; 
}    
}
#endif