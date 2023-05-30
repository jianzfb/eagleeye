#ifndef _EAGLEEYE_COMPARE_OP_
#define _EAGLEEYE_COMPARE_OP_
#include "eagleeye/engine/nano/dataflow/base.h"
#include "eagleeye/basic/Tensor.h"
#include <string>
#include <vector>

namespace eagleeye{
namespace dataflow{
enum CompareOpType{
    EQUAL_COMPARE = 0,
    NOT_EQUAL_COMPARE,
    LESS_THAN_COMPARE,
    LESS_EQUAL_COMPARE,
    GREATER_THAN_COMPARE,
    GREATER_EQUAL_COMPARE
};

class CompareOp:public BaseOp<Tensor, 2, 1>{
public:
    CompareOp(CompareOpType compare_op_type);
    virtual ~CompareOp();

    CompareOp() = default;
    
    virtual int init(std::map<std::string, std::vector<float>> params);
    virtual int init(std::map<std::string, std::vector<std::vector<float>>> params){return 0;};
    virtual int init(std::map<std::string, std::vector<std::string>> params){return 0;}
    
    virtual int runOnCpu(const std::vector<Tensor>& input);
    virtual int runOnGpu(const std::vector<Tensor>& input);

protected:
    CompareOpType m_compare_op_type;
};

} // namespace dataflow
} // namespace eagleeye


#endif