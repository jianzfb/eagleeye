#ifndef _EAGLEEYE_ARGMAX_OP_
#define _EAGLEEYE_ARGMAX_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class ArgmaxOp:public BaseOp<Tensor, 1, 1>{
public:
    ArgmaxOp(int64_t axis);
    virtual ~ArgmaxOp();
    
    ArgmaxOp() = default;

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}

    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    int64_t m_axis;
};
} // namespace dataflow
} // namespace eagleey

#endif