#ifndef _EAGLEEYE_REDUCE_OP_
#define _EAGLEEYE_REDUCE_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
enum ReduceOpType{
    REDUCE_MIN =    0,
    REDUCE_MAX =    1,
    REDUCE_MEAN =   2,
    REDUCE_SUM =    3,
    REDUCE_PROD =   4
};

class ReduceOp:public BaseOp<Tensor, 1, 1>{
public:
    ReduceOp(){}
    ReduceOp(ReduceOpType op_type, std::vector<int64_t> axis, bool keep_axis);
    ReduceOp(const ReduceOp& op);
    virtual ~ReduceOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    ReduceOpType m_op_type;
    bool m_keep_axis;
    std::vector<int64_t> m_axis;

    bool m_reduce_n;
    bool m_reduce_c;
    bool m_reduce_h;
    bool m_reduce_w;
    bool m_reduce_nchw;
    bool m_reduce_hw;
    bool m_reduce_ch;
    bool m_reduce_nc;
};

class ReduceMinOp: public ReduceOp{
public:
    ReduceMinOp(){}
    ReduceMinOp(std::vector<int64_t> axis, bool keep_axis):ReduceOp(REDUCE_MIN, axis, keep_axis){}
    virtual ~ReduceMinOp(){}
};
class ReduceMaxOp: public ReduceOp{
public:
    ReduceMaxOp(){}
    ReduceMaxOp(std::vector<int64_t> axis, bool keep_axis):ReduceOp(REDUCE_MAX, axis, keep_axis){}
    virtual ~ReduceMaxOp(){}
};
class ReduceMeanOp: public ReduceOp{
public:
    ReduceMeanOp(){}
    ReduceMeanOp(std::vector<int64_t> axis, bool keep_axis):ReduceOp(REDUCE_MEAN, axis, keep_axis){}
    virtual ~ReduceMeanOp(){}
};
class ReduceSumOp: public ReduceOp{
public:
    ReduceSumOp(){}
    ReduceSumOp(std::vector<int64_t> axis, bool keep_axis):ReduceOp(REDUCE_SUM, axis, keep_axis){}
    virtual ~ReduceSumOp(){}
};
class ReduceProdOp: public ReduceOp{
public:
    ReduceProdOp(){}
    ReduceProdOp(std::vector<int64_t> axis, bool keep_axis):ReduceOp(REDUCE_PROD, axis, keep_axis){}
    virtual ~ReduceProdOp(){}
};

} // namespace dataflow
} // namespace eagleeye


#endif