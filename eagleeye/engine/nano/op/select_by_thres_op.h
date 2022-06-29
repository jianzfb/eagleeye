#ifndef _EAGLEEYE_SELECT_BY_THRES_OP_
#define _EAGLEEYE_SELECT_BY_THRES_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
class SelectByThresOp: public BaseOp<Tensor, 1, 2>{
public:
    SelectByThresOp() = default;
    SelectByThresOp(std::vector<float> thres, int64_t index, bool attach_index=false, bool exclusive=false);  
    SelectByThresOp(const SelectByThresOp& op);

    virtual ~SelectByThresOp();

    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    std::vector<float> m_thres;
    int64_t m_index;
    bool m_exclusive;
    bool m_attach_index;
};
} // namespace dataflow
} // namespace eagleeye
#endif