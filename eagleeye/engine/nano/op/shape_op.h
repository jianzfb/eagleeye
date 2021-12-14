#ifndef _EAGLEEYE_SHAPE_OP_
#define _EAGLEEYE_SHAPE_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{

class ShapeOp:public BaseOp<Tensor, 1, 1>{
public:
    ShapeOp(std::vector<int64_t> axes);
    ShapeOp(const ShapeOp& op);
    virtual ~ShapeOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int runOnCpu(std::vector<Tensor> input=std::vector<Tensor>{});
    virtual int runOnGpu(std::vector<Tensor> input=std::vector<Tensor>{});

private:
    std::vector<int64_t> m_axes;
};

} // namespace dataflow
} // namespace eagleeye


#endif