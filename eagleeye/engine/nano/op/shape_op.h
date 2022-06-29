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
    ShapeOp(int64_t start=-1, int64_t stop=-1, EagleeyeType data_type=EAGLEEYE_INT);
    ShapeOp(const ShapeOp& op);
    virtual ~ShapeOp();

    ShapeOp() = default;
    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

private:
    int64_t m_start;
    int64_t m_stop;
    EagleeyeType m_data_type;
};

} // namespace dataflow
} // namespace eagleeye


#endif