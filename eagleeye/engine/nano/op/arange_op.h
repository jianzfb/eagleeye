#ifndef _EAGLEEYE_ARANGE_OP_H_
#define _EAGLEEYE_ARANGE_OP_H_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class ArangeOp: public BaseOp<Tensor, 0, 1>{
public:
    ArangeOp() = default;
    ArangeOp(int64_t start, int64_t stop, int64_t step, EagleeyeType data_type);
    ArangeOp(const ArangeOp& op);
    virtual ~ArangeOp(){};

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    int64_t m_start;
    int64_t m_stop;
    int64_t m_step;
    EagleeyeType m_data_type;
};
} // namespace dataflow
} // namespace eagleeye
#endif