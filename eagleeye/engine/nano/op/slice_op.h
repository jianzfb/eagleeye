#ifndef _EAGLEEYE_SLICE_OP_H_
#define _EAGLEEYE_SLICE_OP_H_

#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class SliceOp:public BaseOp<Tensor, 1, 1>{
public:
    SliceOp(std::vector<int> axes={}, std::vector<int> starts={}, std::vector<int> ends={});
    SliceOp(const SliceOp& op);
    virtual ~SliceOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int runOnCpu(std::vector<Tensor> input={});
    virtual int runOnGpu(std::vector<Tensor> input={});

protected:
    std::vector<int> m_axes;
    std::vector<int> m_starts;
    std::vector<int> m_ends;
};
}
}

#endif